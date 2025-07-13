#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <icons.h> // contiene le icone 32x32
#include <input.h>
#include "core/menu.h"
#include "core/menu/WifiMenu.h"
#include "core/menu/rfidmenu.h" // Include il menu RFID
#include "core/config/config.h"
#include "core/littlefs/littlefsweb.h" // Include il file system LittleFS
#include "core/littlefs/littlefs.h"

#define ICON_WIDTH  32
#define ICON_HEIGHT 32
#define ICON_SPACING 6
#define ICON_VISIBLE_COUNT 3

// Array delle icone
const unsigned char* iconList[] = {
  epd_bitmap_wifi_icon_nofocus, epd_bitmap_ble_icon_nofocus, epd_bitmap_nfc_icon_nofocus,
  epd_bitmap_irda_icon_nofocus, epd_bitmap_littlefs_nofocus, epd_bitmap_webserver_nofocus
};

const int iconCount = sizeof(iconList) / sizeof(iconList[0]);

int selectedIndex = 0;

void drawIconMenu() {
  display.clearDisplay();

  int start = selectedIndex;
  if (start > iconCount - ICON_VISIBLE_COUNT) {
    start = iconCount - ICON_VISIBLE_COUNT;
  }
  if (start < 0) start = 0;

  for (int i = 0; i < ICON_VISIBLE_COUNT && (start + i) < iconCount; i++) {
    int x = i * (ICON_WIDTH + ICON_SPACING);
    const unsigned char* icon = iconList[start + i];

    display.drawBitmap(x, 16, icon, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);

    // Evidenziazione
    if ((start + i) == selectedIndex) {
      display.drawRect(x - 2, 14, ICON_WIDTH + 4, ICON_HEIGHT + 4, SSD1306_WHITE);
    }
  }

  display.display();
}

void menuLoop() {
  //disegna le icone
  drawIconMenu();
  while (true) {
    if (digitalRead(buttonPin_RHT) == LOW) {
      selectedIndex++;
      if (selectedIndex >= iconCount) selectedIndex = 0;
      drawIconMenu();
      delay(200);
      while (digitalRead(buttonPin_RHT) == LOW);
    }

    if (digitalRead(buttonPin_LFT) == LOW) {
      selectedIndex--;
      if (selectedIndex < 0) selectedIndex = iconCount - 1;
      drawIconMenu();
      delay(200);
      while (digitalRead(buttonPin_LFT) == LOW);
    }

    if (digitalRead(buttonPin_SET) == LOW) {
      // Azione: selezione dell’icona
      display.clearDisplay();
      display.setCursor(10, 30);
      display.setTextSize(1);
      display.print("Selected: ");
      display.println(selectedIndex);
      display.display();
      if (selectedIndex == 0) {
        selectedIndex = 0;  // reset menu principale
        wifiMenu(); // Entriamo nel sottomenu Wi-Fi
    }else if (selectedIndex == 1) {
        selectedIndex = 0;  // reset menu principale
        // Qui puoi aggiungere il codice per il sottomenu BLE
        display.println("BLE Menu Placeholder");
        display.display();
      } else if (selectedIndex == 2) {
        selectedIndex = 0;  // reset menu principale
        // Qui puoi aggiungere il codice per il sottomenu NFC
        rfidMenu(); // Entriamo nel sottomenu RFID
      } else if (selectedIndex == 3) {
        selectedIndex = 0;  // reset menu principale
        // Qui puoi aggiungere il codice per il sottomenu IRDA
        display.println("IRDA Menu Placeholder");
        display.display();
      }
      else if (selectedIndex == 4) {
        selectedIndex = 0;  // reset menu principale
        // Qui puoi aggiungere il codice per il sottomenu LittleFS
        fileManagerMenu(); // Entriamo nel sottomenu LittleFS
      } else if (selectedIndex == 5) {
        selectedIndex = 0;  // reset menu principale
        webServerMenu(); // Entriamo nel sottomenu Web Server
      }
      delay(500);
      drawIconMenu();
      while (digitalRead(buttonPin_MID) == LOW);
    }
  }
}