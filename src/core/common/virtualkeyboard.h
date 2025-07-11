#pragma once
#include <Arduino.h>
#include <WString.h>
//funzioni per la tastiera virtuale
// Utilizza un display OLED per mostrare i tasti e l'input corrente
// La tastiera è composta da 12 colonne e 4 righe
// I tasti sono definiti in un array bidimensionale
// La funzione getKeyboardInput restituisce l'input dell'utente
//mytext è il testo iniziale, maxSize è la dimensione massima dell'input, msg è il messaggio da visualizzare
String getKeyboardInput(String mytext, int maxSize, String msg);
// Funzione per visualizzare la tastiera sul display
// msg è il messaggio da visualizzare, result è l'input corrente, row e
// col sono le coordinate del cursore, shift indica se il tasto shift è attivo
// La funzione aggiorna il display con i tasti e l'input corrente
//shift è un booleano che indica se il tasto shift è attivo
// row e col sono le coordinate del cursore nella tastiera
void keyboardDisplay(String msg, String result, int row, int col, bool shift);