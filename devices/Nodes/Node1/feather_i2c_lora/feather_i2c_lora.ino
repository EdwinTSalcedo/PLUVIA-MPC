#include <SPI.h>
#include <RH_RF95.h>
#include <Wire.h>
#include <ADS1115_WE.h>

// LoRA PinOut Configuration
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
#define RF95_FREQ 868.100000
#define I2C_SLAVE_ADDR 0x04  // Address of the Feather as the I2C slave
#define I2C_ADDRESS_SENSOR 0x48
#define I2C_ADDRESS_TTGO 0x09  // Dirección I2C de la TTGO

const int relayPin = 12;  // Pin for the relay control
String node = "";  // Store the node identifier
RH_RF95 rf95(RFM95_CS, RFM95_INT);
ADS1115_WE adc = ADS1115_WE(I2C_ADDRESS_SENSOR);

volatile bool i2cDataReceived = false;
String i2cMessage1 = "";

int activationTime1 = 0;
int activationTime2 = 0;
int activationTime3 = 0;

unsigned long startMillis = 0;
bool relayOff = false;
bool readingSensor1 = false;
bool readingSensor2 = false;

void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);

  initLoRa();
  initI2C();
  initSensor1();
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (rf95.available()) {
    receiveMessage();
    String msg = "Mensaje de activacion recibido: " + String(activationTime1) + ", " + String(activationTime2) + ", " + String(activationTime3);
    transmitMessage(msg.c_str());
  }

  if (node == "1") {
    if (i2cDataReceived) {
      transmitMessage(i2cMessage1.c_str());  // Send message via LoRa
      if (i2cMessage1 == "END RAW DATA JETSON") {
        delay(5000);
        digitalWrite(relayPin, HIGH);  // Turn off the relay
        relayOff = true;
        startMillis = currentMillis;
        readingSensor1 = true;  // Start reading sensor 1
      }
      i2cDataReceived = false;  // Reset flag
      i2cMessage1 = "";
    }

    if (relayOff) {
      if (readingSensor1 && (currentMillis - startMillis < activationTime1 * 60000)) {
        float voltage = readChannel(ADS1115_COMP_0_GND);
        if (voltage != -1.0) {  // Check if voltage reading is valid
           String msg = "Sensor 1: " + String(voltage);
           transmitMessage(msg.c_str());
         }
      } else if (readingSensor1 && (currentMillis - startMillis >= activationTime1 * 60000)) {
        readingSensor1 = false;
        startMillis = currentMillis;
        readingSensor2 = true;  // Start reading sensor 2
      }

      if (readingSensor2 && (currentMillis - startMillis < activationTime2 * 60000)) {
        float voltage = readChannel(ADS1115_COMP_1_GND);
        if (voltage != -1.0) {  // Check if voltage reading is valid
            String msg = "Sensor 2: " + String(voltage);
            transmitMessage(msg.c_str());
        }
      } else if (readingSensor2 && (currentMillis - startMillis >= activationTime2 * 60000)) {
        readingSensor2 = false;
        String msg_off = "END RAW DATA";
        transmitMessage(msg_off.c_str());
        relayOff = false;  // Reset the relay off flag
      }
    }
  } else if (node == "2") {
    // Send the received message to TTGO via I2C
    if (sendToTTGO(i2cMessage1)) {
      String response = readFromTTGO();  // Read the response from TTGO
      String formattedResponse = "N3: " + response;
      if (response != "") {
        Serial.println("Respuesta del TTGO: " + formattedResponse);
        transmitMessage(formattedResponse.c_str());
      } else {
        Serial.println("Esperando respuesta nodo 2");
      }
    } else {
      Serial.println("Error: I2C TTGO not found");
    }
  }
  
  delay(500);
}

void initLoRa() {
  if (!rf95.init()) {
    Serial.println("LoRa initialization failed!");
    while (1);
  }
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("LoRa frequency set failed!");
    while (1);
  }
  rf95.setTxPower(23, false);
  Serial.println("LoRa incializada correctamente");
}

void initI2C() {
  Wire.begin(I2C_SLAVE_ADDR);  // Start I2C as slave with specified address
  Wire.onReceive(receiveEvent); // Register event for receiving data
  if (!isI2CDevicePresent(I2C_SLAVE_ADDR)) {
    Serial.println("Error: I2C Jetson not found");
  } else {
    Serial.println("I2C Jetson initialized successfully");
  }
}

void initSensor1() {
  if (!isI2CDevicePresent(I2C_ADDRESS_SENSOR)) {
    Serial.println("Error: I2C ADS1115 not found");
    return;
  }
  adc.setVoltageRange_mV(ADS1115_RANGE_6144);
  adc.setCompareChannels(ADS1115_COMP_0_GND);
  adc.setMeasureMode(ADS1115_CONTINUOUS);
  Serial.println("ADS1115 incializado correctamente");
}

void receiveMessage() {
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (rf95.recv(buf, &len)) {
    buf[len] = '\0';
    String receivedMessage = String((char*)buf);
    if (receivedMessage.startsWith("ACTIVATE")) {
      int first_space = receivedMessage.indexOf(' ', 9);
      int second_space = receivedMessage.indexOf(' ', first_space + 1);
      int third_space = receivedMessage.indexOf(' ', second_space + 1);

      node = receivedMessage.substring(9, first_space);  // Extract the node number as string
      activationTime1 = receivedMessage.substring(first_space + 1, second_space).toInt();
      activationTime2 = receivedMessage.substring(second_space + 1, third_space).toInt();
      activationTime3 = receivedMessage.substring(third_space + 1).toInt();

      digitalWrite(relayPin, LOW);  // Activate the relay to power up Jetson
    }
  } else {
    // Transmission failed or message not received
  }
}


void receiveEvent(int howMany) {
  i2cMessage1 = "";
  bool firstChar = true; 
  while (Wire.available()) {
    char c = Wire.read();
    if (firstChar) {
      // Si es el primer carácter, ignóralo y cambia el flag
      firstChar = false;
    } else {
      i2cMessage1 += c;
    }
  }

  i2cDataReceived = true;  // Set flag to indicate data received
}

void transmitMessage(const char* message) {
  rf95.send((uint8_t *)message, strlen(message));
}

float readChannel(int channel) {
    if (!isI2CDevicePresent(I2C_ADDRESS_SENSOR)) {
        Serial.println("Error: ADS1115 not found during read");
        return -1.0;
    }
    adc.setCompareChannels(channel);
    return adc.getResult_V();
}

bool sendToTTGO(String message) {
  Wire.beginTransmission(I2C_ADDRESS_TTGO);
  Wire.write(message.c_str());
  byte error = Wire.endTransmission();
  return (error == 0);
}

String readFromTTGO() {
  Wire.requestFrom(I2C_ADDRESS_TTGO, 32); // Solicitar 32 bytes del TTGO
  String response = "";
  while (Wire.available()) {
    char c = Wire.read();
    if (isValidChar(c)) {
      response += c;  // Agregar el carácter válido a la respuesta
    } else {
      response += "";  // Omitir caracteres no válidos
    }
  }
  return response;
}

// Función para verificar si un byte es un carácter válido de UTF-8
// Función para verificar si un carácter es válido
bool isValidChar(char c) {
  return (c >= 'A' && c <= 'Z') ||  // Letras mayúsculas
         (c >= 'a' && c <= 'z') ||  // Letras minúsculas
         (c >= '0' && c <= '9') ||  // Números
         c == ',' || c == '.' ||    // Comas y puntos
         c == ' ';                  // Espacios vacíos (si se consideran válidos)
}

bool isI2CDevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return (Wire.endTransmission() == 0);
}
