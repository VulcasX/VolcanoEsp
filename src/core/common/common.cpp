#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "core/common/common.h"
#include "core/config/config.h"

// Funzione che ingloba la stampa su seriale e display
// display_pos_x e display_pos_y sono le coordinate del display
// display_char_size è la dimensione del carattere (1,2,3,4,5)
// color è il colore del testo (SSD1306_WHITE, SSD1306_BLACK)
// se DEBUG è attivo, stampa anche su seriale
// se DISPLAY_PRESENT è attivo, stampa anche sul display
// Check DEBUG e DISPLAY_PRESENT in config.h
void common::print(const char* message,int display_pos_x, int display_pos_y,int display_char_size,uint16_t color) {
        if (DEBUG == 1) {
            Serial.print(message);
        }
        if (DISPLAY_PRESENT == 1) {
            // Assicurati che il display sia inizializzato
            if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
                Serial.println(F("Display OLED non trovato"));
                return; // Esci se il display non è inizializzato
            }
            display.setTextSize(display_char_size);
            display.setCursor(display_pos_x, display_pos_y);
            display.setTextColor(color);
            display.print(message);
            display.display();
        }
    };


