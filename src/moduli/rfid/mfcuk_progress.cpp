/**
 * MFCUK - Funzione di aggiornamento del progresso
 * 
 * Implementa la funzione mfcuk_update_progress dichiarata in mfcuk.h
 * che aggiorna il progresso dell'attacco sia su display che su Serial.
 */

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "mfcuk.h"

// Riferimento al display OLED
extern Adafruit_SSD1306 display;

/**
 * Aggiorna lo stato del progresso dell'attacco
 * @param progress Percentuale di completamento (0-100)
 * @param status Messaggio di stato da visualizzare
 * 
 * NOTA: Commentata per evitare duplicazione con mfcuk.cpp
 */
/* 
void mfcuk_update_progress(int progress, const char* status) {
    // Assicurati che il progresso sia nell'intervallo corretto
    if (progress < 0) progress = 0;
    if (progress > 100) progress = 100;
    
    // Log su Serial
    Serial.print("[MFCUK] Progresso: ");
    Serial.print(progress);
    Serial.print("% - ");
    Serial.println(status);
    
    // Aggiorna il display se disponibile
    if (&display != nullptr) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        
        // Titolo
        display.setCursor(0, 0);
        display.println("MFCUK Attack");
        display.println();
        
        // Messaggio di stato
        display.setCursor(0, 16);
        display.println(status);
        
        // Barra di progresso
        display.drawRect(0, 36, 128, 10, SSD1306_WHITE);
        display.fillRect(2, 38, (124 * progress) / 100, 6, SSD1306_WHITE);
        
        // Percentuale
        display.setCursor(54, 48);
        display.print(progress);
        display.print("%");
        
        // Aggiorna il display
        display.display();
    }
}
*/
