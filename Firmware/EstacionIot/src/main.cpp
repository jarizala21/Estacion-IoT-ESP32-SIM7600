#include <Arduino.h>
#include <SPI.h>
#include <esp_sleep.h>
#include <Adafruit_MAX31865.h>
#include "config.h"   // Archivo con datos privados

#define LOADSW_PIN 32
#define SIM_RX      16
#define SIM_TX      17
#define SIM_PWRKEY  25
#define SIM_BAUD    115200

HardwareSerial sim7600(2);

#define BAT_PIN 35
#define R1 22.0
#define R2 10.0
#define BAT_SAMPLES 8

#define MAX_CS   5
#define MAX_DI   23
#define MAX_DO   19
#define MAX_CLK  18
#define RNOMINAL 100.0
#define RREF     423.0

Adafruit_MAX31865 max31865(MAX_CS, MAX_DI, MAX_DO, MAX_CLK);

void sendAT(const char* cmd, uint32_t waitMs = 1000) {
  sim7600.println(cmd);
  delay(waitMs);
}

String readSIMResponse(uint32_t timeoutMs = 2000) {
  String resp = "";
  uint32_t t = millis();
  while (millis() - t < timeoutMs) {
    while (sim7600.available()) resp += (char)sim7600.read();
    if (resp.indexOf("OK") != -1 || resp.indexOf("ERROR") != -1) break;
    delay(10);
  }
  return resp;
}

bool getCCLKFull(int &hour, int &minute, int &second) {
  while (sim7600.available()) sim7600.read();
  sim7600.println("AT+CCLK?");
  String resp = readSIMResponse(3000);
  int q1 = resp.indexOf('"');
  int q2 = resp.lastIndexOf('"');
  if (q1 == -1 || q2 == -1 || q1 == q2) return false;

  String raw = resp.substring(q1 + 1, q2);
  int comma = raw.indexOf(',');
  if (comma == -1) return false;

  String timePart = raw.substring(comma + 1);

  hour   = timePart.substring(0, 2).toInt() - 1;
  if (hour < 0) hour += 24;

  minute = timePart.substring(3, 5).toInt();
  second = timePart.substring(6, 8).toInt();

  return true;
}

void powerOnSIM7600() {
  digitalWrite(SIM_PWRKEY, LOW);
  delay(1500);
  digitalWrite(SIM_PWRKEY, HIGH);
}

void powerOffSIM7600() {
  sendAT("AT+CPOWD=1", 5000);
  digitalWrite(SIM_PWRKEY, LOW);
  delay(3000);
  digitalWrite(SIM_PWRKEY, HIGH);
}

float readBat() {
  uint32_t mv_sum = 0;
  for (int i = 0; i < BAT_SAMPLES; i++) {
    mv_sum += analogReadMilliVolts(BAT_PIN);
    delay(5);
  }
  return (mv_sum / (float)BAT_SAMPLES / 1000.0) * ((R1 + R2) / R2);
}

void sendData(float tempExt, float tempInt, float vBat) {

  String url = String(BASE_URL);
  url += "?temp="    + String(tempExt, 2);
  url += "&tempint=" + String(tempInt, 2);
  url += "&bat="     + String(vBat, 3);

  sendAT("AT+HTTPTERM");
  sendAT("AT+HTTPINIT");
  sendAT("AT+HTTPPARA=\"CID\",1");

  sim7600.print("AT+HTTPPARA=\"URL\",\"");
  sim7600.print(url);
  sim7600.println("\"");
  delay(1000);

  sendAT("AT+HTTPACTION=0", 10000);
}

uint64_t secondsUntil(int curH, int curM, int curS, int tH, int tM) {
  int totalNow    = curH * 3600 + curM * 60 + curS;
  int totalTarget = tH  * 3600 + tM  * 60;

  if (totalTarget <= totalNow) totalTarget += 24 * 3600;

  return (uint64_t)(totalTarget - totalNow);
}

void setup() {

  Serial.begin(115200);

  pinMode(LOADSW_PIN, OUTPUT);
  digitalWrite(LOADSW_PIN, HIGH);

  pinMode(SIM_PWRKEY, OUTPUT);
  digitalWrite(SIM_PWRKEY, HIGH);

  sim7600.begin(SIM_BAUD, SERIAL_8N1, SIM_RX, SIM_TX);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  max31865.begin(MAX31865_2WIRE);

  delay(2000);

  powerOnSIM7600();
  delay(12000);

  sendAT("AT");
  sendAT("AT+CPIN?");
  sendAT("AT+CPIN=\"" SIM_PIN "\"", 8000);
  sendAT("AT+CREG?");
  sendAT("AT+CSOCKSETPN=1");
  sendAT("AT+NETOPEN", 6000);

  sendAT("AT+CGDCONT=1,\"IP\",\"" APN "\"");
  sendAT("AT+CGACT=1,1", 3000);

  sendAT("AT+CNTP=\"pool.ntp.org\",8");
  sendAT("AT+CNTP", 5000);

  int hour = 0, minute = 0, second = 0;
  getCCLKFull(hour, minute, second);

  bool sendNow = (hour == 8  && minute == 45) ||
                 (hour == 13 && minute == 0);

  if (sendNow) {

    float tempExt = max31865.temperature(RNOMINAL, RREF);
    float tempInt = analogReadMilliVolts(34) / 10.0;
    float vBat    = readBat();

    sendData(tempExt, tempInt, vBat);
  }

  uint64_t sTo845  = secondsUntil(hour, minute, second, 8, 45);
  uint64_t sTo1300 = secondsUntil(hour, minute, second, 13, 0);

  uint64_t sleepSec = (sTo845 < sTo1300) ? sTo845 : sTo1300;

  powerOffSIM7600();
  delay(5000);

  digitalWrite(LOADSW_PIN, LOW);
  delay(1000);

  esp_sleep_enable_timer_wakeup(sleepSec * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {}