#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>

// Definir los pines usados por el módulo transceptor LoRa
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

// Definir la frecuencia LoRa (866E6 para Europa)
#define BAND 866E6

// Dirección I2C del esclavo
#define I2C_ADDRESS 0x09

// Variable para almacenar el mensaje LoRa
String loRaMessage = "";

void setup() {
  // Inicializar el Monitor Serial
  Serial.begin(115200);

  // Inicializar el bus I2C como esclavo
  Wire.begin(I2C_ADDRESS);

  // Configurar la función de recepción de datos por I2C
  Wire.onReceive(receiveEvent);
  Wire.onRequest(onRequest);

  // Inicializar los pines SPI para LoRa
  SPI.begin(SCK, MISO, MOSI, SS);

  // Configurar los pines del módulo LoRa
  LoRa.setPins(SS, RST, DIO0);
  
  // Iniciar la comunicación LoRa
  if (!LoRa.begin(BAND)) {
    Serial.println("¡Fallo en la inicialización de LoRa!");
    while (1);
  }
  Serial.println("LoRa iniciado correctamente.");
}

void loop() {
  // Verificar si hay paquetes LoRa recibidos
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Recibir el paquete LoRa
    String receivedMessage = "";
    while (LoRa.available()) {
      receivedMessage += (char)LoRa.read();
    }

    // Mostrar el mensaje recibido por LoRa en el monitor serial
    Serial.print("Mensaje recibido por LoRa: ");
    Serial.println(receivedMessage);

    // Almacenar el mensaje recibido por LoRa
    loRaMessage = receivedMessage;
  }

  delay(100);
}

// Evento que se llama cuando se reciben datos I2C
void receiveEvent(int howMany) {
  Serial.print("Datos recibidos por I2C: ");
  String receivedMessage = "";
  while (Wire.available()) {
    char c = Wire.read();  // Leer los datos recibidos por I2C
    receivedMessage += isValidUTF8(c) ? c : '1';  // Validar y agregar carácter
  }
  Serial.println(receivedMessage);
  
  // Enviar un mensaje por LoRa
  LoRa.beginPacket();
  LoRa.println(receivedMessage);
  LoRa.endPacket();
  
  Serial.println("Mensaje enviado por LoRa: " + receivedMessage);
}

// Evento que se llama cuando el maestro solicita datos
void onRequest() {
  // Iniciar la transmisión I2C
  Wire.beginTransmission(I2C_ADDRESS);
  
  if (loRaMessage.length() > 0) {
    // Enviar el mensaje LoRa almacenado al maestro I2C si hay un mensaje disponible
    for (size_t i = 0; i < loRaMessage.length(); i++) {
      char c = loRaMessage[i];
      Wire.write(isValidUTF8(c) ? c : ' ');  // Validar y enviar carácter
    }

    // Rellenar los bytes restantes con espacios vacíos
    for (size_t i = loRaMessage.length(); i < 32; i++) {
      Wire.write(' ');
    }
    
    Serial.println("Mensaje enviado por I2C: " + loRaMessage);
    // Limpiar el mensaje LoRa después de enviarlo para evitar múltiples envíos
    loRaMessage = "";
  } else {
    Serial.println("No hay mensaje LoRa para enviar por I2C.");
    // Rellenar todos los 32 bytes con espacios vacíos si no hay mensaje
    for (size_t i = 0; i < 32; i++) {
      Wire.write(' ');
    }
  }

  // Finalizar la transmisión I2C
  Wire.endTransmission();
}


// Función para verificar si un byte es un carácter válido de UTF-8
bool isValidUTF8(char c) {
  return (c >= 0 && c <= 127) || ((c & 0xC0) == 0xC0);  // Validar según UTF-8
}
