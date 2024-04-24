#include "receiver.h"

#define RFM_CHIP_SELECT 4
#define RFM_INTERRUPT 3
#define RFM_RESET 2

// ----------- Data ----------- //
char* message = (char*) malloc(255*sizeof(char));
RH_RF95 tm(RFM_CHIP_SELECT, RFM_INTERRUPT);
// ---------------------------- //

// ----- Helper Functions ----- //
uint32_t getLatestPID(char unit) {
  if (((unit != 'M') && (unit != 'S')) || (message[strlen(message)-2] != unit)) {
    return 0; //? really?
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

Receiver::~Receiver() {
  free(message);
  this->df.close();
}

bool Receiver::init() {
  if (!tm.init()) {
    return false;
  }

  tm.setFrequency(freq);

  //* initialise data file
  if (SD.exists("ASTERIUS_GS_DATA.txt")) {
    SD.remove("ASTERIUS_GS_DATA.txt");
  }

  this->df = SD.open("ASTERIUS_GS_DATA.txt");
  return this->df;
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
  if ((M_latest != 0) && (M_latest - lastKnownM_PID > 1)) {
    //? schwupps! was fehlt? DAS PACKET.
    this->log("Missed packets from Master detected. Filling...");
    this->fill(lastKnownM_PID, M_latest, 'M');
  }
  lastKnownM_PID = M_latest;

  uint32_t S_latest = getLatestPID('S');
  if ((S_latest != 0) && (S_latest - lastKnownS_PID > 1)) {
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
    char* pseudo = (char*) malloc((40+digits)*sizeof(char));
    snprintf(pseudo, (40+digits)*sizeof(char), "Asterius:%li --- | --- --- --- --- --- [%c]", start, unit);
    this->save(pseudo);
    free(pseudo);
  }
}


//! don't try this shit at home, I am seriously insane
void Receiver::process(uint8_t mode, char* data, uint8_t offset) {
  //* modes: 1 - encrypt / 2 - decrypt
  uint8_t mod = (mode == 1) ? 0 : 1;
  uint8_t c = 1;

  for (uint8_t i=0; i < strlen(data); i++) {
    if (isspace(data[i]) || (data[i] < 65 || data[i] > 90) && (data[i] < 97 || data[i] > 122)) {
      continue; //* too much; didn't understand: only process letters
    }

    uint8_t s = offset;
    if (c % 2 == mod) {
      s = (26-offset);
    }

    uint8_t value = 97; //* for lowercase letters
    if (isupper(data[i])) {
      value = 65;
    }

    //? I really hope no one asks me to explain this, cause it's... yes
    data[i] = (char)((int)(data[i] + s - value) % 26 + value);
    c++;
  }

  message = data;
}


void Receiver::log(const char* message) {
  Serial.print("[Log]: ");
  Serial.println(message);
}
// ---------------------------- //
