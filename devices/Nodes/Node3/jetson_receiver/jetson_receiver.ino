#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>

// Define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

// Define the LoRa frequency
#define BAND 866E6

// I2C address for TTGO as slave
#define I2C_SLAVE_ADDR 0x30

const int relayPin = 12;  // Pin for the relay control

volatile bool i2cDataReceived = false;
String i2cMessage1 = "";

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Setup relay pin
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);  // Ensure relay is off initially
  
  // Initialize LoRa
  initLoRa();
  
  // Initialize I2C as slave
  Wire.begin(I2C_SLAVE_ADDR);
  Wire.onReceive(receiveEvent);  // Register event for receiving data
}

void loop() {
  // Check for incoming LoRa messages
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Received a packet
    String loRaData = "";
    
    while (LoRa.available()) {
      loRaData += (char)LoRa.read();
    }

    Serial.print("Received packet: ");
    Serial.println(loRaData);

    // Check if message is an activation command
    if (loRaData.startsWith("ACTIVATE")) {
      digitalWrite(relayPin, LOW);  // Activate the relay to power up Jetson
      String msg = "Activation command received";
      transmitMessage(msg.c_str());
    }
  }

  // Process I2C data if received
  if (i2cDataReceived) {
    // Send data received via I2C to LoRa
    transmitMessage(i2cMessage1.c_str());
    
    // Check if the message is an end command
    if (i2cMessage1 == "END RAW DATA") {
      delay(5000);  // Wait for a while
      digitalWrite(relayPin, HIGH);  // Turn off the relay
    }
    
    // Reset flags
    i2cDataReceived = false;
    i2cMessage1 = "";
  }
}

void initLoRa() {
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initializing OK!");
}

void receiveEvent(int howMany) {
  i2cMessage1 = "";
  bool firstChar = true;
  
  while (Wire.available()) {
    char c = Wire.read();
    if (firstChar) {
      // If it's the first character, ignore it
      firstChar = false;
    } else {
      // Append the character to the message
      i2cMessage1 += c;
    }
  }
  
  // Mark that I2C data has been received
  i2cDataReceived = true;
  Serial.print("I2C message received: ");
  Serial.println(i2cMessage1);
}

void transmitMessage(const char* message) {
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();
  
  Serial.print("Message sent via LoRa: ");
  Serial.println(message);
}
