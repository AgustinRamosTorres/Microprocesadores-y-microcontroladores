#include <ESP8266WiFi.h>
#include <PubSubClient.h> 

// --- Configuración WiFi (Mantener la tuya) ---
const char* ssid = "iPhone de manue";
const char* password = "pitocaca";

// --- Configuración MQTT ---
// NOTA: Para conectar el ESP8266 a un BROKER remoto/privado como msanchez.ovh, 
// el broker DEBE estar expuesto en un puerto TCP estándar (ej. 1883), 
// ya que el ESP8266 (por defecto) no usa WebSockets ni SSL/TLS fácilmente.
// Usaré un broker de prueba por si el tuyo es privado.
const char* mqtt_server = "msanchez.ovh"; 
const int mqtt_port = 1883; // Puerto MQTT estándar (TCP)
const char* mqtt_client_id = "ESP8266_Control_MSanchez"; 

// Usa tus credenciales de acceso, si las tienes.
const char* mqtt_user = "ardu";
const char* mqtt_password = "JMMAMicro";

//const char* mqtt_user = ""; 
//const char* mqtt_password = ""; 

// --- TÓPICOS DE LA WEB (La web escucha y el ESP publica) ---
const char* TOPIC_DOOR_STATUS = "casa/puerta/estado";
const char* TOPIC_TEMP = "casa/sensor/temperatura";
const char* TOPIC_HUM = "casa/sensor/humedad";
const char* TOPIC_NFC_USER = "casa/nfc/usuario";

// --- TÓPICOS DE ORDEN (El ESP escucha y la web publica) ---
// La web envía comandos a estos tópicos.
const char* TOPIC_ORDER_DOOR = "casa/puerta/orden"; 
const char* TOPIC_ORDER_ALARM = "casa/alarma/orden";
const char* TOPIC_ORDER_VENTILATION = "casa/ventilacion/orden";

// Array de tópicos a los que el ESP8266 debe SUSCRIBIRSE para recibir órdenes:
const char* topic_subscriptions[] = {
    TOPIC_ORDER_DOOR,
    TOPIC_ORDER_ALARM,
    TOPIC_ORDER_VENTILATION
};
const int num_subscriptions = 3;

// Objetos
WiFiClient espClient;
PubSubClient client(espClient);

long lastPublish = 0;
// Variables de estado y sensor (simuladas)
int door_state = 0; // 0=CERRADO, 1=ABIERTO
float temperature = 25.5;
float humidity = 60.0;


// --- Funciones ---

/**
 * @brief Se ejecuta cuando se recibe un mensaje MQTT (una orden de la web).
 */
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("ORDEN RECIBIDA en [");
  Serial.print(topic);
  Serial.print("]: ");

  String command = "";
  for (int i = 0; i < length; i++) {
    command += (char)payload[i];
  }
  Serial.println(command);

  // --- Lógica de Control de la Puerta ---
  if (strcmp(topic, TOPIC_ORDER_DOOR) == 0) {
    if (command == "ABRIR") {
      Serial.println("-> Ejecutando orden: ABRIR PUERTA");
      door_state = 1;
      // Aquí iría el código para activar el relé/solenoide
      client.publish(TOPIC_DOOR_STATUS, "ABIERTA"); // Confirma el nuevo estado a la web
    } else if (command == "CERRAR") {
      Serial.println("-> Ejecutando orden: CERRAR PUERTA");
      door_state = 0;
      // Aquí iría el código para desactivar el relé/solenoide
      client.publish(TOPIC_DOOR_STATUS, "CERRADA"); // Confirma el nuevo estado a la web
    }
  }

  // --- Lógica de Control de Alarma ---
  else if (strcmp(topic, TOPIC_ORDER_ALARM) == 0) {
    if (command == "ON") {
      Serial.println("-> Ejecutando orden: ALARMA ON (Zumbador)");
      // Código para activar el zumbador
    } else if (command == "OFF") {
      Serial.println("-> Ejecutando orden: ALARMA OFF");
      // Código para desactivar el zumbador
    }
  }

  // --- Lógica de Control de Ventilación ---
  else if (strcmp(topic, TOPIC_ORDER_VENTILATION) == 0) {
    if (command == "ON") {
      Serial.println("-> Ejecutando orden: VENTILACIÓN ON");
      // Código para encender ventilación
    } else if (command == "OFF") {
      Serial.println("-> Ejecutando orden: VENTILACIÓN OFF");
      // Código para apagar ventilación
    }
  }
}

/**
 * @brief Intenta reconectar el cliente al broker MQTT.
 */
void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
      Serial.println("conectado!");
      
      // Suscribirse a los tópicos de órdenes
      for (int i = 0; i < num_subscriptions; i++) {
        client.subscribe(topic_subscriptions[i]);
        Serial.print("Suscrito a: ");
        Serial.println(topic_subscriptions[i]);
      }
      
      // Publicar estados iniciales
      client.publish(TOPIC_DOOR_STATUS, door_state == 1 ? "ABIERTA" : "CERRADA");
      
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(". Reintentando en 5 segundos...");
      delay(5000);
    }
  }
}

/**
 * @brief Configura la conexión WiFi.
 */
void setup_wifi() {
  // (La función setup_wifi es la misma que ya tenías)
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// --- Setup ---
void setup() {
  Serial.begin(115200);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); 
}

// --- Loop ---
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  
  // Mantiene la conexión y procesa los mensajes entrantes (órdenes de la web)
  client.loop();

  // Publicación periódica de datos (Estado y Sensores)
  long now = millis();
  if (now - lastPublish > 10000) { // Publicar cada 10 segundos (para no saturar)
    lastPublish = now;
    
    // --- SIMULACIÓN DE LECTURA Y PUBLICACIÓN DE DATOS ---
    
    // Simular nueva lectura de sensor (En un proyecto real, se leería el DHT11/22 aquí)
    temperature += 0.1;
    if (temperature > 30.0) temperature = 25.0; 
    humidity -= 0.2;
    if (humidity < 50.0) humidity = 60.0;

    char tempString[8];
    char humString[8];
    
    // Convierte el float a String (con 1 decimal)
    dtostrf(temperature, 4, 1, tempString);
    dtostrf(humidity, 4, 1, humString);

    // 1. Publicar Temperatura
    client.publish(TOPIC_TEMP, tempString);
    
    // 2. Publicar Humedad
    client.publish(TOPIC_HUM, humString);

    // 3. Simulación de Acceso NFC (Esto se haría al pasar una tarjeta)
    // client.publish(TOPIC_NFC_USER, "MSanchez - Acceso OK");
    
    Serial.print("Datos publicados: T=");
    Serial.print(tempString);
    Serial.print(", H=");
    Serial.println(humString);
  }
}