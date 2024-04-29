#ifndef __RECEIVER__
#define __RECEIVER__

#include <SD.h>
#include <RH_RF95.h>

class Receiver {
  private:
    uint32_t lastM_PID;
    uint32_t lastS_PID;

    File df;
    float freq;

    // Fills in blank packets in a range of IDs
    void fill(uint32_t, uint32_t, char);

    // Process (Encrypt/Decrypt) the incoming data
    // Return a pointer to the processed string
    char* process(uint8_t, char*, uint8_t);

  public:
    Receiver(float);
    ~Receiver();

    char* message;

    // Initialise and configure the receiver (using the RH_RF95 module)
    // Return true if initialisation was successful
    bool init();

    // Check if we were able to receive a packet
    // Return true if a packet was received
    bool receive();

    // Saves the specified message to the micro-SD card
    void save(char*);

    // Log a message to the serial monitor
    void log(const char*);

    // Send a message to the Master CanSat
    void send(char*);
};

#endif