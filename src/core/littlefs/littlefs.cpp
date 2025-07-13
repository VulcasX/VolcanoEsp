#include "core/config/config.h"

// Calcolo area testo e barra di scorrimento
#define CHAR_W 5
#define CHAR_H 7
#define CHAR_SPACING 1
#define LINE_SPACING 1
#define BAR_WIDTH ((int)(SCREEN_WIDTH * 0.05))
#define HBAR_HEIGHT 8
#define TEXT_ORIGIN_X 0
#define TEXT_ORIGIN_Y 0
#define TEXT_AREA_W (SCREEN_WIDTH - BAR_WIDTH)
#define TEXT_AREA_H (SCREEN_HEIGHT - HBAR_HEIGHT)
#define CHARS_PER_LINE (TEXT_AREA_W / (CHAR_W + CHAR_SPACING))
#define LINES_ON_SCREEN (TEXT_AREA_H / (CHAR_H + LINE_SPACING))
#include <Adafruit_SSD1306.h>
#include <LittleFS.h> // Include LittleFS per la gestione dei file
#include "input.h" // Include il file di input per i pulsanti
#include "core/config/config.h" // Include il file di configurazione
#include "core/common/common.h" // Include il file comune per le funzioni condivise
#include "core/common/virtualkeyboard.h" // Include il file per la gestione della tastiera
// PIN ESP32 I2C

// Definisci i pin dei pulsanti se non già definiti
// #define buttonPin_UP ...
// #define buttonPin_DWN ...
// #define buttonPin_SET ...
// #define buttonPin_RST ...

#define MAX_FILES 32
#define MAX_FILENAME 32

struct FileEntry {
    char name[MAX_FILENAME];
    bool isDir;
};

FileEntry fileList[MAX_FILES];
int fileCount = 0;

void listFiles(const char* path = "/") {
    fileCount = 0;
    File root = LittleFS.open(path);
    File file = root.openNextFile();
    while (file && fileCount < MAX_FILES) {
        strncpy(fileList[fileCount].name, file.name(), MAX_FILENAME - 1);
        fileList[fileCount].name[MAX_FILENAME - 1] = 0;
        fileList[fileCount].isDir = file.isDirectory();
        fileCount++;
        file = root.openNextFile();
    }
}

void showTxtFile(const char* filename) {
    File file = LittleFS.open(filename, "r");
    if (!file) {
        common::println("Errore apertura file", 0, 0, 1, SSD1306_WHITE);
        delay(2000);
        return;
    }
    // Bufferizzazione e suddivisione in righe
    #define MAX_LINES 256
    String lines[MAX_LINES];
    int lineCount = 0;
    String currentLine = "";
    while (file.available()) {
        char c = file.read();
        if (c == '\r') continue; // ignora CR
        if (c == '\n') {
            int start = 0;
            while (start < currentLine.length() && lineCount < MAX_LINES) {
                lines[lineCount++] = currentLine.substring(start, start + CHARS_PER_LINE);
                start += CHARS_PER_LINE;
            }
            currentLine = "";
        } else {
            currentLine += c;
        }
    }
    // aggiungi l'ultima riga se presente
    if (currentLine.length() > 0 && lineCount < MAX_LINES) {
        int start = 0;
        while (start < currentLine.length() && lineCount < MAX_LINES) {
            lines[lineCount++] = currentLine.substring(start, start + CHARS_PER_LINE);
            start += CHARS_PER_LINE;
        }
    }
    file.close();
    int scroll = 0;
    int hScroll = 0; // scroll orizzontale
    bool redraw = true;
    int maxLineLen = 0;
    for (int i = scroll; i < scroll + LINES_ON_SCREEN && i < lineCount; i++) {
        if (lines[i].length() > maxLineLen) maxLineLen = lines[i].length();
    }
    int maxHScroll = (maxLineLen > CHARS_PER_LINE) ? (maxLineLen - CHARS_PER_LINE) : 0;
    while (true) {
        // Gestione robusta RST: esci solo al rilascio
        if (digitalRead(buttonPin_RST) == LOW) {
            while (digitalRead(buttonPin_RST) == LOW) delay(10);
            common::debounceButton(buttonPin_RST, 50);
            break;
        }
        if (redraw) {
            display.clearDisplay();
            // Testo
            for (int i = 0; i < LINES_ON_SCREEN; i++) {
                if (scroll + i < lineCount) {
                    display.setCursor(0, i * (CHAR_H + LINE_SPACING));
                    display.setTextColor(SSD1306_WHITE);
                    display.setTextSize(1);
                    String toShow = lines[scroll + i];
                    if (hScroll < toShow.length())
                        toShow = toShow.substring(hScroll);
                    else
                        toShow = "";
                    if (toShow.length() > CHARS_PER_LINE)
                        toShow = toShow.substring(0, CHARS_PER_LINE);
                    display.write(toShow.c_str());
                }
            }
            // Barra di scorrimento verticale (a destra)
            int barX = SCREEN_WIDTH - BAR_WIDTH;
            int barY = 0;
            int barH = SCREEN_HEIGHT - HBAR_HEIGHT;
            int barW = BAR_WIDTH;
            display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
            if (lineCount > LINES_ON_SCREEN) {
                int scrollH = (LINES_ON_SCREEN * barH) / lineCount;
                if (scrollH < 8) scrollH = 8;
                int scrollY = (scroll * (barH - scrollH)) / (lineCount - LINES_ON_SCREEN);
                display.fillRect(barX + 1, barY + 1 + scrollY, barW - 2, scrollH - 2, SSD1306_WHITE);
            }
            // Barra di scorrimento orizzontale (in basso)
            int hBarY = SCREEN_HEIGHT - HBAR_HEIGHT;
            int hBarX = BAR_WIDTH;
            int hBarW = SCREEN_WIDTH - BAR_WIDTH;
            int hBarH = HBAR_HEIGHT;
            display.drawRect(hBarX, hBarY, hBarW, hBarH, SSD1306_WHITE);
            if (maxLineLen > CHARS_PER_LINE) {
                int hScrollW = (CHARS_PER_LINE * hBarW) / maxLineLen;
                if (hScrollW < 8) hScrollW = 8;
                int hScrollX = (hScroll * (hBarW - hScrollW)) / (maxLineLen - CHARS_PER_LINE);
                display.fillRect(hBarX + 1 + hScrollX, hBarY + 1, hScrollW - 2, hBarH - 2, SSD1306_WHITE);
            } else {
                display.fillRect(hBarX + 1, hBarY + 1, hBarW - 2, hBarH - 2, SSD1306_WHITE);
            }
            display.display();
            redraw = false;
        }
        if (digitalRead(buttonPin_UP) == LOW && scroll > 0) {
            scroll--;
            redraw = true;
            delay(150);
        }
        if (digitalRead(buttonPin_DWN) == LOW && scroll + LINES_ON_SCREEN < lineCount) {
            scroll++;
            redraw = true;
            delay(150);
        }
        // Scroll orizzontale con LEFT/RIGHT
        if (digitalRead(buttonPin_LFT) == LOW && hScroll > 0) {
            hScroll--;
            redraw = true;
            delay(150);
        }
        if (digitalRead(buttonPin_RHT) == LOW && hScroll < maxHScroll) {
            hScroll++;
            redraw = true;
            delay(150);
        }
        delay(10);
    }
}

void showMfdFile(const char* filename) {
    File file = LittleFS.open(filename, "r");
    if (!file) {
        common::println("Errore apertura file", 0, 0, 1, SSD1306_WHITE);
        delay(2000);
        return;
    }
    // Bufferizza e formatta le righe come per showTxtFile
    #define MAX_MFD_LINES 128
    String lines[MAX_MFD_LINES];
    int lineCount = 0;
    uint8_t data[16];
    int block = 0, sector = 0;
    while (file.read(data, 16) == 16 && lineCount < MAX_MFD_LINES) {
        String line = "S";
        if (sector < 10) line += "0";
        line += String(sector);
        line += " B";
        if (block < 10) line += "0";
        line += String(block);
        line += ": ";
        for (int i = 0; i < 16; i++) {
            char hex[4];
            sprintf(hex, "%02X ", data[i]);
            line += hex;
        }
        // Suddividi la riga lunga in più righe se necessario
        int start = 0;
        while (start < line.length() && lineCount < MAX_MFD_LINES) {
            lines[lineCount++] = line.substring(start, start + CHARS_PER_LINE);
            start += CHARS_PER_LINE;
        }
        block++;
        if (block == 4) { block = 0; sector++; }
    }
    file.close();
    int scroll = 0;
    int hScroll = 0;
    bool redraw = true;
    int maxLineLen = 0;
    for (int i = scroll; i < scroll + LINES_ON_SCREEN && i < lineCount; i++) {
        if (lines[i].length() > maxLineLen) maxLineLen = lines[i].length();
    }
    int maxHScroll = (maxLineLen > CHARS_PER_LINE) ? (maxLineLen - CHARS_PER_LINE) : 0;
    while (true) {
        if (digitalRead(buttonPin_RST) == LOW) {
            while (digitalRead(buttonPin_RST) == LOW) delay(10);
            common::debounceButton(buttonPin_RST, 50);
            break;
        }
        if (redraw) {
            display.clearDisplay();
            for (int i = 0; i < LINES_ON_SCREEN; i++) {
                int idx = scroll + i;
                if (idx < lineCount) {
                    display.setCursor(0, i * (CHAR_H + LINE_SPACING));
                    display.setTextColor(SSD1306_WHITE);
                    display.setTextSize(1);
                    String toShow = lines[idx];
                    if (hScroll < toShow.length())
                        toShow = toShow.substring(hScroll);
                    else
                        toShow = "";
                    if (toShow.length() > CHARS_PER_LINE)
                        toShow = toShow.substring(0, CHARS_PER_LINE);
                    display.write(toShow.c_str());
                }
            }
            // Barra di scorrimento verticale (a destra)
            int barX = SCREEN_WIDTH - BAR_WIDTH;
            int barY = 0;
            int barH = SCREEN_HEIGHT - HBAR_HEIGHT;
            int barW = BAR_WIDTH;
            display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
            if (lineCount > LINES_ON_SCREEN) {
                int scrollH = (LINES_ON_SCREEN * barH) / lineCount;
                if (scrollH < 8) scrollH = 8;
                int scrollY = (scroll * (barH - scrollH)) / (lineCount - LINES_ON_SCREEN);
                display.fillRect(barX + 1, barY + 1 + scrollY, barW - 2, scrollH - 2, SSD1306_WHITE);
            }
            // Barra di scorrimento orizzontale (in basso)
            int hBarY = SCREEN_HEIGHT - HBAR_HEIGHT;
            int hBarX = BAR_WIDTH;
            int hBarW = SCREEN_WIDTH - BAR_WIDTH;
            int hBarH = HBAR_HEIGHT;
            display.drawRect(hBarX, hBarY, hBarW, hBarH, SSD1306_WHITE);
            if (maxLineLen > CHARS_PER_LINE) {
                int hScrollW = (CHARS_PER_LINE * hBarW) / maxLineLen;
                if (hScrollW < 8) hScrollW = 8;
                int hScrollX = (hScroll * (hBarW - hScrollW)) / (maxLineLen - CHARS_PER_LINE);
                display.fillRect(hBarX + 1 + hScrollX, hBarY + 1, hScrollW - 2, hBarH - 2, SSD1306_WHITE);
            } else {
                display.fillRect(hBarX + 1, hBarY + 1, hBarW - 2, hBarH - 2, SSD1306_WHITE);
            }
            display.display();
            redraw = false;
        }
        if (digitalRead(buttonPin_UP) == LOW && scroll > 0) {
            scroll--;
            redraw = true;
            delay(150);
        }
        if (digitalRead(buttonPin_DWN) == LOW && scroll + LINES_ON_SCREEN < lineCount) {
            scroll++;
            redraw = true;
            delay(150);
        }
        if (digitalRead(buttonPin_LFT) == LOW && hScroll > 0) {
            hScroll--;
            redraw = true;
            delay(150);
        }
        if (digitalRead(buttonPin_RHT) == LOW && hScroll < maxHScroll) {
            hScroll++;
            redraw = true;
            delay(150);
        }
        delay(10);
    }
}

// Operazioni base (elimina, copia, incolla, taglia, rinomina, crea cartella)
void fileContextMenu(const char* filename) {
    const char* ops[] = {"Elimina", "Copia", "Taglia", "Incolla", "Rinomina", "Crea Cartella", "Indietro"};
    int opCount = 7, selected = 0, scroll = 0;
    static String clipboard = "";
    static bool isCut = false;
    bool redraw = true;
    while (true) {
        int visibleOps = LINES_ON_SCREEN - 2; // -1 per intestazione, -1 per barra orizzontale
        if (selected < scroll) scroll = selected;
        if (selected > scroll + visibleOps - 1) scroll = selected - visibleOps + 1;
        if (redraw) {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.setTextSize(1);
            display.write("Operazioni:");
            for (int i = 0; i < visibleOps; i++) {
                int idx = scroll + i;
                if (idx < opCount) {
                    if (idx == selected)
                        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
                    else
                        display.setTextColor(SSD1306_WHITE);
                    display.setCursor(0, 10 + i * 10);
                    display.setTextSize(1);
                    display.write(ops[idx]);
                }
            }
            // Barra di scroll verticale
            int barX = SCREEN_WIDTH - BAR_WIDTH;
            int barY = 0;
            int barH = TEXT_AREA_H;
            int barW = BAR_WIDTH;
            display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
            if (opCount > visibleOps) {
                int scrollH = (visibleOps * barH) / opCount;
                if (scrollH < 8) scrollH = 8;
                int scrollY = (scroll * (barH - scrollH)) / (opCount - visibleOps);
                display.fillRect(barX + 1, barY + 1 + scrollY, barW - 2, scrollH - 2, SSD1306_WHITE);
            }
            display.setTextColor(SSD1306_WHITE);
            display.display();
            redraw = false;
        }
        if (digitalRead(buttonPin_UP) == LOW && selected > 0) {
            selected--;
            redraw = true;
            delay(150);
        }
        if (digitalRead(buttonPin_DWN) == LOW && selected < opCount - 1) {
            selected++;
            redraw = true;
            delay(150);
        }
        if (digitalRead(buttonPin_SET) == LOW) {
            delay(200);
            if (selected == 0) { // Elimina
                LittleFS.remove(filename);
                common::println("File eliminato", 0, 56, 1, SSD1306_WHITE);
                display.display();
                delay(1000);
                break;
            }
            if (selected == 1) { // Copia
                clipboard = String(filename);
                isCut = false;
                common::println("Copia pronta", 0, 56, 1, SSD1306_WHITE);
                display.display();
                delay(1000);
            }
            if (selected == 2) { // Taglia
                clipboard = String(filename);
                isCut = true;
                common::println("Taglia pronto", 0, 56, 1, SSD1306_WHITE);
                display.display();
                delay(1000);
            }
            if (selected == 3) { // Incolla
                if (clipboard.length() > 0) {
                    String dest = clipboard.substring(clipboard.lastIndexOf("/") + 1);
                    String base = dest;
                    int copyIdx = 1;
                    // Controllo duplicati
                    while (LittleFS.exists( ("/" + dest).c_str() )) {
                        // Chiedi se vuoi sovrascrivere o rinominare
                        String msg = "Sovrascrivi? S/N";
                        String result = getKeyboardInput("", 16, msg);
                        if (result == "S" || result == "s") {
                            break;
                        } else {
                            // Proponi nome incrementale
                            dest = base;
                            int dot = base.lastIndexOf('.');
                            if (dot > 0) {
                                dest = base.substring(0, dot) + "(" + String(copyIdx) + ")" + base.substring(dot);
                            } else {
                                dest = base + "(" + String(copyIdx) + ")";
                            }
                            copyIdx++;
                            msg = "Nuovo nome:";
                            result = getKeyboardInput(dest, 32, msg);
                            dest = result;
                        }
                    }
                    File srcFile = LittleFS.open(clipboard.c_str(), "r");
                    File dstFile = LittleFS.open( ("/" + dest).c_str(), "w");
                    if (srcFile && dstFile) {
                        while (srcFile.available()) dstFile.write(srcFile.read());
                        srcFile.close();
                        dstFile.close();
                        if (isCut) LittleFS.remove(clipboard.c_str());
                        common::println("Incollato!", 0, 56, 1, SSD1306_WHITE);
                    } else {
                        common::println("Errore incolla", 0, 56, 1, SSD1306_WHITE);
                    }
                    display.display();
                    delay(1000);
                }
            }
            if (selected == 4) { // Rinomina
                String msg = "Nuovo nome:";
                String result = getKeyboardInput(String(filename).substring( String(filename).lastIndexOf("/") + 1 ), 32, msg);
                if (result.length() > 0) {
                    if (LittleFS.exists( ("/" + result).c_str() )) {
                        // File già esistente, chiedi azione
                        String msg2 = "Sovrascrivi? S/N";
                        String res2 = getKeyboardInput("", 16, msg2);
                        if (!(res2 == "S" || res2 == "s")) {
                            // Proponi nome incrementale
                            String base = result;
                            int copyIdx = 1;
                            int dot = base.lastIndexOf('.');
                            if (dot > 0) {
                                result = base.substring(0, dot) + "(" + String(copyIdx) + ")" + base.substring(dot);
                            } else {
                                result = base + "(" + String(copyIdx) + ")";
                            }
                            result = getKeyboardInput(result, 32, "Nuovo nome:");
                        }
                    }
                    if (LittleFS.rename(filename, ("/" + result).c_str())) {
                        common::println("Rinominato!", 0, 56, 1, SSD1306_WHITE);
                    } else {
                        common::println("Errore rinomina", 0, 56, 1, SSD1306_WHITE);
                    }
                    display.display();
                    delay(1000);
                    break;
                }
            }
            if (selected == 5) { // Crea Cartella
                String msg = "Nome cartella:";
                String result = getKeyboardInput("", 32, msg);
                if (result.length() > 0) {
                    if (LittleFS.mkdir(result.c_str())) {
                        common::println("Cartella creata!", 0, 56, 1, SSD1306_WHITE);
                    } else {
                        common::println("Errore creazione", 0, 56, 1, SSD1306_WHITE);
                    }
                    display.display();
                    delay(1000);
                    break;
                }
            }
            if (selected == opCount - 1) break; // Indietro
        }
        if (digitalRead(buttonPin_RST) == LOW) {
            delay(200);
            break;
        }
        delay(10);
    }
}

void fileManagerMenu() {
    common::debounce(50);
    int selected = 0, scroll = 0;
    while (true) {
        listFiles("/");
        bool redraw = true;
        while (true) {
            if (redraw) {
                display.clearDisplay();
                common::println("File Manager", 0, 0, 1, SSD1306_WHITE);
                int visibleFiles = LINES_ON_SCREEN - 2; // -1 per intestazione, -1 per barra orizzontale
                if (selected < scroll) scroll = selected;
                if (selected > scroll + visibleFiles - 1) scroll = selected - visibleFiles + 1;
                for (int i = 0; i < visibleFiles; i++) {
                    int idx = scroll + i;
                    if (idx < fileCount) {
                        if (idx == selected)
                            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
                        else
                            display.setTextColor(SSD1306_WHITE);
                        display.setCursor(0, 10 + i * 10);
                        display.setTextSize(1);
                        display.write(fileList[idx].name);
                    }
                }
                // Barra di scroll verticale
                int barX = SCREEN_WIDTH - BAR_WIDTH;
                int barY = 0;
                int barH = TEXT_AREA_H;
                int barW = BAR_WIDTH;
                display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
                if (fileCount > visibleFiles) {
                    int scrollH = (visibleFiles * barH) / fileCount;
                    if (scrollH < 8) scrollH = 8;
                    int scrollY = (scroll * (barH - scrollH)) / (fileCount - visibleFiles);
                    display.fillRect(barX + 1, barY + 1 + scrollY, barW - 2, scrollH - 2, SSD1306_WHITE);
                }
                display.setTextColor(SSD1306_WHITE);
                display.display();
                redraw = false;
            }
            // Navigazione
            if (digitalRead(buttonPin_UP) == LOW && selected > 0) {
                selected--;
                if (selected < scroll) scroll--;
                redraw = true;
                delay(150);
            }
            if (digitalRead(buttonPin_DWN) == LOW && selected < fileCount - 1) {
                selected++;
                if (selected > scroll + LINES_ON_SCREEN - 1) scroll++;
                redraw = true;
                delay(150);
            }
            // SET breve: apri file
            if (digitalRead(buttonPin_SET) == LOW) {
                unsigned long t0 = millis();
                while (digitalRead(buttonPin_SET) == LOW) delay(10);
                unsigned long t1 = millis();
                if (t1 - t0 > 2000) {
                    // SET lungo: menu contestuale
                    char fullpath[64];
                    snprintf(fullpath, sizeof(fullpath), "/%s", fileList[selected].name);
                    fileContextMenu(fullpath);
                    redraw = true;
                    break;
                } else {
                    // SET breve: apri file
                    char fullpath[64];
                    snprintf(fullpath, sizeof(fullpath), "/%s", fileList[selected].name);
                    if (strstr(fileList[selected].name, ".txt")) showTxtFile(fullpath);
                    else if (strstr(fileList[selected].name, ".mfd")) showMfdFile(fullpath);
                    redraw = true;
                }
            }
            // RST breve: esci
            if (digitalRead(buttonPin_RST) == LOW) {
                unsigned long t0 = millis();
                while (digitalRead(buttonPin_RST) == LOW) delay(10);
                unsigned long t1 = millis();
                if (t1 - t0 > 2000) {
                    // RST lungo: torna indietro
                    return;
                } else {
                    // RST breve: esci file manager
                    return;
                }
            }
            delay(10);
        }
    }
}