#include "receiver.h"


// ----------- Data ----------- //
char* message;
RH_RF95 tm = RH_RF95();
// ---------------------------- //

// ----- Helper Functions ----- //
uint32_t getLatestPID(char unit) {
  if ((unit != 'M') && (unit != 'S')) {
    return 0; //? really?
  }

  if (message[strlen(message)-2] != unit) {
    return 0;
  }

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
Receiver::Receiver(float frequency) {
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
  tm.setModeRx();

  return true;
}


bool Receiver::receive() {
  uint8_t data[255];
  uint8_t len = sizeof(data);

  if(!tm.recv(data, &len)) {
    return false;
  }

  process(2, (char*) data, 1);

  if (strstr(message, "[S->M]") || strstr(message, "[M->S]")) {
    return false; //* this IS ours, but we don't need this here
  }


  //* I'm very confident that I'll fuck the order up, so the absolute will save me (I hope)
  uint32_t M_latest = getLatestPID('M');
  if ((M_latest != 0) && (abs(M_latest - lastKnownM_PID) > 1)) {
    //? schwupps! was fehlt? DAS PACKET.
    this->log("Missed packets from Master detected. Filling...");
    this->fill(lastKnownM_PID, M_latest, 'M');
  }
  lastKnownM_PID = M_latest;

  uint32_t S_latest = getLatestPID('S');
  if ((S_latest != 0) && (abs(S_latest - lastKnownS_PID) > 1)) {
    this->log("Missed packets from Slaves detected. Filling...");
    this->fill(lastKnownS_PID, S_latest, 'S');
  }
  lastKnownS_PID = S_latest;

  return true;
}


void Receiver::save(char* m) {
  this->df.println(m);
  this->df.flush();
}


void Receiver::fill(uint32_t start, uint32_t end, char unit) {
  for (start; start<=end; start++) {
    //? this section really is just... me going insane (JUST for memory allocation)
    uint16_t digits = 1; 
    do {
      digits++;
    } while((start/pow(digits, 10)) >= 1);

    //* in case anybody asks (I'm sure people will)
    //* 40 is the predetermined length of the string
    //* digits just helps me perfectly allocate the right amount so that I don't eat up the memory ;-;
    message = (char*) malloc((40+digits)*sizeof(char));
    snprintf(message, (40+digits)*sizeof(char), "Asterius:%li --- | --- --- --- --- --- [%c]", start, unit);
    this->save(message);
    free(message);
  }
}


//! don't try this shit at home, I am seriously insane
void Receiver::process(uint8_t mode, char* data, uint8_t offset) {
  //* modes: 1 - encrypt / 2 - decrypt
  uint8_t mod = (mode == 1) ? 0 : 1;

  uint8_t counter = 1;
  message = (char*) malloc(sizeof(data));

  for (uint8_t i=0; i < strlen(data); i++) {
    if (isspace(data[i])) {
      message[i] = ' ';
      continue;
    }

    if (((int) data[i] < 65 || (int) data[i] > 90) || ((int) data[i] < 97 || (int) data[i] > 122)) {
      continue; //* too much; didn't understand: only process letters
    }

    uint8_t s = offset;
    if (counter % 2 == mod) {
      s = (26-offset);
    }

    uint8_t value = 97; //* for lowercase letters
    if (isupper(data[i])) {
      value = 65;
    }

    //? I really hope no one asks me to explain this, cause it's... yes
    message[i] = (char)((int)(data[i] + s - value) % 26 + value);
    counter++;
  }
}


void Receiver::log(const char* message) {
  Serial.print("[Log]: ");
  Serial.println(message);
}
// ---------------------------- //
