#include "DHT.h" // Incluye la librería del sensor DHT

// Define el pin digital al que está conectado el sensor DHT11
#define DHTPIN 2 
// Define el tipo de sensor que estamos usando (DHT11, DHT22, etc.)
#define DHTTYPE DHT11 
//-----------------------------------------------------------------------------------//
#define CENTINELASENSORAGUA 400 
#define PINAGUAANALOG 5 
#define PINZUMBADORDIGITAL 3 

//-----------------------------------------------------------------------------------//
// Inicializa el sensor DHT
DHT dht(DHTPIN, DHTTYPE);

//-----------------------------------------------------------------------------------//
void chillarZumbadorInundacion(){

  for (int i = 0; i<20; ++i) {
    tone(PINZUMBADORDIGITAL, 500); // Send 1KHz sound signal...
    delay(100);         // ...for 1 sec
    noTone(PINZUMBADORDIGITAL);     // Stop sound...
    delay(100);         // ...for 1sec
  }

}
//-----------------------------------------------------------------------------------//

void setup() {
  // Inicializa la comunicación serial a 9600 baudios
  // (0, 1) se refiere a los pines RX y TX que utiliza internamente 
  // la placa Arduino para el puerto serie USB.
  pinMode(PINZUMBADORDIGITAL, OUTPUT); 
  Serial.begin(115200); 
  Serial.println("Iniciando la lectura del sensor DHT11...");
  
  // Inicia el sensor DHT
  dht.begin();
}

void loop() {
  // El DHT11 necesita un momento para tomar una lectura.
  // Se recomienda esperar al menos 2 segundos entre lecturas.
  if (analogRead(PINAGUAANALOG) <= CENTINELASENSORAGUA ){
    chillarZumbadorInundacion();
  }

  delay(2000);

  // --- LECTURA DEL SENSOR ---

  // Leer la humedad (H) en porcentaje.
  float h = dht.readHumidity();
  // Leer la temperatura (T) en grados Celsius.
  float t = dht.readTemperature();

  // Comprobar si ha habido algún error en la lectura (por ejemplo, sensor no conectado)
//  if (isnan(h) || isnan(t)) {
//    Serial.println("¡Error al leer los datos del sensor DHT11!");
//    return;
//  }

  // --- ENVÍO DE DATOS POR EL PUERTO SERIE ---

  // Muestra los datos de humedad y temperatura en el Monitor Serie
  Serial.print("TEMP ");
  Serial.print(h);
  Serial.print(" ");
  Serial.println(t);
}