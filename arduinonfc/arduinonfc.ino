#include <SoftwareSerial.h>
#include <PN532_SWHSU.h>
#include <PN532.h>
SoftwareSerial SWSerial( 3, 2 ); // RX, TX
 
PN532_SWHSU pn532swhsu( SWSerial );
PN532 nfc( pn532swhsu );
String tagId = "None", dispTag = "None";
byte nuidPICC[4];
 
void setup(void)
{
  Serial.begin(115200);
  
  //  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata)
  {
    while (1); // Halt
  }
  // Got valid data, print it out!
  // Configure board to read RFID tags
  nfc.SAMConfig();
  //Serial.println("Waiting for an ISO14443A Card ...");
}
 
 
// === AÑADE ESTO ARRIBA (deja tus includes como están) ===
// No necesitas librerías nuevas, usa Adafruit_PN532

// Prototipos
bool tryReadNtagText(String &outText);
bool tryReadMifareClassicBlockASCII(uint8_t blockNumber, String &outText);
void leerNombre();

// === REEMPLAZA loop() ===
void loop() {
  leerNombre();
}

// === IMPLEMENTACIÓN ===
void leerNombre() {
  uint8_t uid[7];
  uint8_t uidLength = 0;

  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    // sin tarjeta
    return;
  }

  // Intento 1: NTAG/Ultralight con NDEF (nombre en un Text Record)
  String nombre;
  if (tryReadNtagText(nombre)) {
    Serial.print("NFC ");
    Serial.println(nombre);
    delay(1000);
    return;
  }

  // Intento 2: MIFARE Classic, bloque 4 (ajusta si lo tienes en otro bloque)
  if (tryReadMifareClassicBlockASCII(4, nombre)) {
    Serial.print("NFC ");
    Serial.println(nombre);
    delay(1000);
    return;
  }

  delay(800);
}

// Lee NDEF Text Record en NTAG/Ultralight (Type 2)
bool tryReadNtagText(String &outText) {
  // En Type 2, los datos NDEF suelen empezar en la página 4 con TLV 0x03
  // Leemos varias páginas consecutivas y buscamos TLV 0x03 (NDEF)
  uint8_t data[4];
  const uint8_t startPage = 4;
  const uint8_t maxPages = 16; // lee 16 páginas (64 bytes), suficiente para un nombre
  uint8_t buffer[64];
  uint8_t idx = 0;

  for (uint8_t p = startPage; p < startPage + maxPages; p++) {
    if (!nfc.mifareultralight_ReadPage(p, data)) return false; // no es Ultralight/NTAG
    for (int i = 0; i < 4 && idx < sizeof(buffer); i++) buffer[idx++] = data[i];
  }

  // TLV parsing: 0x03 = NDEF, 0xFE = Terminator
  int i = 0;
  while (i < idx) {
    uint8_t tlv = buffer[i++];
    if (tlv == 0x00) continue;       // Null TLV
    if (tlv == 0xFE) break;          // Terminator
    if (i >= idx) break;

    uint32_t length = 0;
    if (tlv == 0x03) { // NDEF
      uint8_t lenByte = buffer[i++];
      if (lenByte == 0xFF) { // long form
        if (i + 1 >= idx) return false;
        length = (buffer[i] << 8) | buffer[i + 1];
        i += 2;
      } else {
        length = lenByte;
      }
      if (i + (int)length > idx) return false;

      // NDEF record
      uint8_t* rec = &buffer[i];
      i += length;

      // NDEF header
      if (length < 3) return false;
      uint8_t header = rec[0];
      bool sr = header & 0x10; // Short Record
      uint8_t typeLen = rec[1];
      int pos = 2;
      uint32_t payloadLen = 0;

      if (sr) {
        payloadLen = rec[pos++];
      } else {
        if (pos + 3 >= (int)length) return false;
        payloadLen = ((uint32_t)rec[pos] << 24) | ((uint32_t)rec[pos+1] << 16) | ((uint32_t)rec[pos+2] << 8) | rec[pos+3];
        pos += 4;
      }

      // (Opcional IL) ignorado si no está puesto
      // Tipo
      if (pos + typeLen > (int)length) return false;
      uint8_t* typeField = &rec[pos];
      pos += typeLen;

      // Comprobamos "T" (Text)
      if (!(typeLen == 1 && typeField[0] == 'T')) return false;

      // Payload
      if (pos + (int)payloadLen > (int)length) return false;
      uint8_t* payload = &rec[pos];

      // Primer byte del payload = status: bits 0..4 = len código idioma
      if (payloadLen < 1) return false;
      uint8_t status = payload[0];
      uint8_t langLen = status & 0x1F;
      if (1 + langLen > payloadLen) return false;

      // Texto
      outText = "";
      for (uint32_t k = 1 + langLen; k < payloadLen; k++) {
        char c = (char)payload[k];
        // filtra no imprimibles básicos
        if (c >= 32 && c <= 126) outText += c;
        else if (c == '\n' || c == '\r' || (uint8_t)c >= 160) outText += c; // permitir acentos
      }
      return outText.length() > 0;
    } else {
      // TLV distinto: saltar su longitud
      if (i >= idx) break;
      uint8_t len = buffer[i++];
      if (len == 0xFF) {
        if (i + 1 >= idx) break;
        uint16_t l = (buffer[i] << 8) | buffer[i + 1];
        i += 2 + l;
      } else {
        i += len;
      }
    }
  }
  return false;
}

// Lee un bloque de MIFARE Classic con clave por defecto y lo interpreta como ASCII
bool tryReadMifareClassicBlockASCII(uint8_t blockNumber, String &outText) {
  // OJO: necesitas el UID actual y su longitud para autenticar
  uint8_t uid[7]; uint8_t uidLength = 0;
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) return false;

  // clave A por defecto
  uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

  if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, blockNumber, 0, keya)) {
    return false; // no es Classic o la clave no es la por defecto
  }

  uint8_t data[16];
  if (!nfc.mifareclassic_ReadDataBlock(blockNumber, data)) return false;

  // Convierte bytes imprimibles a String (detente en 0x00 para "C-string" si quieres)
  outText = "";
  for (int i = 0; i < 16; i++) {
    uint8_t b = data[i];
    if (b == 0x00) break; // fin de cadena
    if (b >= 32 && b <= 126) outText += (char)b;
    else if (b == '\n' || b == '\r' || b >= 160) outText += (char)b;
  }
  return outText.length() > 0;
}