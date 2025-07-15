  
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "rfidmenu.h"
#include "input.h" // Include il file di input per i pulsanti
#include "core/config/config.h"
#include "moduli/rfid/rfid.h" // Include il file per la gestione RFID
#include "core/common/common.h" // Include il file comune per le funzioni condivise
#include "moduli/rfid/rfid_advanced_menu.h" // Include il menu avanzato RFID


const char* rfidItems[] = {
  "Scan Card",
  "Dump Card",
  "Write Card",
  "RFID Tools",
  "Back"
};

const int rfidItemsCount = sizeof(rfidItems) / sizeof(rfidItems[0]);
int selectedrfid = 0; 






void rfidMenu() {
  // Attendi che il pulsante SET sia rilasciato prima di mostrare il menu
  while (digitalRead(buttonPin_SET) == LOW) {
    delay(10);
  }


// Mostra il menu aggiornato
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("RFID Menu");

    for (int i = 0; i < rfidItemsCount; i++) {
      display.setCursor(10, 16 + i * 10);
      if (i == selectedrfid)
        display.print("> ");
      else
        display.print("  ");
      display.println(rfidItems[i]);
    }
    display.display();

    delay(30); // evita flickering



  while (true) {
    // 🔼 Navigazione UP
    if (digitalRead(buttonPin_UP) == LOW) {
      selectedrfid--;
      if (selectedrfid < 0) selectedrfid = rfidItemsCount - 1;
      delay(150);
    }

    // 🔽 Navigazione DOWN
    if (digitalRead(buttonPin_DWN) == LOW) {
      selectedrfid = (selectedrfid + 1) % rfidItemsCount;
      delay(150);
    }

    // ✅ Selezione SET
    if (digitalRead(buttonPin_SET) == LOW) {
      delay(150);
      common::print(rfidItems[selectedrfid], 0, 0, 1, SSD1306_WHITE);
      // Controlla quale voce è selezionata
      // Se la voce è "Back", torna al menu principale
      //if (strcmp(rfidItems[selectedrfid], "Back") == 0) {
       if (selectedrfid == 4){
         selectedrfid = 0;  // 🟢 resetta l’indice per il prossimo ingresso
         return;
      } //else if (strcmp(rfidItems[selectedrfid], "Scan card") == 0) {
      // Se la voce è "Scan Card", chiama la funzione rfid()
      else if (selectedrfid == 0) {
        //leggi rfid
        // rfid(); // Chiama la funzione per gestire la scansione RFID
        rfid(); // Chiama la funzione per gestire il dump RFID
      } else if (selectedrfid == 1) {
        // Dump RFID
        dump();
      } else if (selectedrfid == 3) {
        // RFID Tools avanzati (MFCUK e MFOC)
        rfid_advanced_menu();
      } else {
        // Placeholder per altre voci
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.print("Selezionato: ");
        display.println(rfidItems[selectedrfid]);
        display.display();
        delay(1000);
      }
    }

    // 🔙 Ritorno con RST
    if (digitalRead(buttonPin_RST) == LOW) {
      delay(150);
      return;
    }

    // Mostra il menu aggiornato
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("RFID Menu");

    for (int i = 0; i < rfidItemsCount; i++) {
      display.setCursor(10, 16 + i * 10);
      if (i == selectedrfid)
        display.print("> ");
      else
        display.print("  ");
      display.println(rfidItems[i]);
    }
    display.display();

    delay(30); // evita flickering
  }
}

