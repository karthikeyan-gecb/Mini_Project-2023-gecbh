#define ARDUINO_ESP32_CPP_FEATURES 1
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>
#include <string.h>
#include "mbedtls/md.h"

const int relay_pin = 15;
#define SS_PIN 5
#define RST_PIN 22

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Network credentials
#define WIFI_SSID "Redmi !11"
#define WIFI_PASSWORD "vinukichu"

// Firebase project API Key
#define API_KEY "AIzaSyBrbSMARh4HjB75NEP0cGOSEXO15oVS4a4"

// Authorized Username and Corresponding Password
#define USER_EMAIL "vinayakmanoj3112@gmail.com"
#define USER_PASSWORD "vinukichu"

// RTDB URLefine the RTDB URL 
#define DATABASE_URL "https://demo3-10c60-default-rtdb.asia-southeast1.firebasedatabase.app/"

//change default i2c pins of esp32
#define I2C_SDA_PIN 16 // Custom SDA pin
#define I2C_SCL_PIN 17 // Custom SCL pin

//setting the time 
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 16200 ;
const int   daylightOffset_sec = 3600;

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config; 

// Variables to save database paths
String listenerPath = "board1/outputs/digital/";
String readPath = "board1/Read/" ;
String writePath = "board1/write/";

MFRC522 mfrc522(SS_PIN, RST_PIN); //RC522 

//lcd initatilisation
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x3F, lcdColumns, lcdRows);  

//keypad
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};  

byte rowPins[ROWS] = {32, 33, 25, 26}; 
byte colPins[COLS] = {27, 14, 12, 13}; 

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

//Function declaration
void scanRfid(void);
void read_data(void); 
void write_data(void);
void printLocalTime(void);
void read(void);
void hash(void);
void pushButton(void);

//Global Variables
unsigned long timestamp = 0;
int count = 0;
bool signupOK = false;
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
String uid;
bool rfid = true; 
bool entry; 
const int len_key = 4;
char attempt_key[len_key];
int z=0; 
String pinValue;
int regValue;
struct tm timeinfo;
String date_time;
const int output1 = 2; 
int state; 
byte hashValue[32];
String Name;
const int buttonPin=34;
int buttonState;



void setup(){

  Serial.begin(115200);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println(); 

  pinMode(output1, OUTPUT);
  pinMode(relay_pin, OUTPUT);
  pinMode(buttonPin,INPUT);
  digitalWrite(relay_pin, HIGH);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

   /* Sign up */
 Firebase.reconnectWiFi(true);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
 
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  

}

void loop(){ 

  read();

}

void read(){ 

  

  if (Firebase.ready()) {
    if (Firebase.RTDB.getInt(&fbdo, listenerPath + "/2")) {
      if (fbdo.dataType() == "int") {
        state = fbdo.intData();
        
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }

    if (state == 1){ 

      digitalWrite(output1, HIGH);
    }

    else{
       
      digitalWrite(output1, LOW);
      pushButton();

    }
  } 
}

void scanRfid() {

  lcd.setCursor(0, 0); 
  lcd.print("Swipe Tag");
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  // Show UID on the serial monitor
  Serial.print("UID tag: ");
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();

  content.toUpperCase();

  printLocalTime(); 

  String content_array[] = {"F3 76 81 A4", "73 83 30 AB", "E3 DC 5E A4", "63 DE 5D A4", "B3 8F 9C A9"};
  
  for (int i = 0; i < sizeof(content_array); i++) {
    if (content.substring(1) == content_array[i]) {
      rfid = false;
      uid = content_array[i];
      timestamp = millis();
      delay(1000);
      keyPad();
      break;
    }

    else {
      rfid = true;
      Serial.println("Invalid tag");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Invalid tag");
      delay(1000);
      lcd.clear();
      delay(3000);
      entry = false; 
      write_data();
    }
  }
}

void keyPad(){ 

    Serial.println("Enter your Pin");
    // lcd.clear();
    lcd.setCursor(0, 0); 
    lcd.print("Enter your Pin"); 
    delay(1000); 
    lcd.clear();
    

    while (!rfid){ 
    char key = keypad.waitForKey(); 
    if (key){ 
      switch(key){ 
        case '*': 
        z=0; 
        break; 
        default: 
        // lcd.clear();
        Serial.print("*"); 
        lcd.setCursor(0, 0); 
        lcd.print("."); 
        attempt_key[z]=key; 
        z++; 
        if (z == len_key){ 
          read_data(); 
        }
        }
      }
    }
}

void read_data(){

  // Serial.println();
  if (Firebase.ready()) {
    if (Firebase.RTDB.getString(&fbdo, readPath + uid + "/Name")) {
      if (fbdo.dataType() == "string") {
        Name = fbdo.stringData();
        Serial.println(Name);
      }  
    }
    else {
      Serial.println(fbdo.errorReason());
    }

     
    if (Firebase.RTDB.getString(&fbdo, readPath + uid + "/PIN")) {
        pinValue = fbdo.stringData();
        // pinValue.toCharArray(pinValueArray, sizeof(pinValueArray));
        Serial.println(pinValue);
      
    }
    else {
      Serial.println(fbdo.errorReason());
    }

     if (Firebase.RTDB.getInt(&fbdo, readPath + uid + "/Reg No")) {
      if (fbdo.dataType() == "int") {
        regValue = fbdo.intData();
        Serial.println(regValue);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    } 
    
    hash(); 

  }
}

//  void check_key()
// { 

//   const char* expectedHash = pinValue.c_str();
  
//   if (strcmp((const char*)hashValue, expectedHash) == 0) {
//     Serial.println("Access Granted!");
//     z=0; 
//     Serial.println();
//     Serial.println("Access Granted");  
//     lcd.clear(); 
//     delay(1000); 
//     lcd.setCursor(0, 0);
//     lcd.print("Access Granted");

//     //  digitalWrite(relay_pin, HIGH);
     
//      delay(1000);

//      relay();

//      delay(1000);

//      lcd.clear();
     
//      rfid= true;
//      entry = true;
//      write_data();

//   } 
  
//   else {
//     Serial.println("Access Denied");
//     lcd.setCursor(0, 0);
//     lcd.print("Accesss Denied");
//     delay(1000);
//     lcd.clear();
//     z=0; 

//     rfid = true;
//     entry = false;
//     write_data();  
//   }

//     for (int zz=0; zz<len_key; zz++) {
//     attempt_key[zz]=0;
//    }
//  }

 void write_data(){

if(entry){

   if (Firebase.ready()) {

    // Write an Int number on the database path test/int
    if (Firebase.RTDB.setInt(&fbdo, writePath + date_time + "/Reg No", regValue)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    

    if (Firebase.RTDB.setString(&fbdo, writePath + date_time + "/Entry", "Granted")){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setString(&fbdo, writePath + date_time + "/Swipe Time", uid)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
   }
}

  if(!entry){

     if (Firebase.ready()) {

    if (Firebase.RTDB.setInt(&fbdo, writePath + date_time + "/Reg No", regValue)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setString(&fbdo, writePath + date_time  + "/Entry", "Denied")){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setString(&fbdo, writePath + date_time + "/UID", uid)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
 }
}

void printLocalTime(){
  
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }

  int year = timeinfo.tm_year + 1900;
  int month = timeinfo.tm_mon + 1;
  int day = timeinfo.tm_mday;
  int hour = timeinfo.tm_hour;
  int minute = timeinfo.tm_min;
  int second = timeinfo.tm_sec;

  String date; 
  String time;
  date_time= String(year) + "-" + String(month) + "-" + String(day) + " " + String(hour) + ":" + String(minute) + ":" + String(second);
  Serial.print(date_time);
}

void relay() { 

  Serial.println("KEEDANU");

  digitalWrite(relay_pin, LOW); 

  delay(5000); 

  Serial.println("SAI KOZHI");

  digitalWrite(relay_pin, HIGH);
  
}

void hash(){

 String combinedNum;

  for (int j=0; j<len_key; j++)
  { 
      combinedNum += String(attempt_key[j]);
  } 

  int pin_check = combinedNum.toInt(); 


char x=pin_check;

char payload[5];
sprintf(payload, "%d", pin_check);

byte shaResult[32];

mbedtls_md_context_t ctx;
mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

const size_t payloadLength = strlen(payload);

mbedtls_md_init(&ctx);
mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
mbedtls_md_starts(&ctx);
mbedtls_md_update(&ctx, (const unsigned char *) payload, payloadLength);
mbedtls_md_finish(&ctx, shaResult);
mbedtls_md_free(&ctx);

Serial.print("Hash: ");

for(int i= 0; i< sizeof(shaResult); i++)
{
char str[3];
sprintf(str, "%02x", (int)shaResult[i]);
Serial.print(str);
}

  String hashString;
  for (int i = 0; i < sizeof(shaResult); i++) {
    char str[3];
    sprintf(str, "%02x", (int)shaResult[i]);
    hashString += str;
  }

  // Compare the hash string with pinValue
  if (hashString.equals(pinValue)) {
    Serial.println("Hash matched");
    Serial.println("Access Granted!");
    z=0; 
    Serial.println();
    Serial.println("Access Granted");  
    lcd.clear(); 
    delay(1000); 
    lcd.setCursor(0, 0);
    lcd.print("Access Granted");

    //  digitalWrite(relay_pin, HIGH);
     
     delay(1000);

     relay();

     delay(1000);

     lcd.clear();
     
     rfid= true;
     entry = true;
     write_data();

    // The hash matches pinValue
  } else {
    Serial.println("Hash does not match");
     Serial.println("Access Denied");
    lcd.setCursor(0, 0);
    lcd.print("Accesss Denied");
    delay(1000);
    lcd.clear();
    z=0; 

    rfid = true;
    entry = false;
    write_data();  

    // The hash does not match pinValue
  }

  for (int zz=0; zz<len_key; zz++) {
    attempt_key[zz]=0;
   }
}

void pushButton(){
  buttonState = digitalRead(buttonPin);
  Serial.println(buttonState);
  if (buttonState == HIGH) {
    relay();
    
  } 
  else{
    scanRfid();
  }
}
  



