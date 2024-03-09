#include "receiver.h"


// ----------- Data ----------- //
char* message;
RH_RF95 tm = RH_RF95();
// ---------------------------- //

// ----- Helper Functions ----- //
uint32_t getLatestPID() {
  char* temp = (char*) malloc(255*sizeof(char)); //! 255 bytes is TOO generous, but better be safe than sorry
  uint16_t len = 0;

  //* The ID starts from the 9th character onwards every time
  for (uint16_t i=9; i<strlen(message); i++) {
    if (message[i] == ' ') {
      break;
    }

    temp[len] = message[i];
    len++;
  }

  uint32_t p_id = (uint32_t) atoi(temp);
  free(temp);

  return p_id;
}
// ---------------------------- //


// ------ Class Methods ------ //
Receiver::Receiver(float frequency = 0.0F) {
  freq = frequency;
}


bool Receiver::init() {
  if (!tm.init()) {
    return false;
  }

  //* initialise data file
  if (SD.exists("ASTERIUS_GS_DATA.txt")) {
    SD.remove("ASTERIUS_GS_DATA.txt");
  }

  this->df = SD.open("ASTERIUS_GS_DATA.txt");
  this->df.seek(0);

  //* setting configutarion
  tm.setFrequency(freq);
  tm.setTxPower(20);

  return true;
}


bool Receiver::receive() {
  uint8_t data[255];
  uint8_t len = sizeof(data);

  if(!tm.recv(data, &len)) {
    return false;
  }

  decrypt(1, (const char*) data);

  //* I'm very confident that I'll fuck the order up, so the absolute will save me (I hope)
  uint32_t latest = getLatestPID();
  if (abs(latest - lastKnownPID) > 1) {
    //? schwupps! was fehlt? DAS PACKET.
    this->log("Missed packets detected. Filling...");
    this->fill(lastKnownPID, latest);
  }
  lastKnownPID = latest;

  return true;
}


void Receiver::save(char* m) {
  this->df.println(m);
  this->df.flush();
}


void Receiver::fill(uint32_t start, uint32_t end) {
  for (start; start<=end; start++) {
    //? this section really is just... me going insane (JUST for memory allocation)
    uint16_t digits = 1; 
    do {
      digits++;
    } while((start/pow(digits, 10)) >= 1);

    //* in case anybody asks (I'm sure people will)
    //* 36 is the length of the string without the "%li"
    //* digits just helps me perfectly allocate the right amount so that I don't eat up the memory ;-;
    message = (char*) malloc((36+digits)*sizeof(char));
    snprintf(message, (36+digits)*sizeof(char), "Asterius:%li --- | --- --- --- --- ---", start);
    this->save(message);
  }
  free(message);
}


//! don't try this shit at home, I am seriously insane
void Receiver::decrypt(uint8_t offset, const char* data) {
  uint8_t counter = 1;
  message = (char*) malloc(sizeof(data));

  for (uint8_t i=0; i < strlen(data); i++) {
    if (isspace(data[i])) {
      message[i] = ' ';
      continue;
    }

    if ((int(data[i]) < 65 || int(data[i]) > 90) || (int(data[i]) < 97 || int(data[i]) > 122)) {
      continue; //* basically: do not decrypt symbols pls
    }

    uint8_t s = offset;
    if (counter % 2 == 1) {
      s = (26-offset);
    }

    uint8_t value = 97; //* for lowercase letters
    if (isupper(data[i])) {
      value = 65;
    }

    //? I really hope no one asks me to explain this, cause it's... yes
    message[i] = char(int(data[i] + s - value)%26 + value);
    counter++;
  }
}


void Receiver::log(const char* message) {
  Serial.print("[Log]: ");
  Serial.println(message);
}
// ---------------------------- //
