#include <Arduino.h>

#include "receiver.h"

#define SD_SPI_ADDR 5
#define FREQUENCY 434.2F

Receiver rv(FREQUENCY);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!(SD.begin(SD_SPI_ADDR) && rv.init())) {
    Serial.println("Could not initialise sensors, check wirings and addresses.");
  }
  Serial.println("Ready!");
}

void loop() {
  if (rv.receive()) {
    rv.save(rv.message); //* this is modified by .receive(), so we're safe (probably) :)
    rv.log("Packet received. Fabulous.");
  }
}
