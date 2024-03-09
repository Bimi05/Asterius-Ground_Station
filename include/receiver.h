#pragma once

#include <SD.h>
#include <RH_RF95.h>

class Receiver {
  private:
    float freq;
    File df; // our data file

    // "Fills" in blank packets in a range of IDs.
    void fill(uint32_t start, uint32_t end);

    // Decrypt the incoming data
    void decrypt(uint8_t offset, const char* data);

  public:
    char* message;
    uint32_t lastKnownPID; // This helps in the checkMissed() function below :)

    Receiver(float);

    // Initialise and configure the receiver (RH_RF95 module used)
    bool init();

    // Check if we were able to receive a packet
    // Return true if yes, false otherwise
    bool receive();

    // Saves most recent stored message to the micro-SD card
    void save(char* message);

    // Log a message to the serial monitor
    void log(const char* message);
};
