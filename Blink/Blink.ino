
// Basicamente con 255 es encendido pero con el 0 es apagado.
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(3, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  analogWrite(3,255);
  delay(1000);                      // wait for a second
  analogWrite(3,0);  
  delay(5000);                      // wait for a second
}
