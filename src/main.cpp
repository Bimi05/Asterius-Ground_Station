#include <Arduino.h>
#include "receiver.h"

#define SD_CHIP_SELECT 5
#define FREQUENCY 433.2F

Receiver rv(FREQUENCY);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  SD.begin(SD_CHIP_SELECT);

  const char* init = (rv.init()) ? "Ready!" : "Initialisation failed.";
  Serial.println(init);
}

void loop() {
  if (rv.receive()) {
    rv.save(rv.message); //* this is modified by .receive(), so we're safe (probably) :)
    rv.log("Packet received. Fabulous.");
    rv.log(rv.message);
  }

  while (Serial.available() != 0) {
    if (Serial.readString() == "detach") {
      char order[] = "Asterius:DETACH [G->M]";
      rv.sendMessage(order);
    }
  }
}
