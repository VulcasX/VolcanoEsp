#include "core/common/common.h"
#include "core/littlefs/littlefsweb.h"
#include <input.h>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Adafruit_SSD1306.h>
#include "core/common/virtualkeyboard.h"

extern Adafruit_SSD1306 display;

// Pin definitions
//const int buttonPin_UP = 35;   // GPIO35
//const int buttonPin_DWN = 34;  // GPIO34
//const int buttonPin_SET = 39;  // GPIO39
//const int buttonPin_RST = 36;  // GPIO36

// Definizioni per il display
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define CHAR_W 5
#define CHAR_H 7
#define CHAR_SPACING 1
#define LINE_SPACING 1
#define TEXT_AREA_H (SCREEN_HEIGHT - 10)  // -10 per il titolo
#define LINES_ON_SCREEN ((TEXT_AREA_H) / (CHAR_H + LINE_SPACING))
#define VISIBLE_ITEMS (LINES_ON_SCREEN - 2)  // -1 per il titolo, -1 per margine

// Struttura configurazione server
struct ServerConfig {
    char ssid[32];
    char password[64];
    char encryption[20];
    char serverAddress[16];
    bool isConfigured;
};

ServerConfig serverConfig;
AsyncWebServer server(80);
bool isServerRunning = false;
bool isFileOperationInProgress = false;

// Verifica la validità della configurazione
bool validateServerConfig() {
    // Verifica SSID
    if (strlen(serverConfig.ssid) == 0) {
        return false;
    }

    // Verifica password se richiesta
    if (strcmp(serverConfig.encryption, "Open") != 0) {
        if (strlen(serverConfig.password) < 8) {
            return false;
        }
    }

    // Verifica indirizzo server
    if (strlen(serverConfig.serverAddress) == 0) {
        return false;
    }

    return true;
}

// Funzione per salvare la configurazione
void saveServerConfig() {
    File configFile = LittleFS.open("/littlefsserver.conf", "w");
    if (!configFile) {
        return;
    }
    configFile.write((uint8_t*)&serverConfig, sizeof(ServerConfig));
    configFile.close();
}

// Funzione per caricare la configurazione
void loadServerConfig() {
    File configFile = LittleFS.open("/littlefsserver.conf", "r");
    if (configFile && configFile.size() == sizeof(ServerConfig)) {
        configFile.read((uint8_t*)&serverConfig, sizeof(ServerConfig));
    } else {
        // Configurazione standard di default
        memset(&serverConfig, 0, sizeof(ServerConfig));
        strcpy(serverConfig.ssid, "VolcanoESPwebserver");
        strcpy(serverConfig.password, "12345678");
        strcpy(serverConfig.encryption, "WPA2");
        strcpy(serverConfig.serverAddress, "192.168.4.1");
        serverConfig.isConfigured = true;
        saveServerConfig(); // Salva la configurazione standard
    }
    if (configFile) {
        configFile.close();
    }
}

// Mostra le informazioni di rete
void showNetworkInfo() {
    display.clearDisplay();
    common::println("Info Server Web", 0, 0, 1, SSD1306_WHITE);
    display.setCursor(0, 10);
    display.print("SSID: ");
    display.println(serverConfig.ssid);
    display.setCursor(0, 20);
    display.print("Sicurezza: ");
    display.println(serverConfig.encryption);
    display.setCursor(0, 30);
    display.print("IP: ");
    display.println(WiFi.softAPIP().toString());
    display.setCursor(0, 40);
    if (strcmp(serverConfig.encryption, "Open") != 0) {
        display.print("PSW: ");
        display.println(serverConfig.password);
    }
    display.setCursor(0, 50);
    display.println("RST lungo per uscire");
    display.display();
}

// Pagina HTML principale
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>VolcanoEsp File Manager</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            background-color: #f0f0f0;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .file-list {
            list-style: none;
            padding: 0;
        }
        .file-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 10px;
            border-bottom: 1px solid #eee;
        }
        .file-item:hover {
            background-color: #f5f5f5;
        }
        .actions {
            display: flex;
            gap: 10px;
        }
        button {
            background-color: #4CAF50;
            color: white;
            padding: 8px 16px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
        }
        button:hover {
            background-color: #45a049;
        }
        .delete {
            background-color: #f44336;
        }
        .delete:hover {
            background-color: #da190b;
        }
        .upload-form {
            margin: 20px 0;
            padding: 20px;
            border: 2px dashed #ccc;
            border-radius: 4px;
        }
        .new-folder {
            margin: 20px 0;
        }
        .breadcrumb {
            display: flex;
            align-items: center;
            padding: 10px 0;
            margin-bottom: 20px;
            border-bottom: 1px solid #eee;
        }
        .breadcrumb a {
            color: #4CAF50;
            text-decoration: none;
            padding: 5px;
        }
        .breadcrumb a:hover {
            text-decoration: underline;
        }
        .breadcrumb span {
            padding: 0 5px;
        }
        .file-name {
            cursor: pointer;
            color: #333;
            text-decoration: none;
        }
        .folder {
            color: #4CAF50;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>VolcanoEsp File Manager</h1>
        
        <div id="breadcrumb" class="breadcrumb">
            <!-- Breadcrumb navigation -->
        </div>
        
        <div class="upload-form">
            <h3>Carica File</h3>
            <form id="uploadForm" enctype="multipart/form-data">
                <input type="file" name="file">
                <button type="submit">Carica</button>
            </form>
        </div>

        <div class="new-folder">
            <h3>Crea Cartella</h3>
            <input type="text" id="folderName" placeholder="Nome cartella">
            <button onclick="createFolder()">Crea</button>
        </div>

        <h3>File nel sistema</h3>
        <ul class="file-list" id="fileList">
            <!-- Lista file qui -->
        </ul>
    </div>

    <script>
        let currentPath = '/';

        function updateBreadcrumb() {
            const parts = currentPath.split('/').filter(p => p);
            let html = '<a href="#" onclick="navigateTo(\'/\')">Home</a>';
            let path = '';
            
            parts.forEach(part => {
                path += '/' + part;
                html += '<span>/</span><a href="#" onclick="navigateTo(\'' + path + '\')">' + part + '</a>';
            });
            
            document.getElementById('breadcrumb').innerHTML = html;
        }

        function navigateTo(path) {
            currentPath = path;
            updateBreadcrumb();
            loadFiles();
        }

        function loadFiles() {
            fetch('/list?path=' + encodeURIComponent(currentPath))
                .then(response => response.json())
                .then(files => {
                    const fileList = document.getElementById('fileList');
                    fileList.innerHTML = '';
                    files.forEach(file => {
                        const li = document.createElement('li');
                        li.className = 'file-item';
                        
                        const fileNameClass = file.isDirectory ? 'file-name folder' : 'file-name';
                        const onClick = file.isDirectory ? 
                            `onclick="navigateTo('${file.path}')"` :
                            `onclick="downloadFile('${file.path}')"`;

                        if (file.name === '..') {
                            li.innerHTML = `
                                <a href="#" class="${fileNameClass}" ${onClick}>⬆️ Livello Superiore</a>
                                <div class="actions"></div>
                            `;
                        } else {
                            li.innerHTML = `
                                <a href="#" class="${fileNameClass}" ${onClick}>${file.isDirectory ? '📁 ' : '📄 '}${file.name}</a>
                                <div class="actions">
                                    ${!file.isDirectory ? `<button onclick="downloadFile('${file.path}')">Scarica</button>` : ''}
                                    <button class="delete" onclick="deleteFile('${file.path}')">Elimina</button>
                                </div>
                            `;
                        }
                        fileList.appendChild(li);
                    });
                });
        }

        function downloadFile(path) {
            window.location.href = `/download?file=${encodeURIComponent(path)}`;
        }

        function deleteFile(path) {
            if (confirm('Sei sicuro di voler eliminare questo elemento?')) {
                fetch(`/delete?file=${encodeURIComponent(path)}`, { method: 'DELETE' })
                    .then(response => {
                        if (response.ok) {
                            loadFiles();
                        } else {
                            alert('Errore durante l\'eliminazione');
                        }
                    });
            }
        }

        function createFolder() {
            const folderName = document.getElementById('folderName').value;
            if (!folderName) {
                alert('Inserisci un nome per la cartella');
                return;
            }

            const fullPath = (currentPath === '/' ? '' : currentPath) + '/' + folderName;

            fetch('/createFolder', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ name: fullPath })
            })
            .then(response => {
                if (response.ok) {
                    loadFiles();
                    document.getElementById('folderName').value = '';
                } else {
                    alert('Errore durante la creazione della cartella');
                }
            });
        }

        // Gestione upload file
        document.getElementById('uploadForm').onsubmit = function(e) {
            e.preventDefault();
            const formData = new FormData(this);
            const file = formData.get('file');
            if (!file) {
                alert('Seleziona un file da caricare');
                return;
            }

            // Aggiungi il path corrente al nome del file
            const fullPath = (currentPath === '/' ? '' : currentPath) + '/' + file.name;
            const newFormData = new FormData();
            newFormData.append('file', file, fullPath);

            fetch('/upload', {
                method: 'POST',
                body: newFormData
            })
            .then(response => {
                if (response.ok) {
                    loadFiles();
                    this.reset();
                } else {
                    alert('Errore durante il caricamento del file');
                }
            });
        };

        // Inizializzazione
        updateBreadcrumb();
        loadFiles();
    </script>
</body>
</html>
)rawliteral";

void initFileWebServer() {
    // Prima di iniziare il server, verifichiamo la configurazione
    if (!validateServerConfig()) {
        display.clearDisplay();
        common::println("Config non valida!", 0, 0, 1, SSD1306_WHITE);
        display.display();
        delay(2000);
        return;
    }

    display.clearDisplay();
    common::println("Creazione AP WiFi...", 0, 0, 1, SSD1306_WHITE);
    display.display();

    // Configura l'ESP32 come Access Point
    WiFi.mode(WIFI_AP);
    IPAddress localIP;
    if(strlen(serverConfig.serverAddress) > 0) {
        localIP.fromString(serverConfig.serverAddress);
    } else {
        localIP = IPAddress(192, 168, 4, 1);
    }
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);

    // Configura l'IP statico per l'AP
    WiFi.softAPConfig(localIP, gateway, subnet);

    // Avvia l'Access Point
    bool apStarted;
    if (strcmp(serverConfig.encryption, "Open") == 0) {
        apStarted = WiFi.softAP(serverConfig.ssid);
    } else {
        apStarted = WiFi.softAP(serverConfig.ssid, serverConfig.password);
    }

    if (!apStarted) {
        display.clearDisplay();
        common::println("Errore AP WiFi!", 0, 0, 1, SSD1306_WHITE);
        common::println("Verifica config.", 0, 10, 1, SSD1306_WHITE);
        display.display();
        delay(2000);
        return;
    }

    delay(500); // Attendi che l'AP sia pronto

    // Mostra info di rete
    display.clearDisplay();
    common::println("AP WiFi Creato!", 0, 0, 1, SSD1306_WHITE);
    display.setCursor(0, 10);
    display.print("SSID: ");
    display.println(serverConfig.ssid);
    display.setCursor(0, 20);
    display.print("IP: ");
    display.println(WiFi.softAPIP().toString());
    display.setCursor(0, 30);
    if (strcmp(serverConfig.encryption, "Open") != 0) {
        display.print("PSW: ");
        display.println(serverConfig.password);
    } else {
        display.println("Rete aperta");
    }
    display.display();
    delay(2000);

    // Mostra info complete
    showNetworkInfo();
    delay(1000);  // Mostra le info per 1 secondo

    // Inizializzazione endpoints
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
    });

    // Lista file
    server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        auto array = doc.to<JsonArray>();
        String path = "/";
        
        if(request->hasParam("path")) {
            path = request->getParam("path")->value();
            // Assicurati che il path inizi con /
            if(!path.startsWith("/")) {
                path = "/" + path;
            }
        }
        
        File dir = LittleFS.open(path);
        if(!dir || !dir.isDirectory()){
            request->send(404, "text/plain", "Directory not found");
            return;
        }

        // Se non siamo nella root, aggiungi il link alla directory superiore
        if(path != "/") {
            auto obj = array.add<JsonObject>();
            obj["name"] = "..";
            obj["path"] = path.substring(0, path.lastIndexOf("/"));
            if(obj["path"].as<String>().length() == 0) obj["path"] = "/";
            obj["isDirectory"] = true;
            obj["size"] = 0;
        }
        
        File file = dir.openNextFile();
        while(file){
            auto obj = array.add<JsonObject>();
            String fileName = String(file.name());
            // Rimuovi il path dal nome del file per mostrare solo il nome
            if(path != "/") {
                fileName = fileName.substring(path.length());
                if(fileName.startsWith("/")) fileName = fileName.substring(1);
            }
            obj["name"] = fileName;
            obj["path"] = String(file.name()); // Path completo
            obj["size"] = file.size();
            obj["isDirectory"] = file.isDirectory();
            file = dir.openNextFile();
        }

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // Download file
    server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
        if(!request->hasParam("file")){
            request->send(400, "text/plain", "File parameter missing");
            return;
        }

        String filename = request->getParam("file")->value();
        if(!LittleFS.exists("/" + filename)){
            request->send(404, "text/plain", "File not found");
            return;
        }

        request->send(LittleFS, "/" + filename, String(), true);
    });

    // Upload file
    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200);
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
        if(!index){
            if(LittleFS.exists("/" + filename)){
                LittleFS.remove("/" + filename);
            }
            request->_tempFile = LittleFS.open("/" + filename, "w");
        }

        if(request->_tempFile){
            request->_tempFile.write(data, len);
        }

        if(final){
            request->_tempFile.close();
        }
    });

    // Elimina file
    server.on("/delete", HTTP_DELETE, [](AsyncWebServerRequest *request){
        if(!request->hasParam("file")){
            request->send(400, "text/plain", "File parameter missing");
            return;
        }

        String filename = request->getParam("file")->value();
        if(LittleFS.remove("/" + filename)){
            request->send(200, "text/plain", "File deleted");
        } else {
            request->send(500, "text/plain", "Delete failed");
        }
    });

    // Crea cartella
    server.on("/createFolder", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        JsonDocument doc;
        deserializeJson(doc, (char*)data);
        String folderName = doc["name"].as<String>();

        if(LittleFS.mkdir("/" + folderName)){
            request->send(200, "text/plain", "Folder created");
        } else {
            request->send(500, "text/plain", "Create folder failed");
        }
    });

    server.begin();
    isServerRunning = true;
}

void stopFileWebServer() {
    if (isFileOperationInProgress) {
        common::println("Operazioni in corso", 0, 0, 1, SSD1306_WHITE);
        common::println("Attendere...", 0, 10, 1, SSD1306_WHITE);
        display.display();
        delay(2000);
        return;
    }
    
    server.end();
    WiFi.softAPdisconnect(true);  // Spegne l'AP
    WiFi.mode(WIFI_OFF);          // Spegne completamente il WiFi
    isServerRunning = false;
    
    display.clearDisplay();
    common::println("Server fermato", 0, 0, 1, SSD1306_WHITE);
    display.display();
    delay(1000);
}

void configureServer() {
    bool editing = true;
    int selected = 0;
    int scroll = 0;
    const char* menu_items[] = {
        "Nome rete (SSID)",
        "Password",
        "Crittografia",
        "Indirizzo server",
        "Salva e torna",
        "Indietro senza salvare"
    };
    int num_items = 6;
    int visible_items = LINES_ON_SCREEN - 2; // -1 per il titolo, -1 per margine

    while (editing) {
        display.clearDisplay();
        common::println("Configurazione Server", 0, 0, 1, SSD1306_WHITE);
        
        // Gestione scroll
        if (selected < scroll) scroll = selected;
        if (selected >= scroll + visible_items) scroll = selected - visible_items + 1;

        // Visualizza gli elementi visibili
        for (int i = 0; i < visible_items; i++) {
            int idx = scroll + i;
            if (idx < num_items) {
                if (idx == selected) {
                    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
                } else {
                    display.setTextColor(SSD1306_WHITE);
                }
                display.setCursor(0, 10 + i * 10);
                display.print(menu_items[idx]);
            }
        }

        // Barra di scroll
        if (num_items > visible_items) {
            int barHeight = (visible_items * (display.height() - 10)) / num_items;
            if (barHeight < 8) barHeight = 8;
            int barY = (scroll * ((display.height() - 10) - barHeight)) / (num_items - visible_items);
            display.drawRect(display.width() - 4, 10, 4, display.height() - 10, SSD1306_WHITE);
            display.fillRect(display.width() - 3, 10 + barY, 2, barHeight, SSD1306_WHITE);
        }
        
        display.display();

        if (digitalRead(buttonPin_UP) == LOW && selected > 0) {
            selected--;
            delay(200);
        }
        if (digitalRead(buttonPin_DWN) == LOW && selected < num_items - 1) {
            selected++;
            delay(200);
        }
        if (digitalRead(buttonPin_SET) == LOW) {
            delay(200);
            switch (selected) {
                case 0: // SSID
                    {
                        String result = getKeyboardInput(String(serverConfig.ssid), 32, "Inserisci SSID:");
                        if (result.length() > 0) {
                            strncpy(serverConfig.ssid, result.c_str(), 31);
                        }
                    }
                    break;
                case 1: // Password
                    {
                        String result = getKeyboardInput(String(serverConfig.password), 64, "Inserisci Password:");
                        if (result.length() > 0) {
                            strncpy(serverConfig.password, result.c_str(), 63);
                        }
                    }
                    break;
                case 2: // Crittografia
                    {
                        const char* crypto_types[] = {"WPA2", "WPA", "WEP", "Open"};
                        int crypto_selected = 0;
                        bool selecting = true;
                        
                        // Trova l'indice della crittografia corrente
                        for (int i = 0; i < 4; i++) {
                            if (strcmp(serverConfig.encryption, crypto_types[i]) == 0) {
                                crypto_selected = i;
                                break;
                            }
                        }
                        
                        while (selecting) {
                            display.clearDisplay();
                            common::println("Tipo Crittografia:", 0, 0, 1, SSD1306_WHITE);
                            for (int i = 0; i < 4; i++) {
                                if (i == crypto_selected) {
                                    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
                                } else {
                                    display.setTextColor(SSD1306_WHITE);
                                }
                                display.setCursor(0, 10 + i * 10);
                                display.print(crypto_types[i]);
                            }
                            display.display();

                            if (digitalRead(buttonPin_UP) == LOW && crypto_selected > 0) {
                                crypto_selected--;
                                delay(200);
                            }
                            if (digitalRead(buttonPin_DWN) == LOW && crypto_selected < 3) {
                                crypto_selected++;
                                delay(200);
                            }
                            if (digitalRead(buttonPin_SET) == LOW) {
                                strncpy(serverConfig.encryption, crypto_types[crypto_selected], 19);
                                // Se non è Open, verifica la password
                                if (strcmp(crypto_types[crypto_selected], "Open") != 0 && strlen(serverConfig.password) < 8) {
                                    display.clearDisplay();
                                    common::println("Password richiesta!", 0, 0, 1, SSD1306_WHITE);
                                    common::println("Min. 8 caratteri", 0, 10, 1, SSD1306_WHITE);
                                    display.display();
                                    delay(2000);
                                    String result = getKeyboardInput(String(""), 64, String("Inserisci Password:"));
                                    if (result.length() >= 8) {
                                        strncpy(serverConfig.password, result.c_str(), 63);
                                    } else {
                                        strncpy(serverConfig.encryption, "Open", 19); // Ripristina a Open se la password non è valida
                                        display.clearDisplay();
                                        common::println("Password non valida", 0, 0, 1, SSD1306_WHITE);
                                        common::println("Sicurezza: Open", 0, 10, 1, SSD1306_WHITE);
                                        display.display();
                                        delay(2000);
                                    }
                                }
                                selecting = false;
                                delay(200);
                            }
                            if (digitalRead(buttonPin_RST) == LOW) {
                                selecting = false;
                                delay(200);
                            }
                        }
                    }
                    break;
                case 3: // Indirizzo server
                    {
                        String result = getKeyboardInput(String(serverConfig.serverAddress), 16, "Inserisci IP:");
                        if (result.length() > 0) {
                            strncpy(serverConfig.serverAddress, result.c_str(), 15);
                        }
                    }
                    break;
                case 4: // Salva e torna
                    if (validateServerConfig()) {
                        serverConfig.isConfigured = true;
                        saveServerConfig();
                        editing = false;
                    } else {
                        display.clearDisplay();
                        common::println("Config non valida!", 0, 0, 1, SSD1306_WHITE);
                        if (strlen(serverConfig.ssid) == 0) {
                            common::println("SSID mancante", 0, 10, 1, SSD1306_WHITE);
                        }
                        if (strcmp(serverConfig.encryption, "Open") != 0 && strlen(serverConfig.password) < 8) {
                            common::println("Password non valida", 0, 20, 1, SSD1306_WHITE);
                            common::println("Min. 8 caratteri", 0, 30, 1, SSD1306_WHITE);
                        }
                        display.display();
                        delay(2000);
                    }
                    break;
                case 5: // Indietro senza salvare
                    editing = false;
                    break;
            }
        }
        if (digitalRead(buttonPin_RST) == LOW) {
            editing = false;
            delay(200);
        }
        delay(10);
    }
}

void webServerMenu() {
    common::debounce(50);
    bool running = true;
    int selected = 0;
    int lastSelected = -1;  // Per evitare ridisegni non necessari
    const char* menu_items[] = {
        "Avvia Server",
        "Configura Server",
        "Torna Indietro"
    };
    int num_items = 3;
    unsigned long lastButtonPress = 0;
    const int debounceDelay = 200;  // Tempo di debounce per i pulsanti

    loadServerConfig(); // Carica la configurazione all'avvio

    while (running) {
        // Aggiorna il display solo se è cambiato qualcosa
        if (selected != lastSelected) {
            display.clearDisplay();
            common::println("Menu Server Web", 0, 0, 1, SSD1306_WHITE);
            
            for (int i = 0; i < num_items; i++) {
                if (i == selected) {
                    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
                } else {
                    display.setTextColor(SSD1306_WHITE);
                }
                display.setCursor(0, 10 + i * 10);
                display.print(menu_items[i]);
            }

            // Mostra lo stato del server
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 45);
            display.print("Stato: ");
            display.print(isServerRunning ? "Attivo" : "Spento");
            
            display.display();
            lastSelected = selected;
        }

        // Gestione input con debounce
        unsigned long currentMillis = millis();
        if (currentMillis - lastButtonPress >= debounceDelay) {
            if (digitalRead(buttonPin_UP) == LOW && selected > 0) {
                selected--;
                lastButtonPress = currentMillis;
            }
            if (digitalRead(buttonPin_DWN) == LOW && selected < num_items - 1) {
                selected++;
                lastButtonPress = currentMillis;
            }
            if (digitalRead(buttonPin_SET) == LOW) {
                lastButtonPress = currentMillis;
                
                switch (selected) {
                    case 0: // Avvia Server
                        if (!isServerRunning) {
                            initFileWebServer();
                            if (isServerRunning) {
                                display.clearDisplay();
                                common::println("Server avviato su:", 0, 0, 1, SSD1306_WHITE);
                                common::println(getServerAddress().c_str(), 0, 10, 1, SSD1306_WHITE);
                                common::println("RST lungo per uscire", 0, 20, 1, SSD1306_WHITE);
                                display.display();
                                
                                // Loop del server con gestione migliorata
                                while (isServerRunning) {
                                    if (digitalRead(buttonPin_RST) == LOW) {
                                        unsigned long pressTime = millis();
                                        while (digitalRead(buttonPin_RST) == LOW) {
                                            delay(10);
                                            if (millis() - pressTime > 2000) {  // Ridotto da 3000 a 2000ms
                                                stopFileWebServer();
                                                delay(500);  // Piccolo delay dopo lo stop
                                                break;
                                            }
                                        }
                                    }
                                    delay(10);
                                }
                            }
                            // Forza un ridisegno del menu
                            lastSelected = -1;
                        } else {
                            stopFileWebServer();
                            // Forza un ridisegno del menu
                            lastSelected = -1;
                        }
                        break;
                    case 1: // Configura Server
                        configureServer();
                        // Forza un ridisegno del menu
                        lastSelected = -1;
                        break;
                    case 2: // Torna Indietro
                        if (isServerRunning) {
                            stopFileWebServer();
                        }
                        running = false;
                        break;
                }
            }
            if (digitalRead(buttonPin_RST) == LOW) {
                if (isServerRunning) {
                    stopFileWebServer();
                }
                running = false;
                lastButtonPress = currentMillis;
            }
        }
        delay(10);  // Piccolo delay per evitare sovraccarico CPU
    }
    // Delay finale per evitare rimbalzi quando si torna al menu precedente
    delay(200);
}

// Funzione per ottenere l'indirizzo IP del server
String getServerAddress() {
    return WiFi.localIP().toString();
}