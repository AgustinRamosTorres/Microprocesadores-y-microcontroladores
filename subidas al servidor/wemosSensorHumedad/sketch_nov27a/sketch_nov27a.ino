  #include <vector>
  #include <WString.h>

  // --- CONFIGURACIÓN ---
  #define BAUDRATE 115200 
  // Aumentar el búfer RX al máximo posible para la memoria (2048 bytes)
  #define RX_BUFFER_SIZE 2048 

  // --- VARIABLES GLOBALES ---
  // Búfer temporal para acumular la línea que se está recibiendo actualmente
  String serialDataBuffer = ""; 

  // Cola de mensajes: Almacena las líneas completas pendientes de procesamiento
  std::vector<String> messageQueue; 


// --- FUNCIÓN DE PROCESAMIENTO ---
/**
 * @brief Función dedicada a procesar UN ÚNICO mensaje de la cola.
 * * Este es el único lugar donde se permite la lógica pesada.
 * * Debe ser llamado de forma controlada en loop().
 * @param line La línea de texto completa recibida (ej: "COMANDO:123").
 */
void processOneMessage(String line) {
  // *******************************************************************
  // ** EJEMPLO DE PROCESAMIENTO LENTO (SIMULACIÓN) **
  // *******************************************************************
  // SIMULAMOS UNA TAREA LENTA QUE PODRÍA OCUPAR EL CPU (¡PERO SOLO POR UNA VEZ!)
  // Si la línea empieza con "LENTO", esperamos 50 ms.
  
  Serial.print("✅ Procesando línea (");
  Serial.print(line.length());
  Serial.print(" bytes) desde la cola: ");
  Serial.println(line);

  // Aquí iría tu lógica real (controlar relés, actualizar variables, enviar a MQTT, etc.)
  // Por ejemplo:
  /*
  if (line.startsWith("SET_TEMP:")) {
    // ... tu código de control ...
  }
  */
}


// --- SETUP ---
void setup() {
  // 1. CRÍTICO: Aumentar el tamaño del búfer de recepción 
  Serial.setRxBufferSize(RX_BUFFER_SIZE); 
  
  // 2. Inicializar la comunicación serial
  Serial.begin(BAUDRATE); 
  
  Serial.println("\n--- ESP8266 Listos ---");
  Serial.println("Recepción de datos seriales en cola activa.");
}


// --- LOOP PRINCIPAL ---
void loop() {
  
  // ==========================================================
  // 1. FASE DE LECTURA RÁPIDA (Lectura No Bloqueante)
  // ESTA FASE ES PRIORITARIA Y DEBE SER MUY RÁPIDA.
  // ==========================================================
  
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

  // ==========================================================
  // 2. FASE DE PROCESAMIENTO DE COLA (Procesamiento Controlado)
  // Procesamos como MÁXIMO un solo mensaje por pasada del loop.
  // Esto permite que el loop regrese inmediatamente a la lectura serial.
  // ==========================================================
  
  if (!messageQueue.empty()) {
    // 1. Obtener el primer mensaje de la cola (el más antiguo)
    String lineToProcess = messageQueue.front(); 
    
    // 2. Ejecutar la función de procesamiento (puede ser lenta)
    processOneMessage(lineToProcess);
    
    // 3. Eliminar el mensaje de la cola una vez procesado
    messageQueue.erase(messageQueue.begin()); 
  }

  // ==========================================================
  // 3. OTRAS TAREAS DEL ESP8266
  // Aquí se ejecutan tareas regulares y no bloqueantes (ej: Servidor Web, MQTT, Sensores)
  // ==========================================================
  
  // Ejemplo: Lógica de red no bloqueante
  // WiFiClient.handleClient();
  // MqttClient.loop(); 
}