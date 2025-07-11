//per ora vengono programmati poi saranno letti da un file di configurazione
#pragma once
// Include guard to prevent multiple inclusions of this header file
//Abilita il debug e il display
// Se DEBUG è a 1, abilita il debug
// Se DISPLAY_PRESENT è a 1, abilita il display
// Se DISPLAY_PRESENT è a 1, include la libreria Adafruit_SSD1306
// Definisce le dimensioni dello schermo
// Se DISPLAY_PRESENT è a 1, definisce le dimensioni dello schermo
// Include la libreria Adafruit_SSD1306 per il display OLED
#define DEBUG 1
#define DISPLAY_PRESENT 1
// Define the display type
#if defined(DISPLAY_PRESENT) && DISPLAY_PRESENT == 1 
    #include <Adafruit_SSD1306.h>
    // Define the screen dimensions
    // These values are for a 128x64 OLED display
    #define SCREEN_WIDTH 128
    #define SCREEN_HEIGHT 64
    extern Adafruit_SSD1306 display;  // ✅ solo dichiarazione
#endif

#define SPEEDBOOT 1
