#include <WiFi.h>
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>

const char* apSSID = "ESP32-Access-Point";
const char* apPassword = "123456789";

AsyncWebServer server(80);

const int eepromSize = 64; // Size of EEPROM to use for storing credentials
int ssidAddress = 0;
int passwordAddress = ssidAddress + eepromSize / 2;
String receivedSSID;
String receivedPassword;


void setup() {
  Serial.begin(115200);
  
  // Set to AP mode
  WiFi.softAP(apSSID, apPassword);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Read stored SSID and password from EEPROM
  EEPROM.begin(eepromSize);
  receivedSSID = readFromEEPROM(ssidAddress);
  receivedPassword = readFromEEPROM(passwordAddress);
  EEPROM.end();

  // connect to WiFi network (if valid credentials)
  if (receivedSSID.length() > 0 && receivedPassword.length() > 0) {
    connectToWiFi();
  }

  // request handler for input data
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "<form action=\"/save\" method=\"post\">SSID: <input type=\"text\" name=\"ssid\"><br>Password: <input type=\"password\" name=\"password\"><br><input type=\"submit\" value=\"Save\"></form>");
  });

  // request handler to save data
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
    if(request->hasParam("ssid", true) && request->hasParam("password", true)){
      receivedSSID = request->getParam("ssid", true)->value();
      receivedPassword = request->getParam("password", true)->value();

      // Save to EEPROM
      EEPROM.begin(eepromSize);
      writeToEEPROM(ssidAddress, receivedSSID);
      writeToEEPROM(passwordAddress, receivedPassword);
      EEPROM.end();

      Serial.print("Received SSID: ");
      Serial.println(receivedSSID);
      Serial.print("Received password: ");
      Serial.println(receivedPassword);

      // Restart ESP32
      request->send(200, "text/html", "Data saved. The ESP32 will now restart.");
      delay(200);
      ESP.restart();
    } else {
      request->send(400, "text/html", "Missing data.");
    }
  });

  // Start the web server
  server.begin();
}


void loop() {
}


String readFromEEPROM(int address) {
  String value;
  for (int i = address; i < address + eepromSize / 2; i++) {
    char c = EEPROM.read(i);
    if (c == 0) break; // Null terminator found, stop reading
    value += c;
  }
  return value;
}


void writeToEEPROM(int address, const String& value) {
  int length = value.length();
  for (int i = 0; i < length; i++) {
    EEPROM.write(address + i, value[i]);
  }

  EEPROM.write(address + length, 0);
  EEPROM.commit();
}


void connectToWiFi() {
  Serial.print("Connecting to WiFi ");
  Serial.println(receivedSSID);
  
  WiFi.begin(receivedSSID.c_str(), receivedPassword.c_str());

  int timeout = 30;
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(1000);
    Serial.print(".");
    timeout--;
  }

  // Display status on the serial monitor
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("Connected to WiFi: ");
    Serial.println(receivedSSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi");
  }
}
