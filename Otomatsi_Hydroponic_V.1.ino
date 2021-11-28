#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#endif
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#define WIFI_SSID "SONIC Farm_plus"
#define WIFI_PASSWORD "JayaJayaJaya"
#define API_KEY "AIzaSyDF7ehqU1OVsU8rV61xmXFf-uA3AEQEtvg"
#define DATABASE_URL "hidroponik-25171-default-rtdb.firebaseio.com"

#define USER_EMAIL "hasanudinhendra18@gmail.com"
#define USER_PASSWORD "demimasadepan"

//Define Firebase Data object
FirebaseData fbdo;
FirebaseData stream;

FirebaseAuth auth;
FirebaseConfig config;
//----MILIIS
unsigned long sendDataPrevMillis = 0;
unsigned long currentMillissensor = 0;
unsigned long currentMillislcd = 0;
//PIN YANG DIGUNAKAN
const int pH  = 34;
const int pinTds = 35;
const int button1 = 12;
const int button2 = 13;
const int pompa1 = 14;
const int pompa2 = 27;

//-----LOGIKA
int pTds;
int pPh;
int pBtn1;
int pBtn2;
//----PH

float Po = 0;
float PH_step;
int nilai_analog_PH;
double TeganganPh;

//untuk kalibrasi
float PH4 = 3.145;
float PH7 = 2.541;

LiquidCrystal_I2C lcd(0x27, 16, 2);

//tds
int myInt1 = 0;
String myString1;

//ph
int myInt2 = 0;
String myString2;

float teg[10];
float tds ;
float rata_rata_teg;
//-------BTAS GLOBAL SENSOR

//PATH UNTUK MONITOR
String parentPath = "/sistem1/sistem1";
String childPath[4] = {"/ppm", "/ph", "/mPpm", "/mPh"};
//----MULTIPATH
String parentPathSistem2 = "/sistem2";
String parentPathSistem3 = "/sistem3";
String parentPathSistem4 = "/sistem4";
String parentPathSistem5 = "/sistem5";
//-----BATAS

void streamCallback(MultiPathStreamData stream)
{
  size_t numChild = sizeof(childPath) / sizeof(childPath[0]);

  for (size_t i = 0; i < numChild; i++)
  {
    if (stream.get(childPath[i]))
    {
      Serial.printf("path: %s, event: %s, type: %s, value: %s%s", stream.dataPath.c_str(), stream.eventType.c_str(), stream.type.c_str(), stream.value.c_str(), i < numChild - 1 ? "\n" : "");
      handlePin(stream.dataPath, stream.value);
    }
  }

  Serial.println();
  Serial.printf("Received stream payload size: %d (Max. %d)\n\n", stream.payloadLength(), stream.maxPayloadLength());
}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
    Serial.println("stream timed out, resuming...\n");

  if (!stream.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
}
void setup()
{
  lcd.begin();
  lcd.backlight();
  pinMode(pinTds, INPUT);
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(pompa1, OUTPUT);
  pinMode(pompa2, OUTPUT);
  pinMode(pH, INPUT);
  digitalWrite(pompa1, HIGH);
  digitalWrite(pompa2, HIGH);

  //LCD
  lcd.setCursor(0, 0);
  lcd.print(" Selamat Datang");
  lcd.setCursor(0, 1);
  lcd.print(" ..Solartech..");
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&config, &auth);

  Firebase.reconnectWiFi(true);

  //Recommend for ESP8266 stream, adjust the buffer size to match your stream data size
#if defined(ESP8266)
  stream.setBSSLBufferSize(2048 /* Rx in bytes, 512 - 16384 */, 512 /* Tx in bytes, 512 - 16384 */);
#endif


  if (!Firebase.beginMultiPathStream(stream, parentPath))
    Serial.printf("sream begin error, %s\n\n", stream.errorReason().c_str());

  Firebase.setMultiPathStreamCallback(stream, streamCallback, streamTimeoutCallback);
}

void loop()
{
  //------------------READING SENSOR TDAS DAN PH
  if ((millis() - currentMillissensor) > 500) {
    currentMillissensor = millis();

    for ( int i = 0; i < 10; i++) {
      int val = analogRead(pinTds);
      teg[i] = val * (5.0 / 1023);
    }

    rata_rata_teg = (teg[0] + teg[1] + teg[2] + teg[3] + teg[4] + teg[5] + teg[6] + teg[7] + teg[8] + teg[9]) / 10 ;
    tds = (179.0 * rata_rata_teg) - 1073 ;

    myString1 = tds;
    myInt1 = myString1.toInt();
    if (myInt1 < 0) {
      myInt1 = 0;
    }


    nilai_analog_PH = analogRead(pH);
    TeganganPh = nilai_analog_PH * (5.0 / 4095.0);
    PH_step = (PH4 - PH7) / 3;
    Po = 7.00 + ((PH7 - TeganganPh) / PH_step);     //Po = 7.00 + ((teganganPh7 - TeganganPh) / PhStep);
    // rmus parsing
    myString2 = String(Po);
    myInt2 = myString2.toInt();
    if (myInt2 < 0) {
      myInt2 = 0;
    }
  }

  //-------------KIRIM DATA KE FIREBASE
  if (Firebase.ready() && (millis() - sendDataPrevMillis >= 15000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    //***** Sending Data
    Firebase.setInt(fbdo, "/sistem1/sistem1/ppmMonitor/ppmMonitor", myInt1);
    Firebase.setInt(fbdo, "/sistem1/sistem1/phMonitor/phMonitor", myInt2);
  }
  ///......LOGIKA TOMBOL
  int btn1 = digitalRead(button1);
  if (btn1 == 1 || pBtn1 == 1 || myInt1 < pTds) {
    digitalWrite(pompa1, LOW);
  } else if (btn1 == 0 || pBtn1 == 0 || myInt1 >= pTds) {
    digitalWrite(pompa1, HIGH);
  }
  int btn2 = digitalRead(button2);

  if (btn2 == 1 || pBtn2 == 1 || myInt2 < pPh) {
    digitalWrite(pompa2, LOW);
  } else if (btn2 == 0 || pBtn2 == 0 || myInt2 >= pPh) {
    digitalWrite(pompa2, HIGH);
  }
  //-------LCD
  if ((millis() - currentMillislcd) >= 3000) {
    currentMillislcd = millis();
    lcd.clear();
  }
  lcd.setCursor(0, 0);
  lcd.print("PPM: ");
  lcd.setCursor(4, 0);
  lcd.print(myInt1);
  lcd.setCursor(0, 1);
  lcd.print("PH : ");
  lcd.setCursor(4, 1);
  lcd.print(myInt2);
  lcd.setCursor(9, 0);
  lcd.print("st: ");
  lcd.setCursor(12, 0);
  lcd.print(pTds);
  lcd.setCursor(9, 1);
  lcd.print("st: ");
  lcd.setCursor(12, 1);
  lcd.print(pPh);
  Serial.print("Nutrisi: ");
  Serial.println(tds);
  Serial.print("PH : ");
  Serial.println(Po);

}
void handlePin(String path, String value) {
  if (path == "/ppm/ppmContent") {
    pTds = value.toInt();
  } else if (path == "/mPpm/mPpm") {
    pBtn1 = value.toInt();
  } else if (path == "/ph/phContent") {
    pPh = value.toInt();
  } else if (path == "/mPh/mPh") {
    pBtn2 = value.toInt();
  } else {
    Serial.println("NO MATCHED");
  }
}
