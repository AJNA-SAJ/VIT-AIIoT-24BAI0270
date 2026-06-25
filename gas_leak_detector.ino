#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define GAS_SENSOR_PIN 34 //GPIO34 for analog input from MQ-2
#define RED_LED_PIN 19    //GPIO18 for red LED
#define GREEN_LED_PIN 18  //GPIO19 for green LED
#define BUZZER_PIN 23     //GPIO23 for buzzer
 
//Threshold value for gas detection(adjust as necessary)
#define GAS_THRESHOLD 2600

//Thingspeak wifi settings
const char* ssid= "Wokwi-GUEST";
const char* password= ""; //empty password
const char* thingspeakApiKey= "CGZWPW33WOQOF32V"; 
const char* thingspeakServer= "http://api.thingspeak.com/update";

unsigned long lastUploadMillis = 0;
const unsigned long uploadInterval = 15000;

LiquidCrystal_I2C lcd(0x27, 16, 2); //Initialize the LCD(I2C address may vary)

void connectWiFi(){
  Serial.print("Connecting to WiFi SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  unsigned long start= millis();
  while(WiFi.status()!= WL_CONNECTED && millis() - start < 15000){ // wait up to 15s
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if(WiFi.status() == WL_CONNECTED){
    Serial.println("WiFi connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else{
    Serial.println("WiFi connection failed.");
  }
}

void setup(){
  Serial.begin(115200); //Start Serial communication
  lcd.init();        //Initialize the LCD 
  lcd.backlight();      //Turn on the LCD backlight

  pinMode(RED_LED_PIN,OUTPUT);
  pinMode(GREEN_LED_PIN,OUTPUT);
  pinMode(BUZZER_PIN,OUTPUT);
  
  //Initial state
  digitalWrite(RED_LED_PIN,LOW);
  digitalWrite(GREEN_LED_PIN,HIGH);
  digitalWrite(BUZZER_PIN,LOW);
  
  lcd.print("Leakage Detector");
  delay(2000);
  lcd.clear();

  connectWiFi();
}

void uploadToThingSpeak(int gasValue, int status){
  if(WiFi.status()!= WL_CONNECTED){
    Serial.println("Not connected to WiFi. Attempting reconnect...");
    connectWiFi();
    if(WiFi.status()!= WL_CONNECTED){
      Serial.println("Skipping ThingSpeak upload - no WiFi.");
      return;
    }
  }
  //Prepare URL:
  String url= String(thingspeakServer) + "?api_key=" + thingspeakApiKey
               + "&field1=" + String(gasValue)
               + "&field2=" + String(status);
  Serial.print("ThingSpeak URL: ");
  Serial.println(url); //helpful for debugging
  HTTPClient http;
  http.begin(url);
  int httpCode= http.GET();
  Serial.print("ThingSpeak HTTP response code: ");
  Serial.println(httpCode);
  if(httpCode > 0){
    //Optionally read response payload (ThingSpeak typically returns entry id or 0)
    String payload = http.getString();
    Serial.print("ThingSpeak response payload: ");
    Serial.println(payload);
  } else{
    Serial.print("HTTP GET failed, error: ");
    Serial.println(httpCode);
  }
  http.end();
}

void loop(){
  int gasValue= analogRead(GAS_SENSOR_PIN); //Read gas value from MQ-2
  Serial.print("Gas Value: ");
  Serial.println(gasValue); //Print gas value to Serial Monitor

  int status= 0; //0=Safe,1=Leak

  //Check gas concentration
  if(gasValue > GAS_THRESHOLD) {
    status= 1;
    //Gas level high
    digitalWrite(RED_LED_PIN,HIGH); //Turn on red LED
    digitalWrite(GREEN_LED_PIN,LOW); //Turn off green LED
    digitalWrite(BUZZER_PIN,HIGH); //Activate buzzer
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Gas Detected!");
  } else{
    status= 0;
    //Gas level safe
    digitalWrite(RED_LED_PIN,LOW); //Turn off red LED
    digitalWrite(GREEN_LED_PIN,HIGH); //Turn on green LED
    digitalWrite(BUZZER_PIN,LOW); //Deactivate buzzer
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("All Clear       ");
  }

  lcd.setCursor(0,1);
  lcd.print("Gas: ");
  lcd.print(gasValue);
  lcd.print("    ");

  //Upload to ThingSpeak every 15 seconds (respecting free-tier limits)
  unsigned long now= millis();
  if(now - lastUploadMillis >= uploadInterval){
    lastUploadMillis= now;
    uploadToThingSpeak(gasValue,status);
  }

  delay(100); 
}

