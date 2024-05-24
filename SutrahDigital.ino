#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUDP.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <FirebaseESP32.h>
#include <EEPROM.h>

#define obstacle 5
#define buzzer 33
#define pushButton 4

char ssid[] = "your ssid";
char pw[] = "your password";

#define FIREBASE_HOST ""
#define FIREBASE_AUTH ""

FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

LiquidCrystal_I2C LCD(0x27, 16, 2);
WiFiUDP ntpUDP;
RTC_DS3231 rtc;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

int countSujud = 0, totalRakaah = 0, previousState;
int bacaSensor, totalRakaatSholat;
unsigned long previousMillis = 0;
bool value = LOW;
String waktuSholat;
int buttonState = LOW;
int lastButtonState = LOW;
unsigned long debounceDelay = 50;

void checkButton() {
  buttonState = digitalRead(pushButton);
  if (buttonState != lastButtonState) {
    delay(debounceDelay);
    buttonState = digitalRead(pushButton);
    if (buttonState == LOW) {
      Serial.println("ditekan");
      totalRakaatSholat = 0; // Reset nilai totalRakaatSholat menjadi 0
      EEPROM.write(0, 0); // Simpan nilai 0 ke EEPROM
      EEPROM.commit();
    }
  }
  lastButtonState = buttonState;
}

void sensorDetection(int rakaat) {
  int sensorValue = digitalRead(obstacle);
  if (sensorValue == LOW && previousState == HIGH) {
    countSujud++;
    Serial.print("Jumlah Sujud : ");
    Serial.println(countSujud);
  }
  if (countSujud == 2) {
    countSujud = 0;
    totalRakaah++;
    Serial.print("Jumlah Rakaat: ");
    Serial.println(totalRakaah);
    totalRakaatSholat++;
  }
  previousState = sensorValue;
  if ((totalRakaah >= rakaat && countSujud >= 1) || totalRakaah == rakaat) {
    if (millis() - previousMillis >= 100) {
      digitalWrite(buzzer, !value);
      value = !value;
      previousMillis = millis();
    }
  }
}

void resetEEPROM() {
  EEPROM.write(0, 0);
  EEPROM.commit();
}

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, pw);
  pinMode(obstacle, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(pushButton, INPUT_PULLUP);

  LCD.begin();
  LCD.clear();
  LCD.setCursor(4, 0);
  LCD.print("WELCOME");
  LCD.setCursor(0, 1);
  LCD.print("Connecting...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Tidak terhubung ke jaringan WiFi...");
  }
  
  Serial.println("Terhubung ke jaringan WiFi!");
  LCD.clear();
  LCD.setCursor(4, 0);
  LCD.print("WELCOME");
  LCD.setCursor(0, 1);
  LCD.print("Connected!!!");
  timeClient.begin();
  timeClient.setTimeOffset(25200);

  rtc.begin();

  // Periksa apakah RTC kehilangan daya, jika ya, atur waktu default
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time to default!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  firebaseConfig.host = FIREBASE_HOST;
  firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);

  EEPROM.begin(512); // Initialize EEPROM with a size of 512 bytes
  totalRakaatSholat = EEPROM.read(0); // Read the stored value from address 0
  if (totalRakaatSholat == 255) { // If the value is 255, it means it's the first time the device is powered on
    totalRakaatSholat = 0; // Initialize the value to 0
  }
}

void loop() {
  checkButton();
  timeClient.update();

  DateTime now = timeClient.getEpochTime(); // Ambil waktu sekarang
  int currentHour = now.hour();
  int currentMinutes = now.minute();
  int currentSeconds = now.second();

  char hourStr[3], minuteStr[3], secondStr[3];

  if (currentHour < 10) {
    sprintf(hourStr, "0%d", currentHour);
  } else {
    sprintf(hourStr, "%d", currentHour);
  }

  if (currentMinutes < 10) {
    sprintf(minuteStr, "0%d", currentMinutes);
  } else {
    sprintf(minuteStr, "%d", currentMinutes);
  }

  if (currentSeconds < 10) {
    sprintf(secondStr, "0%d", currentSeconds);
  } else {
    sprintf(secondStr, "%d", currentSeconds);
  }

  String waktu = String(hourStr) + ":" + String(minuteStr) + ":" + String(secondStr);

  Serial.println(waktu);
  Serial.print("waktu saat ini: ");
  if (currentHour >= 4 && currentHour < 6) {
    Serial.println("Subuh");
    waktuSholat = "SUBUH";
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print(waktu);
    LCD.setCursor(8, 0);
    LCD.print("-Subuh");
    sensorDetection(2);
  } else if (currentHour >= 12 && currentHour < 15) {
    Serial.println("Dzuhur");
    waktuSholat = "DZUHUR";
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print(waktu);
    LCD.setCursor(8, 0);
    LCD.print("-Dzuhur");
    sensorDetection(4);
  } else if (currentHour >= 15 && currentHour <= 17) {
    Serial.println("Ashar");
    waktuSholat = "ASHAR";
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print(waktu);
    LCD.setCursor(8, 0);
    LCD.print("-Ashar");
    sensorDetection(4);
  } else if (currentHour >= 18 && currentHour < 19) {
    Serial.println("Maghrib");
    waktuSholat = "MAGHRIB";
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print(waktu);
    LCD.setCursor(8, 0);
    LCD.print("-Maghrib");
    sensorDetection(3);
  } else if (currentHour >= 19 && currentHour <= 24) {
    Serial.println("Isya");
    waktuSholat = "ISYA";
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print(waktu);
    LCD.setCursor(8, 0);
    LCD.print("-Isya");
    sensorDetection(4);
  }else{
    Serial.println("-");
    waktuSholat = "-";
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print(waktu);
    LCD.setCursor(8, 0);
    LCD.print("-");
    sensorDetection(4);
  }
  String tampilSholat = "Sujud:" + String(countSujud) + "-Rakaat:" + String(totalRakaah);
  LCD.setCursor(0, 1);
  LCD.print(tampilSholat);
  if (totalRakaatSholat <= 17){
    Serial.println(totalRakaatSholat);
  }else{
    Serial.println("Kelebihan");
  }
  Serial.println(totalRakaatSholat);
  EEPROM.write(0, totalRakaatSholat);
  EEPROM.commit();
  sendFirebase(waktuSholat, totalRakaah, totalRakaatSholat);
}

void sendFirebase(String waktuSholat, int totalRakaah, int totalRakaatSholat){
  if(Firebase.ready()){
    Firebase.set(firebaseData, "/Bacaan/waktuSholat", waktuSholat);
    Firebase.setFloat(firebaseData, "/Bacaan/RakaatSaatIni", totalRakaah);
    Firebase.setFloat(firebaseData, "/Bacaan/totalRakaatSholat", totalRakaatSholat);
  }else{
    Serial.println("Failed to Send Data to Firebase");
  }
}
