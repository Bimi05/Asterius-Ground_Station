#include "receiver.h"

#define RFM_CHIP_SELECT 6
#define RFM_INTERRUPT 3
#define RFM_RESET 2

RH_RF95 tm(RFM_CHIP_SELECT, RFM_INTERRUPT);

// ----- Helper Functions ----- //
uint32_t getLatestPID(char* message, char unit) {
  size_t u_index = strlen(message)-2;
  if (((unit != 'M') && (unit != 'S')) || (message[u_index] != unit)) {
    return 0; //? really?
  }

  char temp[10];
  for (uint16_t i=9; i<strlen(message); i++) { //* ID starts from 9th character in array
    if (message[i] == ' ') {
      break;
    }

    temp[i-9] = message[i];
  }
  return (uint32_t) atoi(temp);
}
// ---------------------------- //

// ------ Initialisation ------ //
Receiver::Receiver(float frequency) {
  freq = frequency;
}

Receiver::~Receiver() {
  df.close();
}

bool Receiver::init() {
  if (!tm.init()) {
    return false;
  }

  tm.setFrequency(freq);
  tm.setModeRx();

  return true;
}
// ---------------------------- //


// -------- Processing -------- //
char* Receiver::process(uint8_t mode, char* data, uint8_t offset) {
  //! don't try this shit at home, I am seriously insane
  //* modes: 1 - encrypt / 2 - decrypt
  uint8_t mod = (mode == 1) ? 0 : 1;
  uint8_t c = 1;

  for (uint8_t i=0; i < strlen(data); i++) {
    if (isspace(data[i]) || ((data[i] < 65 || data[i] > 90) && (data[i] < 97 || data[i] > 122))) {
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

  return data;
}

bool Receiver::receive() {
  uint8_t data[250];
  uint8_t len = sizeof(data);

  if(!tm.recv(data, &len)) {
    return false;
  }

  char* packet = process(2, (char*) data, 1);
  if (strstr(packet, "[S->M]") != NULL || strstr(packet, "[M->S]") != NULL) {
    return false; //* this IS ours, but we can safely ignore it
  }

  uint32_t M_latest = getLatestPID(packet, 'M');
  if ((M_latest != 0) && (M_latest - lastM_PID > 1)) {
    this->fill(lastM_PID, M_latest, 'M');
  }

  uint32_t S_latest = getLatestPID(packet, 'S');
  if ((S_latest != 0) && (S_latest - lastS_PID > 1)) {
    this->fill(lastS_PID, S_latest, 'S');
  }

  lastM_PID = M_latest;
  lastS_PID = S_latest;
  message = packet;

  return true;
}

void Receiver::save(char* message) {
  df = SD.open("ASTERIUS.TXT", FILE_WRITE);
  if (df) {
    df.println(message);
    df.flush();
  }
  df.close();
}

void Receiver::fill(uint32_t start, uint32_t end, char unit) {
  char pseudo[250];
  for (uint32_t i=1; i<=(end-start); i++) {
    snprintf(pseudo, 250*sizeof(char), "Asterius:%li --- | --- --- --- --- --- [%c]", start+i, unit);
    this->save(pseudo);
  }
}
// ---------------------------- //


// ----------- Misc ----------- //
void Receiver::send(char* message) {
  tm.setModeTx();
  tm.setTxPower(20); //* make sure it reaches

  char* resp = process(1, message, 1);
  tm.send((uint8_t*) resp, strlen(resp));
  tm.waitPacketSent();

  tm.setModeRx();
}

void Receiver::log(const char* message) {
  Serial.print("[Log]: ");
  Serial.println(message);
  Serial.flush();
}
// ---------------------------- //