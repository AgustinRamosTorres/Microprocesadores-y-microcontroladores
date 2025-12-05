#include "DHT.h"  // Incluye la librería del sensor DHT
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
//-----------------------------------------------------------------------------------//
Servo servo;   // Crea un objeto servo
String bufferComando = "";
String p = "CERRADA"; 
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
void procesarComando(String linea) {
  linea.trim();
  if (!linea.startsWith("CMD ")) return;

  // Quitar "CMD "
  linea = linea.substring(4);

  // topic  value
  int sep = linea.indexOf(' ');
  if (sep == -1) return;

  String topic = linea.substring(0, sep);
  String value = linea.substring(sep + 1);


  // ---- PUERTA ----
  if (topic == "puerta/orden") {
    if (value == "ABRIR") abrir();
    else if (value == "CERRAR") cerar();
  }

  // ---- VENTILACION ----
  else if (topic == "ventilacion/orden") {
    if (value == "ON") encenderVentilador();
    else if (value == "OFF") apagarVentilador();
  }

  // ---- ALARMA ----
  else if (topic == "alarma/orden") {
    if (value == "ON") {
      Serial.println("ALARMA ACTIVADA");
      tone(PINZUMBADORDIGITAL, 1000);
    } else if (value == "OFF") {
      Serial.println("ALARMA DESACTIVADA");
      noTone(PINZUMBADORDIGITAL);
    }
  }

  // ---- PERMISOS ----
  else if (topic == "puerta/permisos") {
    Serial.print("Permisos: ");
    Serial.println(value);
  }
}

// Leer línea completa proveniente del Wemos
void leerComandosWemos() {
  while (Serial.available()) {
    char c = Serial.read();

    // Construimos la línea SOLO si empieza por CMD
    if (bufferComando.length() == 0) {
      // Aún no sabemos lo que es → solo aceptamos si empieza por 'C'
      if (c != 'C') continue;  
    }

    bufferComando += c;

    // Si llegó fin de línea, procesar
    if (c == '\n') {
      if (bufferComando.startsWith("CMD ")) {
        procesarComando(bufferComando);
      }
      // Limpiar siempre
      bufferComando = "";
    }
  }
}

void chillarZumbadorInundacion() {

  for (int i = 0; i < 20; ++i) {
    tone(PINZUMBADORDIGITAL, 500);  // Send 1KHz sound signal...
    setColor(0, 0, 255);
    delay(100);                  // ...for 1 sec
    noTone(PINZUMBADORDIGITAL);  // Stop sound...
    setColor(255, 0, 0);
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
    setColor(255, 0, 0);
    delay(100);                  // ...for 1 sec
    noTone(PINZUMBADORDIGITAL);  // Stop sound...
    setColor(0, 0, 0);
    delay(100);  // ...for 1sec
  }
  setColor(255, 0, 0);
}

void setColor(int red, int green, int blue) {
  analogWrite(PINROJO, red);
  analogWrite(PINVERDE, green);
  analogWrite(PINAZUL, blue);
}

void abrir(){
  p = "ABIERTA";
  
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

    // Limpia la pantalla
  display.clearDisplay();
  display.display();

  display.print("ADELANTE");

  display.display();

  for (int angulo = 90; angulo <= 180; angulo++) {
    servo.write(angulo);
    delay(15);  // Ajusta la velocidad del movimiento
  }
}

void cerar(){
    p = "CERRADA";

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

    // Limpia la pantalla
  display.clearDisplay();
  display.display();

  display.print("CERRADO");

  display.display();
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
  cerar();

  //-----------------------------------------------------------------------------------//
}

void loop() {

  // -------------------- NUEVO ------------------------
  leerComandosWemos();   // ← lee CMD del Wemos continuamente
  // ----------------------------------------------------

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

  delay(2000);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  pintaTemperatura(h, t);
  Serial.print ("PUERTA ");
  Serial.println(p);
  Serial.print("TEMP ");
  Serial.println(t);
  Serial.print("HUM ");
  Serial.println(h);
}