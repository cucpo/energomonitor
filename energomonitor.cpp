// energometer 12.2.2025 -16:01
//   _____                                                                    
//  /  __ \                                                                   
//  | /  \/ _   _   ___  _ __    ___                                          
//  | |    | | | | / __|| '_ \  / _ \                                         
//  | \__/\| |_| || (__ | |_) || (_) |                                        
//   \____/ \__,_| \___|| .__/  \___/                                         
//                      | |                                                   
//                      |_|                                                   
//   _____                                                    _               
//  |  ___|                                                  | |              
//  | |__   _ __    ___  _ __   __ _   ___   _ __ ___    ___ | |_   ___  _ __ 
//  |  __| | '_ \  / _ \| '__| / _` | / _ \ | '_ ` _ \  / _ \| __| / _ \| '__|
//  | |___ | | | ||  __/| |   | (_| || (_) || | | | | ||  __/| |_ |  __/| |   
//  \____/ |_| |_| \___||_|    \__, | \___/ |_| |_| |_| \___| \__| \___||_|   
//                              __/ |                                         
//  

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>  // Knižnica pre mDNS

// Globálne premenné pre MQTT nastavenia
char mqtt_server[40];  // MQTT server
char mqtt_port[6];     // MQTT port
char mqtt_user[40];    // MQTT používateľ
char mqtt_password[40];// MQTT heslo
char mqtt_topic_power[40];  // MQTT téma pre výkon
char mqtt_topic_pulses[40]; // MQTT téma pre impulzy

// Korekčný faktor pre výpočet výkonu
float correction_factor = 1.0;  // Predvolená hodnota (žiadna korekcia)

// Nastavenia pre S0 impulzy
const int num_meters = 2;  // Počet wattmetrov
const int s0_pins[num_meters] = {2, 5};  // GPIO piny pre S0 impulzy (D4 = GPIO 2, D1 = GPIO 5)
const int led_pin = 4;  // GPIO pin pre vstavanú LED (D4 = GPIO 2)
const int pulses_per_kWh = 1000;  // Počet impulzov na 1 kWh

// Premenné pre každý wattmeter
volatile unsigned long pulse_count[num_meters] = {0};  // Počítadlo impulzov
unsigned long last_pulse_time[num_meters] = {0};  // Čas posledného impulzu
float power[num_meters] = {0.0};  // Aktuálny výkon vo W

// MQTT klient
WiFiClient espClient;
PubSubClient client(espClient);

// Webový server na porte 80 (štandardný port)
ESP8266WebServer server(80);

// Funkcie pre spracovanie prerušenia (každý impulz)
void IRAM_ATTR countPulse0() {
  handlePulse(0);
}

void IRAM_ATTR countPulse1() {
  handlePulse(1);
}

// Spoločná funkcia pre spracovanie impulzov
void handlePulse(int meter_index) {
  pulse_count[meter_index]++;
  unsigned long current_time = millis();
  unsigned long time_diff = current_time - last_pulse_time[meter_index];

  // Výpočet výkonu (P = 3600 / (time_diff * pulses_per_kWh)) s korekčným faktorom
  if (time_diff > 0) {
    power[meter_index] = (3600000.0 / (time_diff * pulses_per_kWh)) * correction_factor;  // Výkon vo W
  }
  last_pulse_time[meter_index] = current_time;

  // Rozsvieti LED na 100 ms
  digitalWrite(led_pin, LOW);  // LED na NodeMCU svieti, keď je pin LOW
  delay(100);  // Krátke oneskorenie
  digitalWrite(led_pin, HIGH);  // Vypni LED
}

// Pripojenie k MQTT serveru
void reconnect() {
  while (!client.connected()) {
    Serial.print("Pokus o pripojenie k MQTT serveru...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("Pripojené");
    } else {
      Serial.print("Chyba, rc=");
      Serial.print(client.state());
      Serial.println(" Skúsim znova za 5 sekúnd...");
      delay(5000);
    }
  }
}

// Funkcia pre webové rozhranie
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>Energometer Control</title></head><body>";
  html += "<h1>Energometer Control</h1>";
  for (int i = 0; i < num_meters; i++) {
    html += "<p>Wattmeter " + String(i + 1) + ": " + String(power[i], 2) + " W, Pulses: " + String(pulse_count[i]) + "</p>";
  }
  html += "<p><a href='/config'>Konfigurovať MQTT</a></p>";
  html += "<p><a href='/correction'>Nastaviť korekčný faktor</a></p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Funkcia pre konfiguráciu MQTT
void handleConfig() {
  if (server.method() == HTTP_POST) {
    // Uloženie nových hodnôt z formulára
    strncpy(mqtt_server, server.arg("server").c_str(), sizeof(mqtt_server));
    strncpy(mqtt_port, server.arg("port").c_str(), sizeof(mqtt_port));
    strncpy(mqtt_user, server.arg("user").c_str(), sizeof(mqtt_user));
    strncpy(mqtt_password, server.arg("password").c_str(), sizeof(mqtt_password));
    strncpy(mqtt_topic_power, server.arg("topic_power").c_str(), sizeof(mqtt_topic_power));
    strncpy(mqtt_topic_pulses, server.arg("topic_pulses").c_str(), sizeof(mqtt_topic_pulses));

    // Presmerovanie späť na hlavnú stránku
    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    // Zobrazenie konfiguračného formulára
    String html = "<!DOCTYPE html><html><head><title>MQTT Konfigurácia</title></head><body>";
    html += "<h1>MQTT Konfigurácia</h1>";
    html += "<form method='POST'>";
    html += "Server: <input type='text' name='server' value='" + String(mqtt_server) + "'><br>";
    html += "Port: <input type='text' name='port' value='" + String(mqtt_port) + "'><br>";
    html += "Používateľ: <input type='text' name='user' value='" + String(mqtt_user) + "'><br>";
    html += "Heslo: <input type='password' name='password' value='" + String(mqtt_password) + "'><br>";
    html += "Téma pre výkon: <input type='text' name='topic_power' value='" + String(mqtt_topic_power) + "'><br>";
    html += "Téma pre impulzy: <input type='text' name='topic_pulses' value='" + String(mqtt_topic_pulses) + "'><br>";
    html += "<input type='submit' value='Uložiť'>";
    html += "</form>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  }
}

// Funkcia pre nastavenie korekčného faktora
void handleCorrection() {
  if (server.method() == HTTP_POST) {
    // Uloženie nového korekčného faktora
    correction_factor = server.arg("correction").toFloat();

    // Presmerovanie späť na hlavnú stránku
    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    // Zobrazenie formulára pre nastavenie korekčného faktora
    String html = "<!DOCTYPE html><html><head><title>Korekčný faktor</title></head><body>";
    html += "<h1>Nastaviť korekčný faktor</h1>";
    html += "<form method='POST'>";
    html += "Korekčný faktor: <input type='text' name='correction' value='" + String(correction_factor, 2) + "'><br>";
    html += "<input type='submit' value='Uložiť'>";
    html += "</form>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  }
}

void setup() {
  Serial.begin(115200);

  // Inicializácia GPIO pre S0 impulzy a LED
  for (int i = 0; i < num_meters; i++) {
    pinMode(s0_pins[i], INPUT_PULLUP);
  }
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, HIGH);  // Vypni LED na začiatku

  // Nastavenie prerušenia pre S0 impulzy
  attachInterrupt(digitalPinToInterrupt(s0_pins[0]), countPulse0, FALLING);
  attachInterrupt(digitalPinToInterrupt(s0_pins[1]), countPulse1, FALLING);

  // Inicializácia WiFiManager
  WiFiManager wifiManager;

  // Pridanie vlastných parametrov pre MQTT konfiguráciu
  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "MQTT Používateľ", mqtt_user, 40);
  WiFiManagerParameter custom_mqtt_password("password", "MQTT Heslo", mqtt_password, 40);
  WiFiManagerParameter custom_mqtt_topic_power("topic_power", "MQTT Téma pre výkon", mqtt_topic_power, 40);
  WiFiManagerParameter custom_mqtt_topic_pulses("topic_pulses", "MQTT Téma pre impulzy", mqtt_topic_pulses, 40);

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_topic_power);
  wifiManager.addParameter(&custom_mqtt_topic_pulses);

  // Ak sa nepodarí pripojiť k uloženej sieti, spustí sa režim AP (Access Point)
  if (!wifiManager.autoConnect("ESP8266-Config")) {
    Serial.println("Nepodarilo sa pripojiť. Reštartujem...");
    delay(3000);
    ESP.restart();
  }

  // Uloženie MQTT nastavení
  strncpy(mqtt_server, custom_mqtt_server.getValue(), sizeof(mqtt_server));
  strncpy(mqtt_port, custom_mqtt_port.getValue(), sizeof(mqtt_port));
  strncpy(mqtt_user, custom_mqtt_user.getValue(), sizeof(mqtt_user));
  strncpy(mqtt_password, custom_mqtt_password.getValue(), sizeof(mqtt_password));
  strncpy(mqtt_topic_power, custom_mqtt_topic_power.getValue(), sizeof(mqtt_topic_power));
  strncpy(mqtt_topic_pulses, custom_mqtt_topic_pulses.getValue(), sizeof(mqtt_topic_pulses));

  // Ak sa pripojenie podarí
  Serial.println("WiFi pripojene");
  Serial.print("IP adresa: ");
  Serial.println(WiFi.localIP());

  // Inicializácia mDNS
  if (MDNS.begin("esp8266")) {  // Hostname: esp8266.local
    Serial.println("mDNS spustené. Prístupná adresa: http://esp8266.local");
    // Pridanie informácií o MQTT témach do mDNS
    MDNS.addServiceTxt("http", "tcp", "mqtt_topic_power", mqtt_topic_power);
    MDNS.addServiceTxt("http", "tcp", "mqtt_topic_pulses", mqtt_topic_pulses);
  } else {
    Serial.println("Chyba pri spustení mDNS!");
  }

  // Pripojenie k MQTT serveru
  client.setServer(mqtt_server, atoi(mqtt_port));

  // Nastavenie webového servera na porte 80
  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/correction", handleCorrection);
  server.begin();
  Serial.println("Webovy server spusteny na porte 80");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Posielanie dát cez MQTT raz za minútu
  static unsigned long last_send_time = 0;
  if (millis() - last_send_time > 60000) {  // 60000 ms = 1 minúta
    for (int i = 0; i < num_meters; i++) {
      // Poslať aktuálny výkon
      char power_str[10];
      dtostrf(power[i], 6, 2, power_str);
      client.publish((String(mqtt_topic_power) + String(i + 1)).c_str(), power_str);

      // Poslať celkový počet impulzov
      char pulses_str[20];
      sprintf(pulses_str, "%lu", pulse_count[i]);
      client.publish((String(mqtt_topic_pulses) + String(i + 1)).c_str(), pulses_str);
    }
    last_send_time = millis();
  }

  // Spracovanie požiadaviek webového servera
  server.handleClient();

  // Aktualizácia mDNS
  MDNS.update();
}