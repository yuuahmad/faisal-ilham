#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// include board
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <ACS712.h>

// include firebase dan wifi
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#define WIFI_SSID "YUUAHMAD laptop" // ini adalah nama wifi
#define WIFI_PASSWORD "yusuf1112"   // dan ini adalah passwordnya. kosongkan bagian ini kalau tidak pakai password
#define API_KEY "AIzaSyBsVj4YXqGT7PexdZ0QD1wK2UUjRtPk878"
#define DATABASE_URL "bangfaisal-1-default-rtdb.asia-southeast1.firebasedatabase.app" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app
#define USER_EMAIL "ahmadyusufmaulana0@gmail.com"
#define USER_PASSWORD "yusuf1112"

// kode untuk sensor arus
ACS712 sensor_arus_1(ACS712_05B, 34);
ACS712 sensor_arus_2(ACS712_20A, 35);

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;

// ini adalah setiing lcd
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ini adalah setting untuk voltage sensor
// nilai maksimal untuk voltage sensor adalah 25 volt
// voltage sensor bekerja dengan cara yang sama dengan voltage divider
const int voltageSensor = 33;
float vOUT1 = 0.0;
float vIN1 = 0.0;
float R1 = 30000.0;
float R2 = 7500.0;
int valuevoltage = 0;

// settingan voltage sensor kedua
const int voltageSensor2 = 32;
float vOUT2 = 0.0;
float vIN2 = 0.0;
int valuevoltage2 = 0;

// settingan relay
const int relay1 = 2;
const int relay2 = 4;

// boolean untuk relay
bool keadaanRelay1 = false;
bool keadaanRelay2 = false;

// ini settingan nilai tulis untuk lihat data
int nilaitulis = 0;

// ini adalah variabel nilai daya
float daya = 0;
void setup()
{

  Serial.begin(115200);
  // ini adalah inisiasi seting untuk lcd
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("polisi tidur    ");
  lcd.setCursor(0, 1);
  lcd.print("generator v1.0  ");
  delay(5000);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  lcd.setCursor(0, 0);
  lcd.print("connecting     ");
  lcd.setCursor(0, 1);
  lcd.print("               ");
  delay(2000);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    lcd.setCursor(0, 1);
    lcd.print(".");
    delay(2000);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  lcd.setCursor(0, 0);
  lcd.print("connected!?    ");
  lcd.setCursor(0, 1);
  lcd.print("IP:");
  lcd.print(WiFi.localIP());
  delay(1000);

  // kalibrasi nilai sensor arus
  Serial.println("Calibrating... Ensure that no current flows through the sensor at this moment");
  int zero_1 = sensor_arus_1.calibrate();
  int zero_2 = sensor_arus_2.calibrate();
  Serial.println("Done!");
  Serial.println("Zero point for this sensor_arus_1 = " + zero_1);
  Serial.println("Zero point for this sensor_arus_2 = " + zero_1);

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(5);

  // setting inisiasi relay
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  delay(2000);
}

void loop()
{

  // ini adalah kode untuk sensor arus
  float arus_1 = sensor_arus_1.getCurrentDC();
  float arus_2 = sensor_arus_2.getCurrentDC();
  // tampilkan nilai arus ke serial dan lcd
  Serial.println(String("arus 1= ") + arus_1 + " arus 2 = " + arus_2);

  //kode sensor voltase.
  // nilali 4.3 didapatkan dari trial dan error yang saya lakukan.
  // harusnya nilai ini menggunakan nilai 5, karena votage refnya memang segitu.
  // namun karena masalah resolusi, saya jadikan 4.3
  valuevoltage = analogRead(voltageSensor);
  vOUT1 = (valuevoltage * 4.3) / 4095.0;
  vIN1 = vOUT1 / (R2 / (R1 + R2));
  daya = vIN1 * arus_1;
  lcd.setCursor(0, 0);
  lcd.print("daya:");
  lcd.print(daya, 2);
  lcd.print("      ");

  // kode sensor voltase kedua
  valuevoltage2 = analogRead(voltageSensor2);
  vOUT2 = (valuevoltage2 * 4.3) / 4095.0;
  vIN2 = vOUT2 / (R2 / (R1 + R2));
  lcd.setCursor(0, 1);
  lcd.print("tgngn bat= ");
  lcd.print(vIN2, 2);
  lcd.print("       ");

  if (vIN1 > 3)
  {
    digitalWrite(relay1, HIGH);
    keadaanRelay1 = true;
  }
  else
  {
    digitalWrite(relay1, LOW);
    keadaanRelay1 = false;
  }

  if (vIN2 > 3)
  {
    digitalWrite(relay2, HIGH);
    keadaanRelay2 = true;
  }
  else
  {
    digitalWrite(relay2, LOW);
    keadaanRelay2 = false;
  }

  // ini kode untuk upload kode ke firebase
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 4000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    Firebase.RTDB.setFloat(&fbdo, "/daya", daya);
    Firebase.RTDB.setFloat(&fbdo, "/voltage1", vIN1);
    Firebase.RTDB.setFloat(&fbdo, "/voltage2", vIN2);
    Firebase.RTDB.setFloat(&fbdo, "/current1", arus_1);
    Firebase.RTDB.setFloat(&fbdo, "/current2", arus_2);
    Firebase.RTDB.setBool(&fbdo, "/keadaanRelay1", keadaanRelay1);
    Firebase.RTDB.setBool(&fbdo, "/keadaanRelay2", keadaanRelay2);
    Serial.print("Selesai Tulis");
    Serial.print(nilaitulis);
    Serial.println();
    nilaitulis++;
  }
}