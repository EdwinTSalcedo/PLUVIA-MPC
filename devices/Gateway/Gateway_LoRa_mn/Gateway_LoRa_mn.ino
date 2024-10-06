#include <SPI.h>
#include <RH_RF95.h>

// LoRA PinOut Configuration
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
#define RF95_FREQ 868.100000

RH_RF95 rf95(RFM95_CS, RFM95_INT);

void setup() {
  initSerial();
  initLoRa();
  Serial.println("LoRa Initialized. Waiting for activation signal...");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    processSerialInput(input);
  }

  if (rf95.available()) {
    receiveLoRaMessage();
  }
}

void initSerial() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  Serial.begin(115200);
  while (!Serial) delay(1);
}

void initLoRa() {
  resetLoRaModule();

  if (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }

  rf95.setTxPower(23, false);
}

void resetLoRaModule() {
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
}

void processSerialInput(String input) {
  // Example input: "ACTIVATE 5" or "ACTIVATE 10"
  if (input.startsWith("ACTIVATE")) {
    int duration = input.substring(9).toInt();
    //sendActivationSignal(duration);
    String message = "ACTIVATE " + String(duration);
    transmitMessage(message.c_str());
    Serial.println("Activation signal sent: " + message);
  }
}

void transmitMessage(const char* message) {
  rf95.send((uint8_t *)message, strlen(message));
  
  if (rf95.waitPacketSent()) {
    Serial.println("Message sent successfully");
  } else {
    Serial.println("Message sending failed");
  }
}

void receiveLoRaMessage() {
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (rf95.recv(buf, &len)) {
    buf[len] = '\0';
    String receivedMessage = String((char*)buf);
    
    if (receivedMessage == "END RAW DATA") {
      Serial.println("End of raw data received.");
      resetLocalVariables();
    } else {
      Serial.println("Received: " + receivedMessage);
    }
  } else {
    Serial.println("Receive failed");
  }
}

void resetLocalVariables() {
  // Reset any local variables or states as needed
  Serial.println("Local variables reset. Waiting for next activation signal...");
}

void endOfDay() {
  Serial.println("End of day. Closing program and serial connection.");
  Serial.end();
  while (true) {} // Keep the program ended
}
