#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"

// --- Sensor DHT11 en GPIO2 (D2)
DHTesp dht;

// --- Configuración WiFi ---
const char* ssid = "iPhone de manue";
const char* password = "pitocaca";

// --- Configuración MQTT ---
const char* mqtt_server = "msanchez.ovh"; 
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP8266_Control_MSanchez"; 

const char* mqtt_user = "ardu";
const char* mqtt_password = "JMMAMicro";

// --- Tópicos ---
const char* TOPIC_DOOR_STATUS = "casa/puerta/estado";
const char* TOPIC_TEMP = "casa/sensor/temperatura";
const char* TOPIC_HUM = "casa/sensor/humedad";
const char* TOPIC_NFC_USER = "casa/nfc/usuario";

const char* TOPIC_ORDER_DOOR = "casa/puerta/orden"; 
const char* TOPIC_ORDER_ALARM = "casa/alarma/orden";
const char* TOPIC_ORDER_VENTILATION = "casa/ventilacion/orden";

const char* topic_subscriptions[] = {
    TOPIC_ORDER_DOOR,
    TOPIC_ORDER_ALARM,
    TOPIC_ORDER_VENTILATION
};
const int num_subscriptions = 3;

// MQTT
WiFiClient espClient;
PubSubClient client(espClient);

long lastPublish = 0;
int door_state = 0;

// ------------------------ CALLBACK MQTT ------------------------
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("ORDEN RECIBIDA en [");
  Serial.print(topic);
  Serial.print("]: ");

  String command;
  for (int i = 0; i < length; i++) command += (char)payload[i];
  Serial.println(command);

  // --- Puerta ---
  if (strcmp(topic, TOPIC_ORDER_DOOR) == 0) {
    if (command == "ABRIR") {
      door_state = 1;
      client.publish(TOPIC_DOOR_STATUS, "ABIERTA");
    } else if (command == "CERRAR") {
      door_state = 0;
      client.publish(TOPIC_DOOR_STATUS, "CERRADA");
    }
  }

  // --- Alarma ---
  else if (strcmp(topic, TOPIC_ORDER_ALARM) == 0) {
    Serial.println(command == "ON" ? "ALARMA ON" : "ALARMA OFF");
  }

  // --- Ventilación ---
  else if (strcmp(topic, TOPIC_ORDER_VENTILATION) == 0) {
    Serial.println(command == "ON" ? "VENTILACIÓN ON" : "VENTILACIÓN OFF");
  }
}

// ------------------------ RECONNECT MQTT ------------------------
void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
      Serial.println("conectado!");

      for (int i = 0; i < num_subscriptions; i++) {
        client.subscribe(topic_subscriptions[i]);
      }

      client.publish(TOPIC_DOOR_STATUS, door_state == 1 ? "ABIERTA" : "CERRADA");
    } else {
      Serial.print("falló, rc=");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

// ------------------------ WIFI ------------------------
void setup_wifi() {
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.println(WiFi.localIP());
}

// ------------------------ SETUP ------------------------
void setup() {
  Serial.begin(115200);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // ---- Inicializar DHT ----
  dht.setup(2, DHTesp::DHT11);  // GPIO2 = D2
  Serial.println("DHT11 inicializado");
}

// ------------------------ LOOP ------------------------
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  long now = millis();
  if (now - lastPublish > 10000) {
    lastPublish = now;

    TempAndHumidity data = dht.getTempAndHumidity();

    if (isnan(data.humidity) || isnan(data.temperature)) {
      Serial.println("Error leyendo DHT11");
      return;
    }

    char tempString[10];
    char humString[10];

    dtostrf(data.temperature, 4, 1, tempString);
    dtostrf(data.humidity, 4, 1, humString);

    client.publish(TOPIC_TEMP, tempString);
    client.publish(TOPIC_HUM, humString);

    Serial.print("Publicado -> T: ");
    Serial.print(tempString);
    Serial.print(" °C  |  H: ");
    Serial.println(humString);
  }
}
