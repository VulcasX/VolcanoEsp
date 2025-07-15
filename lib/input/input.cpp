#include "input.h"
#include <Arduino.h>

// Definizione dei pin del joystick di navigazione
const int buttonPin_RST = 26;     // the number of the reset button pin
const int buttonPin_SET = 4;      // the number of the set button pin
const int buttonPin_MID = 25;     // the number of the middle button pin
const int buttonPin_RHT = 27;     // the number of the right button pin
const int buttonPin_LFT = 14;     // the number of the left button pin
const int buttonPin_DWN = 12;     // the number of the down button pin
const int buttonPin_UP = 17;      // the number of the up button pin

void setupInputPins() {
  pinMode(buttonPin_RST, INPUT_PULLUP);
  pinMode(buttonPin_SET, INPUT_PULLUP);
  pinMode(buttonPin_MID, INPUT_PULLUP);
  pinMode(buttonPin_RHT, INPUT_PULLUP);
  pinMode(buttonPin_LFT, INPUT_PULLUP);
  pinMode(buttonPin_DWN, INPUT_PULLUP);
  pinMode(buttonPin_UP, INPUT_PULLUP);
}
