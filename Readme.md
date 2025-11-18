# Sensor LDR
## Foto
![alt text](image.png)

---
## Código de ejemplo
````ino
int PinLDR = A5;
int luminosidad=0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  luminosidad=analogRead(A0);
  

  if(luminosidad<300) 
  {
    Serial.println("hay menos luz");
    Serial.println((String)"Temperatura actual:\t" + (String)luminosidad);
  }
  else//modo noche
  {
    Serial.println("hay más luz");
    Serial.println((String)"Temperatura actual:\t" + (String)luminosidad);
    }
  
}
````