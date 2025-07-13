#include <Arduino.h>
//libreria per funzionamento i2c
#include <Wire.h>
//librerie per funzionamento schermo oled i2c
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//libreria per funzionamento icone
#include <icons.h>
//libreria per il logo
#include <logo.h>
//libreria joystick
#include <input.h>
#include <LittleFS.h> //libreria per il file system
//libreria per il menu
#include "core/menu.h"

#include "core/config/config.h"
#include "core/common/common.h"
#include "core/common/virtualkeyboard.h"
#if DISPLAY_PRESENT == 1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);  // ✅ definizione unica
#endif

//bitmap icone

void setup() {
  // put your setup code here, to run once:
  Wire.begin(21,22);
  Serial.begin(115200);
  Serial.println("Hello, World!\n");

 if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Display OLED non trovato"));
    while(true);
  }
#if SPEEDBOOT == 0
//esegue il logo
  for(int j=0 ;j<1 ;j++){
    for (int i = 0; i < NUM_FRAMES; i++) {
        display.clearDisplay();
        display.drawBitmap(0, 0, frames[i], 128, 64, SSD1306_WHITE);
        display.display();
        delay(300);
    }
  }
  display.clearDisplay();
  common::print("Volcano ESP", 0, 0, 1, SSD1306_WHITE);
  delay(2000);
  #endif  
  // Inizializza il file system LittleFS
  if (!LittleFS.begin()) {
    //LittleFS.format(); 
    Serial.println("Errore LittleFS");
  }
  Serial.println("LittleFS OK");


  //da cambiare se ci sono altri input
  //setto i pin in uscita per il joystick
  setupInputPins();
 //setto il menu
   drawIconMenu();
}

void loop() {
  menuLoop();
}