#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <vector>
#include <WString.h>

//-----------------------------------------------------------------------------//
// SERIAL HANDLING
#define BAUDRATE 115200 
#define RX_BUFFER_SIZE 2048 
String serialDataBuffer = ""; 
//-----------------------------------------------------------------------------//

// --- Configuración WiFi ---
const char* ssid = "amsiedad";
const char* password = "Patata123";

// --- Configuración MQTT ---
const char* mqtt_server   = "msanchez.ovh"; 
const int   mqtt_port     = 1883;
const char* mqtt_client_id = "ESP8266_NFC_MSanchez"; 
const char* mqtt_user     = "ardu";
const char* mqtt_password = "JMMAMicro";

// ------------------ TOPICS PUBLICACIÓN (Arduino → Web/MQTT) ------------------
const char* TOPIC_TEMP        = "sensor/temperatura";
const char* TOPIC_HUM         = "sensor/humedad";
const char* TOPIC_FIRE        = "sensor/fuego";
const char* TOPIC_WATER       = "sensor/agua";
const char* TOPIC_VENT_STATE  = "ventilacion/estado";
const char* TOPIC_DOOR_STATUS = "puerta/estado";
const char* TOPIC_NFC_USER    = "nfc/leido";

// ------------------ TOPICS ÓRDENES (Web → Arduino) ------------------
const char* TOPIC_ORDER_DOOR        = "puerta/orden";
const char* TOPIC_ORDER_ALARM       = "alarma/orden";
const char* TOPIC_ORDER_VENT        = "ventilacion/orden";
const char* TOPIC_ORDER_PERMISSIONS = "puerta/permisos";

// --- Lista suscripciones ---
const char* topic_subscriptions[] = {
  TOPIC_ORDER_DOOR,
  TOPIC_ORDER_ALARM,
  TOPIC_ORDER_VENT,
  TOPIC_ORDER_PERMISSIONS
};
const int num_subscriptions = 4;

WiFiClient espClient;
PubSubClient client(espClient);


// ---------------------------------------------------------------------------
//  PROCESAR MENSAJE ARDUINO → PUBLICAR MQTT
//  El Arduino enviará cosas como:
//  TEMP 24.5
//  HUM 60
//  FUEGO OK
//  AGUA MOJADO
//  PUERTA ABIERTA
//  NFC Manuel
//  VENT ENCENDIDO
// ---------------------------------------------------------------------------
void procesarMensajeArduino(String line) {
    line.trim();
    if (line.length() == 0) return;
    int spaceIndex = line.indexOf(' ');
    if (spaceIndex == -1) return;

    String key = line.substring(0, spaceIndex);
    String value = line.substring(spaceIndex + 1);

    key.trim();
    value.trim();

    Serial.print("Arduino → ");
    Serial.print(key);
    Serial.print(" = ");
    Serial.println(value);

    // --- ENRUTAR AL TOPIC MQTT CORRESPONDIENTE ---
    if (key == "TEMP")
        client.publish(TOPIC_TEMP, value.c_str());

    else if (key == "HUM")
        client.publish(TOPIC_HUM, value.c_str());

    else if (key == "FUEGO")
        client.publish(TOPIC_FIRE, value.c_str());

    else if (key == "AGUA")
        client.publish(TOPIC_WATER, value.c_str());

    else if (key == "VENT")
        client.publish(TOPIC_VENT_STATE, value.c_str());

    else if (key == "PUERTA")
        client.publish(TOPIC_DOOR_STATUS, value.c_str());

    else if (key == "NFC")
        client.publish(TOPIC_NFC_USER, value.c_str());
}

// ---------------------------------------------------------------------------
// LECTURA SERIAL DESDE ARDUINO LINEA A LINEA
// ---------------------------------------------------------------------------
void lecturaSerie() {
    while (Serial.available() > 0) {
        char incomingChar = Serial.read();

        if (incomingChar == '\n') {
            if (serialDataBuffer.length() > 0) {
                procesarMensajeArduino(serialDataBuffer);
            }
            serialDataBuffer = "";
        } 
        else if (incomingChar != '\r') {
            serialDataBuffer += incomingChar;
        }
    }
}


// ---------------------------------------------------------------------------
// MQTT CALLBACK — RECIBIR ORDEN Y ENVIÁRSELA AL ARDUINO
// ARDUINO RECIBIRÁ:
// CMD puerta/orden ABRIR
// CMD alarma/orden ON
// CMD ventilacion/orden OFF
// CMD puerta/permisos CORRECTO
//
// ---------------------------------------------------------------------------
void callback(char* topic, byte* payload, unsigned int length) {
    String command = "";

    for (int i = 0; i < length; i++)
        command += (char)payload[i];

    Serial.print("MQTT → ORDEN: ");
    Serial.print(topic);
    Serial.print(" => ");
    Serial.println(command);

    // reenviamos al Arduino
    Serial.print("CMD ");
    Serial.print(topic);
    Serial.print(" ");
    Serial.println(command);
}


// ---------------------------------------------------------------------------
// Reconexion MQTT
// ---------------------------------------------------------------------------
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando MQTT...");

    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
      Serial.println("Conectado!");

      for (int i = 0; i < num_subscriptions; i++) {
        client.subscribe(topic_subscriptions[i]);
        Serial.print("Suscrito: ");
        Serial.println(topic_subscriptions[i]);
      }

    } else {
      Serial.print("Falló rc=");
      Serial.print(client.state());
      Serial.println(" Reintentando...");
      delay(3000);
    }
  }
}


// ---------------------------------------------------------------------------
// WIFI
// ---------------------------------------------------------------------------
void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}


// ---------------------------------------------------------------------------
// SETUP
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.setRxBufferSize(RX_BUFFER_SIZE);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  Serial.println("\nWemos listo.");
}


// ---------------------------------------------------------------------------
// LOOP
// ---------------------------------------------------------------------------
void loop() {

  if (!client.connected())
    reconnect();

  client.loop();

  // Leer mensajes del Arduino
  lecturaSerie();
}
