#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "core/common/common.h"
#include "core/config/config.h"
#include "common.h"
#include <input.h>

// Funzione che ingloba la stampa su seriale e display
// display_pos_x e display_pos_y sono le coordinate del display
// display_char_size è la dimensione del carattere (1,2,3,4,5)
// color è il colore del testo (SSD1306_WHITE, SSD1306_BLACK)
// se DEBUG è attivo, stampa anche su seriale
// se DISPLAY_PRESENT è attivo, stampa anche sul display
// Check DEBUG e DISPLAY_PRESENT in config.h
void common::print(const char* message,int display_pos_x, int display_pos_y,int display_char_size,uint16_t color,uint16_t color_bkg) {
        if (DEBUG == 1) {
            Serial.print(message);
        }
        /*
        if (DISPLAY_PRESENT == 1) {
            // Assicurati che il display sia inizializzato
            if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
                Serial.println(F("Display OLED non trovato"));
                return; // Esci se il display non è inizializzato
            }*/
            display.setTextSize(display_char_size);
            display.setCursor(display_pos_x, display_pos_y);
            display.setTextColor(color);
            display.setTextColor(color, color_bkg);
            display.print(message);
            display.display();
        //}
    }

// Versione println: stampa su seriale e display OLED con newline
void common::println(const char* message,int display_pos_x, int display_pos_y,int display_char_size,uint16_t color, uint16_t color_bkg) {
    if (DEBUG == 1) {
        Serial.println(message);
    }
    /*
    if (DISPLAY_PRESENT == 1) {
        // Assicurati che il display sia inizializzato
        if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
            Serial.println(F("Display OLED non trovato"));
            return; // Esci se il display non è inizializzato
        }
    */
        display.setTextSize(display_char_size);
        display.setCursor(display_pos_x, display_pos_y);
        display.setTextColor(color, color_bkg);
        display.println(message);
        display.display();
    //}
}

void common::debounce(int waitTime) {
  // Funzione di debounce per i pulsanti
  delay(waitTime);
  while (digitalRead(buttonPin_SET) == LOW || digitalRead(buttonPin_RST) == LOW) {
    delay(10); // Attendi che il pulsante sia rilasciato
  }
}
void common::debounceButton(int buttonPin, int waitTime) {
  // Funzione di debounce per un singolo pulsante
  delay(waitTime);
  while (digitalRead(buttonPin) == LOW) {
    delay(10); // Attendi che il pulsante sia rilasciato
  }
}

int common::getSpacing(const char* str, int textSize) {
    // Ogni carattere è largo 5 pixel (5+0 spazio) moltiplicato per la dimensione
    int charWidth = 6 * textSize;
    return strlen(str) * charWidth;
}