#include <Arduino.h>

#include "receiver.h"

#define SD_CHIP_SELECT 5
#define FREQUENCY 433.2F

Receiver rv(FREQUENCY);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!SD.begin(SD_CHIP_SELECT) && rv.init()) {
    Serial.println("Initialisation failed. Please check wirings and/or addresses.");
    return;
  }
  Serial.println("Ready!");
}

void loop() {
  if (rv.receive()) {
    rv.log("Packet received. Fabulous.");
    rv.save(rv.message);
    rv.log(rv.message);
  }

  while (Serial.available() != 0) {
    if (Serial.readString() == "detach") {
      char order[] = "Asterius:DETACH [G->M]";
      rv.send(order);
    }
  }
}