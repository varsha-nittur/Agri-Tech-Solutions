// Agri-Tech Solutions: one ESP8266 + Blynk IoT
// 1) Laser fence  2) Smart irrigation  3) Weather monitoring

#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Agritech Solutions"
#define BLYNK_AUTH_TOKEN    "YOUR_BLYNK_AUTH_TOKEN"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

char ssid[] = "YOUR_WIFI_NAME";
char pass[] = "YOUR_WIFI_PASSWORD";

#define SOIL1  D0          // soil sensor 1 (DO pin); soil sensor 2 (DO pin) is on A0
#define DHTPIN D7          // DHT11 data pin
#define BUZZER D8

// Laser fence: one LDR module per border (North, East, South, West)
int    ldrPin[4]    = {D1, D2, D5, D6};
String border[4]    = {"North", "East", "South", "West"};
bool   wasBroken[4] = {false, false, false, false};

// Irrigation: one pump relay per region, plus a manual switch from the app
int  relayPin[2] = {D3, D4};
bool manualOn[2] = {false, false};

DHT dht(DHTPIN, DHT11);
BlynkTimer timer;

// ---- 1) Laser fence: runs every 100 ms ----
void checkLaserFence() {
  bool alarm = false;
  for (int i = 0; i < 4; i++) {
    bool broken = (digitalRead(ldrPin[i]) == HIGH);   // HIGH = laser blocked (LDR in the dark)
    if (broken != wasBroken[i]) {                     // only act when the state changes
      wasBroken[i] = broken;
      Blynk.virtualWrite(V10 + i, broken ? 255 : 0);  // LED on the dashboard
      if (broken) Blynk.logEvent("intrusion", (border[i] + " border: animal detected!").c_str());
    }
    if (broken) alarm = true;
  }
  digitalWrite(BUZZER, alarm);                        // buzzer on while any beam is blocked
}

// ---- 2) Irrigation: runs every second ----
void waterPlants() {
  bool dry[2] = { digitalRead(SOIL1) == HIGH, analogRead(A0) > 512 };   // HIGH = soil is dry
  for (int r = 0; r < 2; r++) {
    bool pumpOn = dry[r] || manualOn[r];              // water if dry, or if the farmer switches it on
    digitalWrite(relayPin[r], pumpOn ? LOW : HIGH);   // relay board is active-LOW
    Blynk.virtualWrite(V2 + r, dry[r] ? "Dry" : "Wet");
    Blynk.virtualWrite(V4 + r, pumpOn ? 255 : 0);
  }
}

BLYNK_WRITE(V6) { manualOn[0] = param.asInt(); }      // manual pump switch, region 1
BLYNK_WRITE(V7) { manualOn[1] = param.asInt(); }      // manual pump switch, region 2

// ---- 3) Weather: runs every 5 seconds ----
void sendWeather() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) return;
  Blynk.virtualWrite(V0, t);
  Blynk.virtualWrite(V1, h);
}

void setup() {
  for (int i = 0; i < 4; i++) pinMode(ldrPin[i], INPUT);
  pinMode(SOIL1, INPUT);
  pinMode(BUZZER, OUTPUT);
  for (int r = 0; r < 2; r++) {
    digitalWrite(relayPin[r], HIGH);                  // pumps off at start
    pinMode(relayPin[r], OUTPUT);
  }
  dht.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(100L, checkLaserFence);
  timer.setInterval(1000L, waterPlants);
  timer.setInterval(5000L, sendWeather);
}

void loop() {
  Blynk.run();
  timer.run();
}
