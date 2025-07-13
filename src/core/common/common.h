#pragma once

#include <Adafruit_SSD1306.h>

class common {
public:
    //funzione che ingloba la stampa su seriale e display
    //display_pos_x e display_pos_y sono le coordinate del display
    //display_char_size è la dimensione del carattere (1,2,3,4,5)
    //color è il colore del testo (SSD1306_WHITE, SSD1306_BLACK)
    //se DEBUG è attivo, stampa anche su seriale
    //se DISPLAY_PRESENT è attivo, stampa anche sul display
    //Check DEBUG e DISPLAY_PRESENT in config.h
    //Per il display, usa SSD1306_WHITE per il testo bianco su sfondo nero
    //Per il display, usa SSD1306_BLACK per il testo nero su sfondo bianco
    //Per il display, usa SSD1306_INVERSE per invertire i colori
    static void print(const char* message,int display_pos_x, int display_pos_y,int display_char_size,uint16_t color, uint16_t color_bkg = SSD1306_BLACK);
    static void println(const char* message,int display_pos_x, int display_pos_y,int display_char_size,uint16_t color, uint16_t color_bkg = SSD1306_BLACK);
    // Funzione di debounce per i pulsanti
    // Attende che il pulsante sia rilasciato prima di continuare
    static void debounce(int waitTime);
    //Funzione per il debounce dei pulsanti
    // Attende che il pulsante sia rilasciato prima di continuare
    static void debounceButton(int buttonPin, int waitTime);
    // Funzione per calcolare lo spazio necessario per una stringa
    // str è la stringa da calcolare
    // textSize è la dimensione del testo (1, 2, 3, 4, 5)
    // displayWidth è la larghezza del display in pixel
    static int getSpacing(const char* str, int textSize);
};
