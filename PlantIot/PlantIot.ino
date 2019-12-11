
#include <AWS_IOT.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include "time.h"



#include "DHT.h"

#define DHTPIN 22 // what digital pin we're connected to
int soilSensor = 39;
int cdsSensor = 36;
#define LEDPIN 16 // LED

// Uncomment whatever type you're using!
#define DHTTYPE DHT11 // DHT 11

// plant led
const int plantLedRed = 17;
const int plantLedBlue = 5;

boolean redLed = false;
boolean blueLed = false;
boolean redLedByApp = false;
boolean blueLedByApp = false;

//time
const char* ntpServer = "pool.ntp.org";
uint8_t timeZone = 9;
uint8_t summerTime = 0; // 3600

int s_hh = 12;      // 시간 설정 변수 < 0 조건 위해 자료형 int
int s_mm = 59;
uint8_t s_ss = 45;
uint16_t s_yy = 2019;
uint8_t s_MM = 11;
uint8_t s_dd = 19;

int second = 0;
int minute = 0;

time_t now;
time_t prevEpoch;
struct tm timeinfo;

void get_NTP() {
  configTime(3600 * timeZone, 3600 * summerTime, ntpServer);
  Serial.println("NTP sync");
  while(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    set_time();                // set time
    return;
  }
}

void set_time() {
  struct tm tm;
  tm.tm_year = s_yy - 1900;
  tm.tm_mon = s_MM - 1;
  tm.tm_mday = s_dd;
  tm.tm_hour = s_hh;
  tm.tm_min = s_mm;
  tm.tm_sec = s_ss;
  time_t t = mktime(&tm);
  printf("Setting time: %s", asctime(&tm));
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}

void printLocalTime() {
  if (time(&now) != prevEpoch) {
    Serial.println(time(&now));
    getLocalTime(&timeinfo);
    int dd = timeinfo.tm_mday;
    int MM = timeinfo.tm_mon + 1;
    int yy = timeinfo.tm_year +1900;
    int ss = timeinfo.tm_sec;
    int mm = timeinfo.tm_min;
    int hh = timeinfo.tm_hour;
    int week = timeinfo.tm_wday;
    second = ss;
    minute = mm;
    if(hh == 7) {
      redLed = true;
      blueLed = true;
    } else if(hh == 18) {
      redLed = false;
      blueLed = false;
    }
    Serial.print(week); Serial.print(". ");
    Serial.print(yy); Serial.print(". ");
    Serial.print(MM); Serial.print(". ");
    Serial.print(dd); Serial.print(" ");
    Serial.print(hh); Serial.print(": ");
    Serial.print(mm); Serial.print(": ");
    Serial.println(ss);
    prevEpoch = time(&now);
  }
}

// dht
DHT dht(DHTPIN, DHTTYPE);
boolean led_state = false;


//aws
AWS_IOT hornbill; // AWS_IOT instance

char WIFI_SSID[]="J house";
//char WIFI_SSID[]="KAU-Guest";
char WIFI_PASSWORD[]= "Applecare12!@";
char HOST_ADDRESS[]="a2iilqapybb349-ats.iot.ap-northeast-2.amazonaws.com";
char CLIENT_ID[]= "iotService";
char TOPIC_NAME_update[]= "$aws/things/iotService/shadow/update";
char TOPIC_NAME_delta[]= "$aws/things/iotService/shadow/update/delta";
char TOPIC_NAME_get[]= "$aws/things/iotService/shadow/get";

int status = WL_IDLE_STATUS;
int tick = 0, msgCount = 0, msgReceived = 0;
char payload[512];
char rcvdPayload[512];

// subscribe ../update/delta
void callBackDelta(char *topicName, int payloadLen, char *payLoad)
{
  strncpy(rcvdPayload,payLoad,payloadLen);
  rcvdPayload[payloadLen] = 0;
  msgReceived = 1;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  while (status != WL_CONNECTED){  
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(WIFI_SSID);
    
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//    status = WiFi.begin(WIFI_SSID, NULL);

    // wait 5 seconds for connection:
    delay(5000);
  }
  
  Serial.println("Connected to wifi");
  
  if(hornbill.connect(HOST_ADDRESS,CLIENT_ID)== 0) { // Connect to AWS
    Serial.println("Connected to AWS");
    delay(1000);
    
    if(0==hornbill.subscribe(TOPIC_NAME_delta,callBackDelta))
//    if(0==hornbill.subscribe(TOPIC_NAME_get,callBackDelta))
      Serial.println("Subscribe(../update/delta) Successfull");
//      Serial.println("Subscribe(../get) Successfull");
    else {
      Serial.println("Subscribe(../update/delta) Failed, Check the Thing Name, Certificates");
//      Serial.println("Subscribe(../get) Failed, Check the Thing Name, Certificates");
      while(1);
    }
  }
  else {
    Serial.println("AWS connection failed, Check the HOST Address");
    while(1);
  }
  delay(2000);
  get_NTP();
  
  dht.begin(); //Initialize the DHT11 sensor
  pinMode(LEDPIN, OUTPUT);
  pinMode(DHTPIN, INPUT);
  pinMode(plantLedRed, OUTPUT);
  pinMode(plantLedBlue, OUTPUT);
  digitalWrite(plantLedRed, HIGH);
  digitalWrite(plantLedBlue, HIGH);
  digitalWrite(LEDPIN, LOW);
}

void loop() {
  int cds = analogRead(cdsSensor);
  int soil = analogRead(soilSensor);
  Serial.print("cds LightSeneor: ");
  Serial.println(cds);
  Serial.println(soil);
  
  // timeClient Update
  
  if(Serial.available() > 0){
    String temp = Serial.readStringUntil('\n');
    if (temp == "1") set_time();   // set time
    else if (temp == "2") get_NTP(); //NTP Sync
  }
  printLocalTime();
  Serial.println(redLed);
  bool red = false;
  if(redLed || redLedByApp) {
    red = true;
  }
  bool blue = false;
  if(blueLed || blueLedByApp) {
    blue = true;
  }
  if(red && blue) {
    digitalWrite(plantLedRed, HIGH);
    digitalWrite(plantLedBlue, HIGH);
//    Serial.println("test");
  } else if(!red && !blue) {
    digitalWrite(plantLedRed, LOW);
    digitalWrite(plantLedBlue, LOW);
//    Serial.println("nttest");
  }
  
  
  // at first, handle subscribe messages..
  StaticJsonDocument<200> msg; // reserve stack mem for handling json msg
  if(msgReceived == 1) {
    msgReceived = 0;
    Serial.print("Received Message(Update/Delta):");
    Serial.println(rcvdPayload);
    
    // Deserialize the JSON document
    if (deserializeJson(msg, rcvdPayload)) { // if error
      Serial.print(F("deserializeJson() failed.. \n"));
      while(1);
    }
    
    // parsing delta msg
    // ex) {"version":63,"timestamp":1573803485,"state":{"led":"OFF"},
    // "metadata":{"led":{"timestamp":1573803485}}}
    JsonObject state = msg["state"];
//    Serial.print("State");
//    Serial.println(state);
//    JsonObject report = state["reported"];
//    Serial.print("Report");
//    Serial.println(report);
    String led = state["led"];
    if (led.equals("ON")) // turn ON
      led_state = true;
    else if (led.equals("OFF")) // turn OFF
      led_state = false;
    else { // invalid delta
      Serial.println("Invalid ../delta message.. Led");
    }
    String plantLed = state["plantLed"];
    if(plantLed.equals("ON")) {
      redLedByApp = true;
      blueLedByApp = true;
    } else if(plantLed.equals("OFF")) {
      redLedByApp = false;
      blueLedByApp = false;
    }else { // invalid delta
      Serial.println("Invalid ../delta message..PlantLed");
    }
    digitalWrite(LEDPIN, led_state);
    
    // publish report led ON {"state":{"reported":{"led":"ON"}}}
    sprintf(payload,"{\"state\":{\"reported\":{\"led\":\"%s\"}}}",(led_state?"ON":"OFF"));
    if(hornbill.publish(TOPIC_NAME_update,payload) == 0) { // Publish the message
      Serial.print("Publish Message: ");
      Serial.println(payload);
    }

    else {
      Serial.print("Publish failed: ");
      Serial.println(payload);
    }
  }
  
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(5000);
  } else {
    // publish report msg
    // { "state": {
    //    "reported": { "temp": nn, "humid": nn, "led": "ON" | "OFF" }
    //    }
    // }
//    sprintf(payload,"{\"state\":{\"reported\":{\"temp\":%.1f,\"airHumid\":%.1f, \"soilHumid\": %d,\"led\":\"%s\", \"plantLed\": \"%s\"}}}",
//    t,h,soil,(led_state ? "ON" : "OFF" ), (red && blue ? "ON" : "OFF"));
    sprintf(payload,"{\"state\":{\"reported\":{\"temp\":%.1f,\"airHumid\":%.1f, \"soilHumid\": %d, \"cds\": %d, \"timeStamp\": %d,\"led\":\"%s\"}}}",
    t,h,soil, cds, prevEpoch,(led_state ? "ON" : "OFF" ));
    Serial.print("Message: ");
    Serial.println(payload);
//    if(second == 0) {
    if(t < 15) {// 온도가 너무 낮을 상황이 오기 전에 바로 온풍기를 켤 수 있도록 함
      if(hornbill.publish(TOPIC_NAME_update,payload) == 0) { // Publish the message
        Serial.print("Publish Message: ");
        Serial.println(payload);
      } else {
        Serial.print("Publish failed: ");
        Serial.println(payload);
      }
    }
    if(t > 50) { // 이 상황은 불이 난 상황을 가정하여 집에 이상이 있음을 알림
      if(hornbill.publish(TOPIC_NAME_update,payload) == 0) { // Publish the message
        Serial.print("Publish Message: ");
        Serial.println(payload);
      } else {
        Serial.print("Publish failed: ");
        Serial.println(payload);
      }
    }
    if(minute % 3 == 0 && second == 0) { // 매 3분단위 시간마다 퍼블리시
      if(hornbill.publish(TOPIC_NAME_update,payload) == 0) { // Publish the message
        Serial.print("Publish Message: ");
        Serial.println(payload);
      } else {
        if(hornbill.publish(TOPIC_NAME_update,payload) == 0) { // Publish the message
          Serial.print("REPublish Message: ");
          Serial.println(payload);
        }
      }
    }
    
    // the temp and humidity every 1 minute.
    vTaskDelay(1000 / portTICK_RATE_MS);
  }
}
