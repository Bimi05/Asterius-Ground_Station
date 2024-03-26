#pragma once

#include <SD.h>
#include <RH_RF95.h>

class Receiver {
  private:
    File df; // our data file
    float freq;
    uint32_t lastKnownM_PID;
    uint32_t lastKnownS_PID;

    // "Fills" in blank packets in a range of IDs.
    void fill(uint32_t start, uint32_t end, char unit);

    // Decrypt the incoming data
    void process(uint8_t mode, char* data, uint8_t offset);

  public:
    char* message;

    Receiver(float frequency);

    // Initialise and configure the receiver (using the RH_RF95 module)
    bool init();

    // Check if we were able to receive a packet
    // Return true if yes, false otherwise
    bool receive();

    // Saves most recent stored message to the micro-SD card
    void save(char* message);

    // Log a message to the serial monitor
    void log(const char* message);
};
