#ifndef INPUT_H
#define INPUT_H

#include <Wire.h>
#include <Arduino.h>

// Dichiarazione dei pin del joystick di navigazione
extern const int buttonPin_RST;    // the number of the reset button pin
extern const int buttonPin_SET;    // the number of the set button pin
extern const int buttonPin_MID;    // the number of the middle button pin
extern const int buttonPin_RHT;    // the number of the right button pin
extern const int buttonPin_LFT;    // the number of the left button pin
extern const int buttonPin_DWN;    // the number of the down button pin
extern const int buttonPin_UP;     // the number of the up button pin

// Funzione per inizializzare i pin di input
void setupInputPins();

#endif // INPUT_H
// the number of the up button pin
//setting dei pulsanti del joystick
  