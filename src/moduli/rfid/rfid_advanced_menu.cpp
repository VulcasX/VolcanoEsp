#include "core/menu/rfidmenu.h"
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "core/common/common.h"
#include "moduli/rfid/rfid.h"
#include "moduli/rfid/mfoc.h"
#include "moduli/rfid/mfoc_keys.h"
#include <input.h>

// Riferimento al display OLED
extern Adafruit_SSD1306 display;

// Menu RFID avanzato che include MFOC e MFCUK
void rfid_advanced_menu() {
    const char* voci[] = {"MFCUK Attack", "MFOC Classic", "MFOC Avanzato", "MFOC Dump Completo", "Key Manager", "Indietro"};
    const int vociCount = sizeof(voci)/sizeof(voci[0]);
    int selezione = 0;
    unsigned long rstPressStart = 0;
    bool needRedraw = true;
    
    while(true) {
        if (needRedraw) {
            // Disegna l'interfaccia del menu
            display.clearDisplay();
            common::println("RFID Advanced", 0, 0, 1, SSD1306_WHITE);
            
            for(int i=0; i<vociCount; i++) {
                if(i == selezione) {
                    display.fillRect(0, 10+i*12, 128, 12, SSD1306_WHITE);
                    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
                } else {
                    display.setTextColor(SSD1306_WHITE);
                }
                display.setCursor(2, 12+i*12);
                display.print(voci[i]);
            }
            
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 10+vociCount*12);
            display.print("RST: Esci");
            
            display.display();
            needRedraw = false;
        }
        
        // Navigazione - pulsante UP
        if(digitalRead(buttonPin_UP) == LOW) {
            selezione--;
            if(selezione < 0) selezione = vociCount-1;
            common::debounceButton(buttonPin_UP, 120);
            needRedraw = true;
        }
        
        // Navigazione - pulsante DOWN
        if(digitalRead(buttonPin_DWN) == LOW) {
            selezione = (selezione+1)%vociCount;
            common::debounceButton(buttonPin_DWN, 120);
            needRedraw = true;
        }
        
        // Selezione - pulsante SET
        if(digitalRead(buttonPin_SET) == LOW) {
            common::debounceButton(buttonPin_SET, 120);
            switch(selezione) {
                case 0: 
                    mfcuk_menu();    // Menu MFCUK
                    break;
                case 1: 
                    mfoc_menu();     // Menu MFOC
                    break;
                case 2: 
                    mfoc_menu_mod(); // Menu MFOC Modificato
                    break;
                case 3: 
                    mfoc_dump_complete(); // MFOC Dump Completo
                    break;
                case 4: 
                    mfoc_key_manager(); // Gestione chiavi
                    break;
                case 5: 
                    return;          // Indietro
            }
            needRedraw = true;
        }
        
        // Uscita con RST premuto
        if(digitalRead(buttonPin_RST) == LOW) {
            if(rstPressStart == 0) rstPressStart = millis();
            // Uscita immediata senza attendere 2 secondi
            if(millis() - rstPressStart > 100) {
                common::debounceButton(buttonPin_RST, 50);
                return;
            }
        } else {
            rstPressStart = 0;
        }
        
        delay(10);  // Piccolo ritardo per evitare consumo CPU
    }
}
