#include <SoftwareSerial.h>
#include <PN532_SWHSU.h>
#include <PN532.h>

// Pines SoftwareSerial para el PN532 (ajusta si los usas diferentes)
SoftwareSerial SWSerial(3, 2); // RX, TX

PN532_SWHSU pn532swhsu(SWSerial);
PN532 nfc(pn532swhsu);

// Prototipos
bool tryReadNtagText(String &outText);
bool tryReadMifareClassicBlockASCII(uint8_t *uid, uint8_t uidLength, uint8_t blockNumber, String &outText);
void leerNombre();

void setup(void)
{
  Serial.begin(115200);

  // MUY IMPORTANTE: inicializar también el puerto SoftwareSerial
  SWSerial.begin(115200);   // si va inestable, prueba con 9600

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata)
  {
    Serial.println("No se encontró PN532 :(");
    while (1); // Halt
  }

  Serial.println("PN532 detectado.");
  nfc.SAMConfig();   // Configurar PN532 para leer tarjetas ISO14443A

  Serial.println("Esperando tarjetas NFC...");
}

void loop()
{
  leerNombre();
  // pequeña pausa para no bombardear el PN532
  delay(50);
}

// Lee una tarjeta y saca por Serial: "NFC <nombre>"
void leerNombre()
{
  uint8_t uid[7];
  uint8_t uidLength = 0;

  // Intentar detectar una tarjeta
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength))
  {
    // No hay tarjeta, salir
    return;
  }

  // Tenemos una tarjeta delante
  String nombre;
  nombre.reserve(64);  // reducir fragmentación de memoria

  // 1º intento: NTAG / Ultralight con registro NDEF de texto
  if (tryReadNtagText(nombre))
  {
    Serial.print("NFC ");
    Serial.println(nombre);
    delay(1000);  // pequeña espera para no repetir la misma tarjeta mil veces
    return;
  }

  // 2º intento: MIFARE Classic, bloque 4 (ajusta si tu nombre está en otro bloque)
  if (tryReadMifareClassicBlockASCII(uid, uidLength, 4, nombre))
  {
    Serial.print("NFC ");
    Serial.println(nombre);
    delay(1000);
    return;
  }

  // Si no hemos podido leer nada útil, esperamos un poco
  delay(200);
}

// ---------------------------------------------------------------------------
// Lee NDEF Text Record en NTAG/Ultralight (Type 2)
// Busca un TLV 0x03 con un registro NDEF de tipo "T" (texto)
// ---------------------------------------------------------------------------
bool tryReadNtagText(String &outText)
{
  // En Type 2, los datos NDEF suelen empezar en la página 4 con TLV 0x03
  uint8_t data[4];
  const uint8_t startPage = 4;
  const uint8_t maxPages = 16; // 16 páginas (64 bytes) -> suficiente para un nombre corto
  uint8_t buffer[64];
  uint8_t idx = 0;

  // Leer varias páginas consecutivas
  for (uint8_t p = startPage; p < startPage + maxPages; p++)
  {
    if (!nfc.mifareultralight_ReadPage(p, data))
    {
      // Si falla, probablemente no es Ultralight/NTAG o no hay datos
      return false;
    }
    for (int i = 0; i < 4 && idx < sizeof(buffer); i++)
    {
      buffer[idx++] = data[i];
    }
  }

  // Parseo TLV: 0x03 = NDEF, 0xFE = Terminator
  int i = 0;
  while (i < idx)
  {
    uint8_t tlv = buffer[i++];

    if (tlv == 0x00)
    {
      // Null TLV, seguir
      continue;
    }
    if (tlv == 0xFE)
    {
      // Terminator TLV
      break;
    }

    if (i >= idx)
      break;

    uint32_t length = 0;

    if (tlv == 0x03)
    {
      // NDEF Message
      uint8_t lenByte = buffer[i++];
      if (lenByte == 0xFF)
      {
        // Long form length (2 bytes)
        if (i + 1 >= idx)
          return false;
        length = (buffer[i] << 8) | buffer[i + 1];
        i += 2;
      }
      else
      {
        length = lenByte;
      }

      if (i + (int)length > idx)
        return false;

      // NDEF record completo
      uint8_t *rec = &buffer[i];
      i += length;

      if (length < 3)
        return false;

      uint8_t header = rec[0];
      bool sr = header & 0x10; // Short Record
      uint8_t typeLen = rec[1];
      int pos = 2;
      uint32_t payloadLen = 0;

      if (sr)
      {
        payloadLen = rec[pos++];
      }
      else
      {
        if (pos + 3 >= (int)length)
          return false;
        payloadLen = ((uint32_t)rec[pos] << 24) |
                     ((uint32_t)rec[pos + 1] << 16) |
                     ((uint32_t)rec[pos + 2] << 8) |
                     rec[pos + 3];
        pos += 4;
      }

      // Campo Type
      if (pos + typeLen > (int)length)
        return false;
      uint8_t *typeField = &rec[pos];
      pos += typeLen;

      // Debe ser tipo "T" (Text record)
      if (!(typeLen == 1 && typeField[0] == 'T'))
        return false;

      // Payload
      if (pos + (int)payloadLen > (int)length)
        return false;
      uint8_t *payload = &rec[pos];

      if (payloadLen < 1)
        return false;

      uint8_t status = payload[0];
      uint8_t langLen = status & 0x1F;

      if (1 + langLen > payloadLen)
        return false;

      // Texto a partir de payload[1 + langLen]
      outText = "";
      for (uint32_t k = 1 + langLen; k < payloadLen; k++)
      {
        char c = (char)payload[k];
        if (c >= 32 && c <= 126)
        {
          outText += c; // ASCII imprimible
        }
        else if (c == '\n' || c == '\r' || (uint8_t)c >= 160)
        {
          // permitir acentos y algunos caracteres extendidos
          outText += c;
        }
      }

      return outText.length() > 0;
    }
    else
    {
      // Otro tipo de TLV: saltamos su longitud
      if (i >= idx)
        break;
      uint8_t len = buffer[i++];

      if (len == 0xFF)
      {
        if (i + 1 >= idx)
          break;
        uint16_t l = (buffer[i] << 8) | buffer[i + 1];
        i += 2 + l;
      }
      else
      {
        i += len;
      }
    }
  }

  return false;
}

// ---------------------------------------------------------------------------
// Lee un bloque de MIFARE Classic (con clave por defecto FF..FF)
// y lo interpreta como ASCII
// ---------------------------------------------------------------------------
bool tryReadMifareClassicBlockASCII(uint8_t *uid, uint8_t uidLength, uint8_t blockNumber, String &outText)
{
  // Clave A por defecto
  uint8_t keya[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  // Autenticar el bloque
  if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, blockNumber, 0, keya))
  {
    // No es Classic o la clave no coincide
    return false;
  }

  uint8_t data[16];
  if (!nfc.mifareclassic_ReadDataBlock(blockNumber, data))
    return false;

  // Convertir a texto ASCII "c-string style"
  outText = "";
  for (int i = 0; i < 16; i++)
  {
    uint8_t b = data[i];
    if (b == 0x00)
      break; // fin de cadena
    if (b >= 32 && b <= 126)
    {
      outText += (char)b;
    }
    else if (b == '\n' || b == '\r' || b >= 160)
    {
      outText += (char)b;
    }
  }

  return outText.length() > 0;
}
