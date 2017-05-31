/*
 * LIST LIBRARIES USED
 */
#include <TimeLib.h>            // Time library
#include <TimeAlarms.h>         // Library for Set alarm. By : Michael Margolis 2008-2011
#include <Wire.h>               // I2C library
#include <OneWire.h>            // 1-Wire library
#include <DS1307RTC.h>          // RTC DS1307 library. By : Michael Margolis 2009
#include <dht.h>                // all DHT library. By : Rob Tillaart 2011
#include <DallasTemperature.h>  // DS18B20 library. By : Miles Burton, Tim Newsome, and Guil Barros
#include <Adafruit_ADS1015.h>   // ADS1115 16 bit ADC library. By : K. Townsend (Adafruit Industries)
#include <Servo.h>              // Servo library
#include <NewPing.h>            // Ultrasonic Range Sensor library. By : Tim Eckel 2015
#include <MQ2.h>                // MQ2 gas sensor library.
#include <LiquidCrystal_I2C.h>  // LCD library using I2C communication. By Francisco Malpartida 2011
#include "pitches.h"


/* DEFINE PARAMETER FOR SENSOR AND COMPONENTS */

//ADS1115
Adafruit_ADS1115 ads;

//D18B20
#define ONE_WIRE_BUS 8
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

//DHT11
dht DHT;
#define DHT11_PIN 5

//ULTRASONIC RANGE - PING
#define TRIGGER_PIN  6  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     7  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

//MQ02
MQ2 mq2(A3);

//ALARM FOR SHOWERING THE PLANT
AlarmId id;

//RTC DS1307
tmElements_t tm;

//LCD I2C PACK
//Set the LCD I2C address. To insure the address is right, please check with I2C_scanner. Available at Arduino Playground
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

//SERVO
Servo servo;

//GLOBAL VARIABEL
byte distance;
int smoke;
int16_t adc0, adc1, adc2, adc3;
float water, soil, termistor, light, lm35, temp, humid, vibration, water_temp;
double ads_max = 32767.0;
String dates, data;

void setup() {
  //SENSORS INITIALIZATION
  ads.begin();          // ADS1115
  mq2.begin();          // MQ02
  sensors.begin();      // DS18B20

  //OUTPUT
  pinMode(2, OUTPUT);   // BUZZER
  pinMode(3, OUTPUT);   // RED LED
  pinMode(4, OUTPUT);   // GREEN LED
  pinMode(10, OUTPUT);  // RELAY LED STRIP
  pinMode(11, OUTPUT);  // RELAY PUMP
  servo.attach(9);      // SERVO
  Serial.begin(9600);   // SERIAL COMMUNICATION
  lcd.begin(16, 2);     // LCD INITIALISATION

  //WELCOME
  lcd.setCursor(0, 0);
  lcd.print(F(" SMART FLOATING"));
  lcd.setCursor(6, 1);
  lcd.print(F("FARM"));
  delay(1000);
  lcd.clear();

  //RTC INITIALISATION
  if (RTC.read(tm)) {
    setSyncProvider(RTC.get);
    dates = print2digit(tm.Day);
    lcd.setCursor(3, 0);
    lcd.print(dates);
    lcd.print("/");
    dates = print2digit(tm.Month);
    lcd.print(dates);
    lcd.print("/");
    lcd.print(String(tmYearToCalendar(tm.Year)));
    lcd.setCursor(4, 1);
    dates = print2digit(tm.Hour);
    lcd.print(dates);
    lcd.print(":");
    dates = print2digit(tm.Minute);
    lcd.print(dates);
    lcd.print(":");
    dates = print2digit(tm.Second);
    lcd.print(dates);
    digitalWrite(4, HIGH);
    digitalWrite(3, LOW);
    delay(1000);
    lcd.clear();
  }
  else {
    lcd.setCursor(0, 0);
    lcd.print(F("Clock chip ERROR"));
    lcd.print(F("Contact CS"));
    digitalWrite(3, HIGH);
    digitalWrite(4, LOW);
    return;
  }


  /* CHECK NETWORK */



  //SET TIME FOR SHOWERING THE PLANT
  Alarm.timerRepeat(7, 00, 0, siram);    // timer for every 1 seconds
  Alarm.timerRepeat(17, 00, 0, siram);
  //Alarm.timerRepeat(1 * 60, ambildata);
}

void loop() {

  unsigned long startTime = 0;
  startTime = millis();

  //READ SENSOR
  light = analogRead(A0) / 1023.0 * 100.0;
  vibration = analogRead(A1) / 1023.0 * 100.0;
  adc0 = ads.readADC_SingleEnded(0);
  adc1 = ads.readADC_SingleEnded(1);
  adc2 = ads.readADC_SingleEnded(2);
  adc3 = ads.readADC_SingleEnded(3);
  water = adc0 / ads_max * 100.0;
  soil = adc1 / ads_max * 100.0;
  termistor = adc2 / ads_max * 100.0;
  lm35 = 6.144 * adc3 * 100.0 / ads_max;
  distance = sonar.ping_cm();
  smoke = mq2.readSmoke();

  //DHT11
  int chk = DHT.read11(DHT11_PIN);
  while (chk < 0) {
    chk = DHT.read11(DHT11_PIN);
    if (chk == 0) {
      break;
    }
  }
  humid = DHT.humidity;
  temp = DHT.temperature;

  //DS18B20
  sensors.requestTemperatures();
  water_temp = sensors.getTempCByIndex(0);

  //GET TIME
  RTC.read(tm);

  //SEND DATA
  data = '|' + String (tmYearToCalendar(tm.Year)) + '/' + String (tm.Month) + '/' + String (tm.Day);
  data = data + ' ' + String (tm.Hour) + ':' + String (tm.Minute) + ',' ;
  data = data + String (distance) + ',' + String (smoke)  + ',' + String (water) + ',' ;
  data = data + String (soil) + ',' + String (termistor) + ',' + String (light) + ',' + String (lm35);
  data = data + ',' + String (temp) + ',' + String (humid) + ',' + String (vibration) + String (water_temp) + '|';

  //CHECK TIME CONSUME FOR DO THIS PROGRAM
  Serial.println(millis() - startTime);
  delay(1000);

  //CHECK THRESHOLD
  if (water < 50.0 || soil < 50.0 || lm35 > 18.0)
  {
    siram();
  }

  if (light < 30.0) // TURN ON LED
  {
    digitalWrite(10, HIGH);
  }

  if (temp > 30 || humid > 75) //OPEN ROOF
  {
    servo.write(90);
  }
  if (temp <= 30 || humid <= 75) //CLOSE ROOF
  {
    servo.write(0);
  }
  if (vibration > 50.0 || smoke > 50.0) //TURN ON BUZZER FOR WARNING
  {
    tone(2, NOTE_G7, 1000);
    delay(2000);
    noTone(2);
  }
}


String print2digit(byte num) {
  String data;
  if (num >= 0 && num < 10)
  {
    data = '0' + String(num);
  }
  else
  {
    data = String(num);
  }
  return data;
}

void siram()
{
  for (byte i = 0; 1 < 6; i++) {
    digitalWrite(11, HIGH);
    delay(8 * 1000);
    digitalWrite(11, LOW);
    delay(2 * 1000);
  }
}

void ambildata()
{

}
