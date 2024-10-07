import dash
from dash import dcc, html
from dash.dependencies import Input, Output, State
import plotly.graph_objs as go
import requests
from datetime import datetime
import pytz
 
# Constants for IP and port
IP_ADDRESS = "54.235.126.90"
PORT_STH = 8666
DASH_HOST = "0.0.0.0"  # Set this to "0.0.0.0" to allow access from any IP
 
# Function to get luminosity data from the API
def get_environment_data(path, lastN):
    url = f"http://{IP_ADDRESS}:{PORT_STH}/STH/v1/contextEntities/type/Lamp/id/urn:ngsi-ld:Lamp:2005/attributes/{path}?lastN={lastN}"
    headers = {
        'fiware-service': 'smart',
        'fiware-servicepath': '/'
    }
    response = requests.get(url, headers=headers)
    if response.status_code == 200:
        data = response.json()
        try:
            values = data['contextResponses'][0]['contextElement']['attributes'][0]['values']
            return values
        except KeyError as e:
            print(f"Key error: {e}")
            return []
    else:
        print(f"Error accessing {url}: {response.status_code}")
        return []
 
# Function to convert UTC timestamps to São Paulo time
def convert_to_sao_paulo_time(timestamps):
    utc = pytz.utc
    sao_paulo = pytz.timezone('America/Sao_Paulo')
    converted_timestamps = []
    for timestamp in timestamps:
        try:
            timestamp = timestamp.replace('T', ' ').replace('Z', '')
            converted_time = utc.localize(datetime.strptime(timestamp, '%Y-%m-%d %H:%M:%S.%f')).astimezone(sao_paulo)
        except ValueError:
            converted_time = utc.localize(datetime.strptime(timestamp, '%Y-%m-%d %H:%M:%S')).astimezone(sao_paulo)
        converted_timestamps.append(converted_time)
    return converted_timestamps

 
# Set lastN value
lastN = 10  # Get 10 most recent points at each interval
 
app = dash.Dash(__name__)
 
app.layout = html.Div([
    html.H1('Environment Data Viewer'),
    dcc.Graph(id='environment-graph'),
    # Store to hold historical data
    dcc.Store(id='environment-data-store', data={'timestamps': [], 'luminosity_values': [], 'temperature_values': [], 'humidity_values': []}),
    dcc.Interval(
        id='interval-component',
        interval=10*1000,  # in milliseconds (10 seconds)
        n_intervals=0
    )
])
 
@app.callback(
    Output('environment-data-store', 'data'),
    Input('interval-component', 'n_intervals'),
    State('environment-data-store', 'data')
)
def update_data_store(n, stored_data):
    # Get luminosity data
    data_luminosity = get_environment_data('luminosity', lastN)
    data_temperature = get_environment_data('temperature', lastN)
    data_humidity = get_environment_data('humidity', lastN)
 
    if data_luminosity and data_temperature and data_humidity:
        # Extract values and timestamps
        luminosity_values = [float(entry['attrValue']) for entry in data_luminosity]  # Ensure values are floats
        temperature_values = [float(entry['attrValue']) for entry in data_temperature]  # Ensure values are floats
        humidity_values = [float(entry['attrValue']) for entry in data_humidity]  # Ensure values are floats
        timestamps = [entry['recvTime'] for entry in data_luminosity]
 
        # Convert timestamps to Lisbon time
        timestamps = convert_to_sao_paulo_time(timestamps)
 
        # Append new data to stored data
        stored_data['timestamps'].extend(timestamps)
        stored_data['luminosity_values'].extend(luminosity_values)
        stored_data['temperature_values'].extend(temperature_values)
        stored_data['humidity_values'].extend(humidity_values)
 
        return stored_data
 
    return stored_data
 
@app.callback(
    Output('environment-graph', 'figure'),
    Input('environment-data-store', 'data')
)
def update_graph(stored_data):
    if stored_data['timestamps'] and stored_data['luminosity_values'] and stored_data['temperature_values'] and stored_data['humidity_values']:
        # Calculate mean luminosity
        mean_luminosity = sum(stored_data['luminosity_values']) / len(stored_data['luminosity_values'])
        mean_temperature = sum(stored_data['temperature_values']) / len(stored_data['temperature_values'])
        mean_humidity = sum(stored_data['humidity_values']) / len(stored_data['humidity_values'])
 
        # Create traces for the plot
        trace_luminosity = go.Scatter(
            x=stored_data['timestamps'],
            y=stored_data['luminosity_values'],
            mode='lines+markers',
            name='Luminosity',
            line=dict(color='orange')
        )

        trace_temperature = go.Scatter(
            x=stored_data['timestamps'],
            y=stored_data['temperature_values'],
            mode='lines+markers',
            name='Temperature',
            line=dict(color='red')
        )

        trace_humidity = go.Scatter(
            x=stored_data['timestamps'],
            y=stored_data['humidity_values'],
            mode='lines+markers',
            name='Humidity',
            line=dict(color='green')
        )

        trace_mean_luminosity = go.Scatter(
            x=[stored_data['timestamps'][0], stored_data['timestamps'][-1]],
            y=[mean_luminosity, mean_luminosity],
            mode='lines',
            name='Mean Luminosity',
            line=dict(color='blue', dash='dash')
        )

        trace_mean_luminosity = go.Scatter(
            x=[stored_data['timestamps'][0], stored_data['timestamps'][-1]],
            y=[mean_luminosity, mean_luminosity],
            mode='lines',
            name='Mean Luminosity',
            line=dict(color='blue', dash='dash')
        )

        trace_mean_temperature = go.Scatter(
            x=[stored_data['timestamps'][0], stored_data['timestamps'][-1]],
            y=[mean_temperature, mean_temperature],
            mode='lines',
            name='Mean Temperature',
            line=dict(color='purple', dash='dash')
        )

        trace_mean_humidity = go.Scatter(
            x=[stored_data['timestamps'][0], stored_data['timestamps'][-1]],
            y=[mean_humidity, mean_humidity],
            mode='lines',
            name='Mean Humidity',
            line=dict(color='cyan', dash='dash')
        )
 
        # Create figure
        fig_luminosity = go.Figure(data=[trace_luminosity, trace_temperature, trace_humidity, trace_mean_luminosity, trace_mean_temperature, trace_mean_humidity])
 
        # Update layout
        fig_luminosity.update_layout(
            title='Environmental Data Over Time',
            xaxis_title='Timestamp',
            yaxis_title='Environment Data',
            hovermode='closest'
        )
 
        return fig_luminosity
 
    return {}
 
if __name__ == '__main__':
    app.run_server(debug=True, host=DASH_HOST, port=8050)