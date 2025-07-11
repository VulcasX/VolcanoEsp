#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include "input.h" // Include il file di input per i pulsanti
#include "moduli/wifi/scan.h"
#include "core/config/config.h"

void scanAndShowNetworks() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.println("Scanning WiFi...");
  display.display();
  // Assicurati che il WiFi sia inizializzato
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(500);
  display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.println("SCANSIONE EFFETTUATA");
    Serial.println("Scansione WiFi completata");
    display.display();
  if (n == 0) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Nessuna rete trovata");
    Serial.println("Nessuna rete trovata");
    display.display();
    delay(3000); // Mostra il messaggio per 3 secondi
  } else {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.println("Reti trovate:");
    Serial.print(n);
    Serial.println(" reti trovate");
    display.display();

    for (int i = 0; i < n; i++) {
      display.setCursor(0, i * 10 + 10); // Sposta il cursore per ogni rete trovata
      display.print(i + 1);
      display.print(": ");
      Serial.print(": ");
      display.println(WiFi.SSID(i));
      display.print("RSSI: ");
      Serial.print("RSSI: ");
      display.println(WiFi.RSSI(i));
      display.display();
      }
      delay(5000); // Mostra le reti trovate per 5 secondi
    // Attendi input RST per uscire o tempo
    unsigned long t0 = millis();
    while (millis() - t0 < 5000) { // Attendi 5 secondi
        if (digitalRead(buttonPin_RST) == LOW) return;
        delay(10);
    }
  }
}