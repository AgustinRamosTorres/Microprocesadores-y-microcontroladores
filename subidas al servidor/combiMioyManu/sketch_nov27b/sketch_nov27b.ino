#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <vector>

#include <WString.h>



//-----------------------------------------------------------------------------//
#define BAUDRATE 115200 
#define RX_BUFFER_SIZE 2048 
String serialDataBuffer = ""; 
std::vector<String> messageQueue; 
//-----------------------------------------------------------------------------//






// --- Configuración WiFi (Mantener la tuya) ---
const char* ssid = "CASA";
const char* password = "FAMILIARTAGUSTIN";

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

//-----------------------------------------------------------------------------//
void lecturaSerie(){

  while (Serial.available() > 0) {
    char incomingChar = Serial.read();
    
    // Si detectamos el salto de línea ('\n'), el mensaje está completo.
    if (incomingChar == '\n') {
      
      if (serialDataBuffer.length() > 0) {
        // ENCOLAR: Añadir la línea completa al final de la cola
        messageQueue.push_back(serialDataBuffer); 
        
        // Opcional: Mostrar que se encoló (solo para debug)
        Serial.print(">>> Línea ENCOLADA: ");
        Serial.println(serialDataBuffer);
      }
      
      // Limpiar el búfer para la siguiente línea
      serialDataBuffer = ""; 
    } 
    // Ignoramos el retorno de carro '\r' si viene con '\n' (CRLF)
    else if (incomingChar != '\r') { 
      // Añadir el carácter al búfer temporal
      serialDataBuffer += incomingChar;
    }
  }

}
//-----------------------------------------------------------------------------//


//-----------------------------------------------------------------------------//
std::vector<String> procesarlinea(String line){

  std::vector<String> tokens;
  
  // 1. Eliminar espacios iniciales y finales
  line.trim(); 
  
  int currentPos = 0;
  int spacePos = -1;

  while (currentPos < line.length()) {
    
    // 2. Buscar la posición del siguiente espacio
    spacePos = line.indexOf(' ', currentPos); 

    // 3. Extraer el token
    String token;
    
    if (spacePos == -1) {
      // Si no hay más espacios, el resto es el ÚLTIMO token
      token = line.substring(currentPos);
      currentPos = line.length(); // Mover al final para terminar el bucle
    } else {
      // Si hay espacio, extrae la subcadena antes del espacio
      token = line.substring(currentPos, spacePos);
      // Avanzar la posición actual justo después del espacio encontrado
      currentPos = spacePos + 1; 
    }
    
    // 4. CRÍTICO: Asegurarse de que el token no esté vacío
    // Esto previene que los múltiples espacios (ej: "a  b") generen un token vacío
    if (token.length() > 0) {
      tokens.push_back(token);
    }

  }

  return tokens;

}
//-----------------------------------------------------------------------------//


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

//-----------------------------------------------------------------------------//
   // 1. CRÍTICO: Aumentar el tamaño del búfer de recepción 
  Serial.setRxBufferSize(RX_BUFFER_SIZE); 

  // Tiene que tener el serial a 115200 que tiene que ser lo mismo que en el arduino
  Serial.println("\n--- ESP8266 Listos ---");
  Serial.println("Recepción de datos seriales en cola activa.");
  //-----------------------------------------------------------------------------//
}

// --- Loop ---
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  
  // Mantiene la conexión y procesa los mensajes entrantes (órdenes de la web)
  client.loop();

//-----------------------------------------------------------------------------//
  lecturaSerie();
//-----------------------------------------------------------------------------//


  // Publicación periódica de datos (Estado y Sensores)
  long now = millis();
  if (now - lastPublish > 10000) { // Publicar cada 10 segundos (para no saturar)
    lastPublish = now;
    
    // --- SIMULACIÓN DE LECTURA Y PUBLICACIÓN DE DATOS ---
    

//----------------------------------------------------------------------------//
    std::vector<String> palabras;
    if (!messageQueue.empty()) {
      // 1. Obtener el primer mensaje de la cola (el más antiguo)
      String lineToProcess = messageQueue.front(); 

      // 2. Ejecutar la función de procesamiento (puede ser lenta)
      palabras = procesarlinea(lineToProcess);

      temperature = palabras[2].toFloat(); // Cambiar y filtrar cuando nos llegue más de una cosa por la serie
      humidity = palabras[1].toFloat() ;
      
      Serial.println ("hola");
      Serial.println (temperature);
      Serial.println (humidity);
      Serial.println ("adios");
      
      // 3. Eliminar el mensaje de la cola una vez procesado
      messageQueue.erase(messageQueue.begin()); 
    }
//-----------------------------------------------------------------------------//


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