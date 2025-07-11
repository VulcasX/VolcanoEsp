#include "input.h"
#include <Arduino.h>
void setupInputPins() {
  pinMode(buttonPin_RST, INPUT_PULLUP);
  pinMode(buttonPin_SET, INPUT_PULLUP);
  pinMode(buttonPin_MID, INPUT_PULLUP);
  pinMode(buttonPin_RHT, INPUT_PULLUP);
  pinMode(buttonPin_LFT, INPUT_PULLUP);
  pinMode(buttonPin_DWN, INPUT_PULLUP);
  pinMode(buttonPin_UP, INPUT_PULLUP);
}
