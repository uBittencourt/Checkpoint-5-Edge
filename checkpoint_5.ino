// Adicionando bibliotecas ao projeto
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHTPIN 4          // Pino onde o DHT22 está conectado
#define DHTTYPE DHT22     // Tipo do sensor

DHT dht(DHTPIN, DHTTYPE); // Instância do sensor DHT

// Configurações - variáveis editáveis
const char* default_SSID = "Wokwi-GUEST";                       // Nome da rede Wi-Fi
const char* default_PASSWORD = "";                              // Senha da rede Wi-Fi
const char* default_BROKER_MQTT = "54.235.126.90";              // IP do Broker MQTT
const int default_BROKER_PORT = 1883;                           // Porta do Broker MQTT
const char* default_TOPICO_SUBSCRIBE = "/TEF/lamp2005/cmd";     // Tópico MQTT de escuta
const char* default_TOPICO_PUBLISH_1 = "/TEF/lamp2005/attrs";   // Tópico MQTT de envio de informações para Broker
const char* default_TOPICO_PUBLISH_2 = "/TEF/lamp2005/attrs/l"; // Tópico MQTT de envio de informações para Broker
const char* default_TOPICO_PUBLISH_3 = "/TEF/lamp2005/attrs/t"; // Tópico MQTT de envio de informações para Broker
const char* default_TOPICO_PUBLISH_4 = "/TEF/lamp2005/attrs/h"; // Tópico MQTT de envio de informações para Broker
const char* default_ID_MQTT = "fiware_2005";                    // ID MQTT
const int default_D4 = 2;                                       // Pino do LED onboard

// Declaração da variável para o prefixo do tópico
const char* topicPrefix = "lamp2005";

// Variáveis para configurações editáveis
char* SSID = const_cast<char*>(default_SSID);
char* PASSWORD = const_cast<char*>(default_PASSWORD);
char* BROKER_MQTT = const_cast<char*>(default_BROKER_MQTT);
int BROKER_PORT = default_BROKER_PORT;
char* TOPICO_SUBSCRIBE = const_cast<char*>(default_TOPICO_SUBSCRIBE);
char* TOPICO_PUBLISH_1 = const_cast<char*>(default_TOPICO_PUBLISH_1);
char* TOPICO_PUBLISH_2 = const_cast<char*>(default_TOPICO_PUBLISH_2);
char* TOPICO_PUBLISH_3 = const_cast<char*>(default_TOPICO_PUBLISH_3);
char* TOPICO_PUBLISH_4 = const_cast<char*>(default_TOPICO_PUBLISH_4);
char* ID_MQTT = const_cast<char*>(default_ID_MQTT);
int D4 = default_D4;

// Objetos para Wi-Fi e MQTT
WiFiClient espClient;
PubSubClient MQTT(espClient);

// Estado do LED (0: desligado, 1: ligado)
char EstadoSaida = '0';

// Inicializa a comunicação serial
void initSerial() {
    Serial.begin(115200);
}

// Inicializa a conexão Wi-Fi
void initWiFi() {
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    reconectWiFi();
}

// Inicializa o cliente MQTT
void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);
    MQTT.setCallback(mqtt_callback);
}

// Configurações iniciais
void setup() {
    InitOutput();
    initSerial();
    initWiFi();
    initMQTT();
    delay(5000);
    dht.begin();
    MQTT.publish(TOPICO_PUBLISH_1, "s|on");
}

// Loop principal
void loop() {
    VerificaConexoesWiFIEMQTT();
    EnviaEstadoOutputMQTT();
    handleLuminosity();
    handleAmbience();
    MQTT.loop();
}

// Reconecta ao Wi-Fi se não estiver conectado
void reconectWiFi() {
    if (WiFi.status() == WL_CONNECTED)
        return;
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());

    // Garantir que o LED inicie desligado
    digitalWrite(D4, LOW);
}

// Função de callback para receber mensagens MQTT
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) {
        char c = (char)payload[i];
        msg += c;
    }
    Serial.print("- Mensagem recebida: ");
    Serial.println(msg);

    // Forma o padrão de tópico para comparação
    String onTopic = String(topicPrefix) + "@on|";
    String offTopic = String(topicPrefix) + "@off|";

    // Compara com o tópico recebido
    if (msg.equals(onTopic)) {
        digitalWrite(D4, HIGH);
        EstadoSaida = '1';
    }

    if (msg.equals(offTopic)) {
        digitalWrite(D4, LOW);
        EstadoSaida = '0';
    }
}

// Verifica as conexões Wi-Fi e MQTT
void VerificaConexoesWiFIEMQTT() {
    if (!MQTT.connected())
        reconnectMQTT();
    reconectWiFi();
}

// Envia o estado do LED via MQTT
void EnviaEstadoOutputMQTT() {
    if (EstadoSaida == '1') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|on");
        Serial.println("- Led Ligado");
    }

    if (EstadoSaida == '0') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|off");
        Serial.println("- Led Desligado");
    }
    Serial.println("- Estado do LED onboard enviado ao broker!");
    delay(1000);
}

// Inicializa o pino do LED 
void InitOutput() {
    pinMode(D4, OUTPUT);
    digitalWrite(D4, HIGH);
    boolean toggle = false;

    for (int i = 0; i <= 10; i++) {
        toggle = !toggle;
        digitalWrite(D4, toggle);
        delay(200);
    }
}

// Tenta reconectar ao Broker MQTT
void reconnectMQTT() {
    while (!MQTT.connected()) {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE);
        } else {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Haverá nova tentativa de conexão em 2s");
            delay(2000);
        }
    }
}

// Faz leitura da luminosidade e publica no tópico apropriado
void handleLuminosity() {
    // Faz leitura e mapeia valores
    const int potPin = 34;
    int sensorValue = analogRead(potPin);
    int luminosity = map(sensorValue, 0, 4095, 0, 100);

    // Converte, imprime e publica os valores de luminosidade
    String mensagem = String(luminosity);
    Serial.print("Valor da luminosidade: ");
    Serial.println(mensagem.c_str());
    MQTT.publish(TOPICO_PUBLISH_2, mensagem.c_str());
}

// Faz leitura da temperatura e umidade e publica nos tópicos apropriados
void handleAmbience() {
  // Realiza a leitura
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Verifica se a leitura falhou
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Falha ao ler do DHT sensor!");
    return;
  }
  
  // Converte os valores para String
  String mensagem_humidity = String(humidity);
  String mensagem_temperature = String(temperature);
  
  // Imprime e publica os valores de umidade
  Serial.print("Umidade: ");
  Serial.print(mensagem_humidity.c_str());
  Serial.println(" %");
  MQTT.publish(TOPICO_PUBLISH_4, mensagem_humidity.c_str());

  // Imprime e publica os valores de temperatura
  Serial.print("Temperatura: ");
  Serial.print(mensagem_temperature.c_str());
  Serial.println(" °C");
  MQTT.publish(TOPICO_PUBLISH_3, mensagem_temperature.c_str());
}