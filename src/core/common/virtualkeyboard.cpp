#include "virtualkeyboard.h"
#include "core/common/common.h"
#include "input.h"
#include "core/config/config.h"

const int keyboardRows = 4;
const int keyboardCols = 12;
char keys[4][12][2] = {
  { {'1','!'}, {'2','@'}, {'3','#'}, {'4','$'}, {'5','%'}, {'6','^'}, {'7','&'}, {'8','*'}, {'9','('}, {'0',')'}, {'-','_'}, {'=','+'} },
  { {'q','Q'}, {'w','W'}, {'e','E'}, {'r','R'}, {'t','T'}, {'y','Y'}, {'u','U'}, {'i','I'}, {'o','O'}, {'p','P'}, {'[','{'}, {']','}'} },
  { {'a','A'}, {'s','S'}, {'d','D'}, {'f','F'}, {'g','G'}, {'h','H'}, {'j','J'}, {'k','K'}, {'l','L'}, {';',':'}, {'"','\''}, {'|','\\'} },
  { {'\\','|'}, {'z','Z'}, {'x','X'}, {'c','C'}, {'v','V'}, {'b','B'}, {'n','N'}, {'m','M'}, {',','<'}, {'.','>'}, {'?','/'}, {'/','/'} }
};
//funzioni per la tastiera virtuale
// Utilizza un display OLED per mostrare i tasti e l'input corrente
// La tastiera è composta da 12 colonne e 4 righe
// I tasti sono definiti in un array bidimensionale
// La funzione getKeyboardInput restituisce l'input dell'utente
//mytext è il testo iniziale, maxSize è la dimensione massima dell'input, msg è il messaggio da visualizzare
String getKeyboardInput(String mytext, int maxSize, String msg) {
  String result = mytext;
  int row = 0, col = 0;
  bool shift = false;
  bool blink = false;
  unsigned long lastBlink = millis();

  #if DISPLAY_PRESENT == 1
  keyboardDisplay(msg, result, row, col, shift);
  while (true) {
    /*
    if ((millis() - lastBlink > 500) && result.length() < maxSize) {
      blink = !blink;
      lastBlink = millis();
    }
    
    if (blink && result.length() < maxSize) {
      display.print("_");
    }
  */

    if (digitalRead(buttonPin_UP) == LOW) {
      row = (row - 1 + keyboardRows) % keyboardRows;
      keyboardDisplay(msg, result, row, col, shift);
      /*
      display.setCursor(64, 0);
      display.print("UP");
      display.display();
      */
      delay(150);
    }
    if (digitalRead(buttonPin_DWN) == LOW) {
      row = (row + 1) % keyboardRows;
      keyboardDisplay(msg, result, row, col, shift);
      /*
      display.setCursor(64, 0);
      display.print("DOWN");
      display.display();
      */
      delay(150);
    }
    if (digitalRead(buttonPin_LFT) == LOW) {
       col = (col - 1 + keyboardCols) % keyboardCols;
      keyboardDisplay(msg, result, row, col, shift);
      /*
      display.setCursor(64, 0);
      display.print("LEFT");
      display.display();
      */
      delay(150);
    }
    if (digitalRead(buttonPin_RHT) == LOW) {
      col = (col + 1) % keyboardCols;
      keyboardDisplay(msg, result, row, col, shift);
      /*
      display.setCursor(64, 0);
      display.print("RIGHT");
      display.display();
      */
      delay(150);
    } else if (digitalRead(buttonPin_SET) == LOW) {
      char ch = keys[row][col][shift ? 1 : 0];
      if (ch == '/') {
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(0, 0);
        display.print(result);
        display.display();
        delay(3000);
        return result;
      } else if (ch == '\\') {
        return "";
      } else if (ch == '|') {
        shift = !shift;
      } else if (ch == '-') {
        if (result.length() > 0) result.remove(result.length() - 1);
      } else {
        if (result.length() < maxSize) result += ch;
      }
      delay(150);
    } else if (digitalRead(buttonPin_RST) == LOW) {
      return "";
    } 
  }
#endif
}

// Funzione per visualizzare la tastiera sul display
// msg è il messaggio da visualizzare, result è l'input corrente, row e
// col sono le coordinate del cursore, shift indica se il tasto shift è attivo
// La funzione aggiorna il display con i tasti e l'input corrente
//shift è un booleano che indica se il tasto shift è attivo
// row e col sono le coordinate del cursore nella tastiera
void keyboardDisplay(String msg, String result, int row, int col, bool shift){
      // Assicurati che il display sia inizializzato
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      // Riga superiore: messaggio + input
      display.setCursor(0, 0);
      display.print(msg.c_str());
      // da cambiare
      display.setCursor(0, 10);
      display.print(result);
      //
      // Tastiera 12 colonne x 4 righe (da y = 24)
      // Disegna la tastiera
      for (int r = 0; r < keyboardRows; r++) {
        for (int c = 0; c < keyboardCols; c++) {
          int x = c * 10;
          int y = 24 + r * 10;
          char ch = keys[r][c][shift ? 1 : 0];
            if (r == row && c == col) {
              display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
            } else {
            display.setTextColor(SSD1306_WHITE);
          }
          //
          display.setCursor(x, y);
          display.write(ch);
        }
      }
      display.display();
      display.setCursor(col,row);
      //debug
      /*
    display.setCursor(0,0);
    display.print("x:");
    display.print(col);
    display.print(" y:");
    display.print(row);
    display.display();
    delay(3000);
    */
    }