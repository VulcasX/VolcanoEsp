#ifndef LITTLEFS_WEB_H
#define LITTLEFS_WEB_H

#include <Arduino.h>
#include <WiFi.h>

// Funzione principale del menu server web
void webServerMenu();

// Funzioni di supporto
void initFileWebServer();
void stopFileWebServer();
void configureServer();
void saveServerConfig();
void loadServerConfig();
String getServerAddress();

#endif