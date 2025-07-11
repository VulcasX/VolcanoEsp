#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include "WifiMenu.h"
#include "input.h" // Include il file di input per i pulsanti
#include "moduli/wifi/scan.h" // Include il file per la scansione delle reti Wi-Fi
#include "core/config/config.h"



const char* wifiItems[] = {
  "Scan Network",
  "Connect to AP",
  "Disconnect",
  "Back"
};

const int wifiItemsCount = sizeof(wifiItems) / sizeof(wifiItems[0]);
int selectedWifi = 0;

void wifiMenu() {
  while (true) {
    // 🔼 Navigazione UP
    if (digitalRead(buttonPin_UP) == LOW) {
      selectedWifi--;
      if (selectedWifi < 0) selectedWifi = wifiItemsCount - 1;
      delay(150);
    }

    // 🔽 Navigazione DOWN
    if (digitalRead(buttonPin_DWN) == LOW) {
      selectedWifi = (selectedWifi + 1) % wifiItemsCount;
      delay(150);
    }

    // ✅ Selezione SET
    if (digitalRead(buttonPin_SET) == LOW) {
      delay(150);
      // Controlla quale voce è selezionata
      if (strcmp(wifiItems[selectedWifi], "Back") == 0) {
        selectedWifi = 0;  // 🟢 resetta l’indice per il prossimo ingresso
        return;
      } else if (strcmp(wifiItems[selectedWifi], "Scan Network") == 0) {
        scanAndShowNetworks();
      } else {
        // Placeholder per altre voci
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.print("Selezionato: ");
        display.println(wifiItems[selectedWifi]);
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
    display.println("Wi-Fi Menu");

    for (int i = 0; i < wifiItemsCount; i++) {
      display.setCursor(10, 16 + i * 10);
      if (i == selectedWifi)
        display.print("> ");
      else
        display.print("  ");
      display.println(wifiItems[i]);
    }
    display.display();

    delay(30); // evita flickering
  }
}