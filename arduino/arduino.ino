#include "DHT.h"  // Incluye la librería del sensor DHT
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
//-----------------------------------------------------------------------------------//
Servo servo;   // Crea un objeto servo
//-----------------------------------------------------------------------------------//
// Inicializa el display I2C de 128x32
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);
//-----------------------------------------------------------------------------------//

// Define el pin digital al que está conectado el sensor DHT11
#define DHTPIN 2
// Define el tipo de sensor que estamos usando (DHT11, DHT22, etc.)
#define DHTTYPE DHT11
//-----------------------------------------------------------------------------------//
#define CENTINELASENSORAGUA 100
#define PINAGUAANALOG 0
#define PINDIGITALFUEGO 4
#define PINZUMBADORDIGITAL 3
#define PINDIGITALVENTILADOR 6
#define PINDIGITALSERVO 5
#define PINROJO 9
#define PINVERDE 10
#define PINAZUL 11

//-----------------------------------------------------------------------------------//
// Inicializa el sensor DHT
DHT dht(DHTPIN, DHTTYPE);

//-----------------------------------------------------------------------------------//
void chillarZumbadorInundacion() {

  for (int i = 0; i < 20; ++i) {
    tone(PINZUMBADORDIGITAL, 500);  // Send 1KHz sound signal...
    setColor(0, 255, 0);
    delay(100);                  // ...for 1 sec
    noTone(PINZUMBADORDIGITAL);  // Stop sound...
    setColor(0, 0, 255);
    delay(100);  // ...for 1sec
  }
  setColor(0, 0, 255);
}

void encenderVentilador(){
  Serial.println("VENT ENCENDIDO");
  analogWrite(PINDIGITALVENTILADOR,255);
}

void apagarVentilador(){
  Serial.println("VENT APAGADO");
  analogWrite(PINDIGITALVENTILADOR,0);
}

void pintaTemperatura(float humedad, float temperatura) {
  // Configura la pantalla
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // Limpia la pantalla
  display.clearDisplay();
  display.display();

  display.print("Temperatura: ");
  display.println(temperatura);
  display.print("Humedad: ");
  display.println(humedad);

  // Muestra todo en el OLED
  display.display();
}

void chillarZumbadorFuego() {

  for (int i = 0; i < 10; ++i) {
    tone(PINZUMBADORDIGITAL, 1000);  // Send 1KHz sound signal...
    setColor(0, 255, 0);
    delay(100);                  // ...for 1 sec
    noTone(PINZUMBADORDIGITAL);  // Stop sound...
    setColor(0, 0, 0);
    delay(100);  // ...for 1sec
  }
  setColor(0, 255, 0);
}

void setColor(int red, int green, int blue) {
  analogWrite(PINROJO, red);
  analogWrite(PINVERDE, green);
  analogWrite(PINAZUL, blue);
}

void abrir(){
  Serial.println("PUERTA ABIERTA");
  for (int angulo = 90; angulo <= 180; angulo++) {
    servo.write(angulo);
    delay(15);  // Ajusta la velocidad del movimiento
  }
}

void cerar(){
    Serial.println("PUERTA CERRADA");
    for (int angulo = 180; angulo >= 90; angulo--) {
    servo.write(angulo);
    delay(15);  // Ajusta la velocidad del movimiento
  }
}
//-----------------------------------------------------------------------------------//

void setup() {
  // Inicializa la comunicación serial a 9600 baudios
  // (0, 1) se refiere a los pines RX y TX que utiliza internamente
  // la placa Arduino para el puerto serie USB.
  pinMode(PINZUMBADORDIGITAL, OUTPUT);
  pinMode(PINDIGITALFUEGO, INPUT);
  pinMode(PINROJO, OUTPUT);
  pinMode(PINVERDE, OUTPUT);
  pinMode(PINAZUL, OUTPUT);
  pinMode(PINDIGITALVENTILADOR, OUTPUT);

  Serial.begin(115200);
  Serial.println("Iniciando la lectura del sensor DHT11...");

  // Inicia el sensor DHT
  dht.begin();
  setColor(255, 255, 255);
  
  //-----------------------------------------------------------------------------------//
  servo.attach(PINDIGITALSERVO);  // Conecta la señal del servo al pin 9
  //-----------------------------------------------------------------------------------//

  // Inicializa el display con voltaje interno, dirección 0x3C
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true);  // Detener si falla
  }

  // Muestra el splash inicial de Adafruit
  display.display();
  delay(1000);

  // Limpia la pantalla
  display.clearDisplay();
  display.display();

  // Configura texto
  pintaTemperatura(0, 0);


  //-----------------------------------------------------------------------------------//
}

void loop() {
  if (analogRead(PINAGUAANALOG) >= CENTINELASENSORAGUA) {
    Serial.println("AGUA MOJADO");
    chillarZumbadorInundacion();
    if (digitalRead(PINDIGITALFUEGO) == 1) {
      Serial.println("FUEGO FUEGO");
      chillarZumbadorFuego();
    } else {
      setColor(255, 255, 255);
      Serial.println("FUEGO OK");
    }
  } else {
    Serial.println("AGUA OK");
    setColor(255, 255, 255);
    if (digitalRead(PINDIGITALFUEGO) == 1) {
      Serial.println("FUEGO FUEGO");
      chillarZumbadorFuego();
    } else {
      setColor(255, 255, 255);
      Serial.println("FUEGO OK");
    }
  }

  //abrir();
  //encenderVentilador();
  delay(2000);  // Recomendable para el sensor de fuego
  //apagarVentilador();
  //cerar();

  // --- LECTURA DEL SENSOR ---
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // --- ENVÍO DE DATOS POR EL PUERTO SERIE ---
  pintaTemperatura(h, t);

  Serial.print("TEMP ");
  Serial.println(t);
  Serial.print("HUM ");
  Serial.println(h);
}