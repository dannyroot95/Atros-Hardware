#include <Arduino.h>
#include <WiFiManager.h>
#include <esp_system.h>

#include <WebServer.h>

#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <FirebaseESP8266.h>
#endif

// Provide the token generation process info.
#include <addons/TokenHelper.h>
#include "time.h"

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

const int wifi = 5;
const int relay = 2;
const int botonPin = 15;
const int sensorPin = 4;

WebServer server(80);

unsigned long serverIP;
const char* ntpServer = "pool.ntp.org";

void handleRoot() {
  String html = "<html>";
  html += "<head>";
  html += "<style>";
  html += "html, body { height: 100%; display: flex; justify-content: center; align-items: center; }";
  html += "h1, p, button, div { font-family: 'Montserrat', sans-serif; }";
  html += "h1, p { margin-bottom: 20px; }";
  html += "</style>";
  html += "<link href=\"https://fonts.googleapis.com/css2?family=Montserrat:wght@400;700&display=swap\" rel=\"stylesheet\">";
  html += "</head>";
  html += "<body>";
  html += "<center>";
  html += "<h1>ATROS V1.0!</h1>";
  html += "<p><button id=\"myButton\" onclick=\"activarRelay()\" style=\"border-radius: 10px; color:#FFF; background-color: #FC0000; padding: 10px 20px;\">Abrir</button></p>";
  html += "<div id=\"loader\" style=\"display:none;\">Abriendo puerta...</div>";
  html += "</center>";
  html += "<script>";
  html += "function activarRelay() {";
  html += "  var button = document.getElementById('myButton');";
  html += "  var loader = document.getElementById('loader');";
  html += "  button.disabled = true;";
  html += "  loader.style.display = 'block';";
  html += "  location.href='/toggle';";
  html += "}";
  html += "</script>";
  html += "</body>";
  html += "</html>";

  server.send(200, "text/html", html);
}


void handleToggle() {
  digitalWrite(relay, HIGH);
  delay(1000);
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup()
{
  Serial.begin(115200);
  pinMode(relay, OUTPUT);
  pinMode(wifi, OUTPUT);
  pinMode(botonPin, INPUT_PULLUP);
  pinMode(sensorPin, INPUT_PULLUP);

  WiFiManager wifiManager;

  //wifiManager.resetSettings();

  // Configura y conecta Wi-Fi usando WiFiManager
  wifiManager.autoConnect("Atros","atrosv10"); 

  Serial.println("IP SERVER : "+WiFi.localIP());
  serverIP = WiFi.localIP();
  Serial.println();

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.begin();
  Serial.println("Servidor web iniciado");

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = "AIzaSyBmLxJjR2Hr1dQzoeZQv51Xf-I7CUJ3wV4";

  /* Assign the user sign in credentials */
  auth.user.email = "atrosx@gmail.com";
  auth.user.password = "123456";

  /* Assign the RTDB URL (required) */
  config.database_url = "puertaelectrica-bf3d5-default-rtdb.firebaseio.com";

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);

  // Comment or pass false value when WiFi reconnection will control by your code or third-party library
  Firebase.reconnectWiFi(true);

  Firebase.setDoubleDigits(5);
  configTime(0, 0, ntpServer);
}
void loop()
{

server.handleClient();

bool estadoActualPulsador = digitalRead(botonPin);

  // Firebase.ready() should be called repeatedly to handle authentication tasks.
 digitalWrite(relay, LOW);

 if (estadoActualPulsador == LOW) {
    resetearConfiguracionWiFiManager();
  }

 if (WiFi.status() == WL_CONNECTED)
  {
    
  digitalWrite(wifi, HIGH);
  
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0))
  {
    
    sendDataPrevMillis = millis();
    int sensorValue = digitalRead(sensorPin);
    Serial.println(sensorValue);

    uint64_t chipId = ESP.getEfuseMac();
    String chipIdString = String((uint16_t)(chipId >> 32), HEX) + String((uint32_t)chipId, HEX);
    
    String pathDoor = "/devices/"+chipIdString+"/door";
    String pathWebserver = "/devices/"+chipIdString+"/web_server";
    String pathStatus = "/devices/"+chipIdString+"/status";
    String pathTime = "/devices/"+chipIdString+"/time";

   
    Firebase.setInt(fbdo, pathDoor.c_str(), sensorValue) ? "ok" : fbdo.errorReason().c_str();
    Firebase.setString(fbdo, pathWebserver.c_str(), convertirIPaString(serverIP)) ? "ok" : fbdo.errorReason().c_str();
    Firebase.setInt(fbdo, pathTime.c_str(), getTime()) ? "ok" : fbdo.errorReason().c_str();
 
    String statusX = Firebase.getString(fbdo, pathStatus) ? fbdo.to<const char *>() : fbdo.errorReason().c_str();
    Serial.println("estado : "+statusX);

    if(statusX == "open"){
      
      digitalWrite(relay, HIGH);
      Firebase.setString(fbdo, pathStatus.c_str(), "close") ? "ok" : fbdo.errorReason().c_str();
      delay(1000);
      digitalWrite(relay, LOW);
      
      //resetearConfiguracionWiFiManager();
      
   }
    
  }
  }else{

    digitalWrite(wifi, LOW);
    
    }
}

String convertirIPaString(unsigned long ipAddressValue) {
  IPAddress ipAddress(ipAddressValue);

  String ipAddressString = "";
  for (int i = 0; i < 4; i++) {
    if (i > 0) {
      ipAddressString += ".";
    }
    ipAddressString += String(ipAddress[i]);
  }

  return ipAddressString;
}

void resetearConfiguracionWiFiManager(){
  
  WiFiManager wifiManager;
  wifiManager.resetSettings();

  digitalWrite(wifi, HIGH);
  delay(500);
   digitalWrite(wifi, LOW);
  delay(500);
   digitalWrite(wifi, HIGH);
  delay(500);
   digitalWrite(wifi, LOW);
  delay(500);
   digitalWrite(wifi, HIGH);
  delay(500);
   digitalWrite(wifi, LOW);
  delay(500);
  reiniciarESP32();
 
}

void reiniciarESP32() {
  esp_restart();
}

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}
