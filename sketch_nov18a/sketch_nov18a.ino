#include "DHT.h"

// ----- Configuración del sensor -----
#define DHTPIN 7        // Pin digital donde está conectado el DHT11
#define DHTTYPE DHT11   // Tipo de sensor (DHT11 azul)

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando sensor DHT11...");
  
  dht.begin();

  Serial.println("Status\tHumedad (%)\tTemp (C)\tTemp (F)\tHeatIndex (C)\t(F)");
}

void loop() {
  // El DHT11 requiere al menos 1 segundo entre lecturas
  delay(2000);

  float humidity = dht.readHumidity();
  float temperatureC = dht.readTemperature();       // Celsius
  float temperatureF = dht.readTemperature(true);   // Fahrenheit

  // Comprobar si la lectura falló
  if (isnan(humidity) || isnan(temperatureC)) {
    Serial.println("Error leyendo del DHT11");
    return;
  }

  // Cálculo de heat index
  float heatIndexC = dht.computeHeatIndex(temperatureC, humidity, false);
  float heatIndexF = dht.computeHeatIndex(temperatureF, humidity, true);

  // Impresión
  Serial.print("OK\t");
  Serial.print(humidity);
  Serial.print("\t\t");
  Serial.print(temperatureC);
  Serial.print("\t\t");
  Serial.print(temperatureF);
  Serial.print("\t\t");
  Serial.print(heatIndexC);
  Serial.print("\t\t");
  Serial.println(heatIndexF);
}
