#include "DHT.h" // Incluye la librería del sensor DHT

// Define el pin digital al que está conectado el sensor DHT11
#define DHTPIN 2 
// Define el tipo de sensor que estamos usando (DHT11, DHT22, etc.)
#define DHTTYPE DHT11 
//-----------------------------------------------------------------------------------//
#define CENTINELASENSORAGUA 100 
#define PINAGUAANALOG 5 
#define PINDIGITALFUEGO 4
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

void chillarZumbadorFuego(){

    for (int i = 0; i<10; ++i) {
    tone(PINZUMBADORDIGITAL, 1000); // Send 1KHz sound signal...
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
  pinMode(PINDIGITALFUEGO, INPUT);
  Serial.begin(115200); 
  Serial.println("Iniciando la lectura del sensor DHT11...");
  
  // Inicia el sensor DHT
  dht.begin();
}

void loop() {
  if (analogRead(PINAGUAANALOG) <= CENTINELASENSORAGUA ){
    chillarZumbadorInundacion();
  }

  if (digitalRead(PINDIGITALFUEGO) == 1){
    chillarZumbadorFuego();
  }

  delay(2000); // Recomendable para el sensor de fuego

  // --- LECTURA DEL SENSOR ---
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // --- ENVÍO DE DATOS POR EL PUERTO SERIE ---

  Serial.print("TEMP ");
  Serial.print(h);
  Serial.print(" ");
  Serial.println(t);
}