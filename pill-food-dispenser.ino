/*
* Name : Askinator
* Description: Pills dispenser
* Author: BARJO
* Version : 1.0
*/

// Display
#include <SPI.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
// Wifi
#include <WiFiManager.h>
// Time
#include <Time.h>
// EEPROM
#include <EEPROM.h>
// Servo
#include <ESP32Servo.h>

// define the number of bytes you want to access
#define EEPROM_SIZE 2

// Screen
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Time from the web
const char* NTP_SERVER = "ch.pool.ntp.org";
const char* TZ_INFO    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";  // enter your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)

tm timeinfo;
time_t now;
long unsigned lastNTPtime;
unsigned long lastEntryTime;

// Feed Time
int feedTimeHour = 6;
int feedTimeMinute = 30;
String feedTime = String(feedTimeHour) + ":" + String(feedTimeMinute);
bool feedTimeChange = false;

// Buttons
const int feedTimeBtn = 26;
const int plusBtn = 27;
int feedTimeBtnState = 0;
int plusBtnState = 0;
int increaseRange = 30; // time in minutes to add to feed time each time the "plus" button is pressed

// IR sensor
const int IRSensor = 33;
int IRState = 0;

// Mode
int displayMode = 0;

// Servo
Servo dogBowl;
const int dogBowlPin = 2;
static String rotationState = "Closed";
//int dogBownlAngle = 0;

// Icons
// 'dog-1', 32x32px
const unsigned char dog_dog_1 [] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe1, 0xff, 
	0xff, 0xff, 0xc0, 0x7f, 0xff, 0xff, 0x80, 0x0f, 0xff, 0xff, 0x80, 0x07, 0xff, 0xff, 0x80, 0x03, 
	0xff, 0xff, 0x04, 0x03, 0xff, 0xff, 0x0c, 0x03, 0xff, 0xff, 0x08, 0x07, 0xff, 0xff, 0x18, 0x0f, 
	0xff, 0xf9, 0xf0, 0x3f, 0xff, 0xe0, 0x00, 0xff, 0xff, 0xc0, 0x01, 0xff, 0xff, 0x80, 0x03, 0xff, 
	0xff, 0x00, 0x03, 0xff, 0xff, 0x00, 0x03, 0xff, 0xfe, 0x1e, 0x03, 0xff, 0xfe, 0x03, 0x81, 0xff, 
	0xfe, 0x00, 0xc1, 0xff, 0xde, 0x00, 0x40, 0xff, 0x9e, 0x00, 0x60, 0x7f, 0x9e, 0x00, 0x60, 0x3f, 
	0xcf, 0x00, 0x60, 0x0f, 0xc7, 0x00, 0x70, 0x0f, 0xe1, 0x00, 0x18, 0x07, 0xf0, 0x00, 0x08, 0x07, 
	0xfc, 0x00, 0x0c, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
// 'dog-2', 32x32px
const unsigned char dog_dog_2 [] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe1, 0xff, 
	0xff, 0xff, 0xc0, 0x7f, 0xff, 0xff, 0x80, 0x0f, 0xff, 0xff, 0x80, 0x07, 0xff, 0xff, 0x80, 0x03, 
	0xff, 0xff, 0x04, 0x03, 0xff, 0xff, 0x0c, 0x03, 0xff, 0xff, 0x08, 0x07, 0xff, 0xff, 0x18, 0x0f, 
	0xff, 0xf9, 0xf0, 0x3f, 0xff, 0xe0, 0x00, 0xff, 0xff, 0xc0, 0x01, 0xff, 0xff, 0x80, 0x03, 0xff, 
	0xff, 0x00, 0x03, 0xff, 0xff, 0x00, 0x03, 0xff, 0xfe, 0x1e, 0x03, 0xff, 0xfe, 0x03, 0x81, 0xff, 
	0xfe, 0x00, 0xc1, 0xff, 0xfe, 0x00, 0x40, 0xff, 0xfe, 0x00, 0x60, 0x7f, 0xfe, 0x00, 0x60, 0x3f, 
	0xff, 0x00, 0x60, 0x0f, 0xff, 0x00, 0x70, 0x0f, 0x80, 0x00, 0x18, 0x07, 0x00, 0x00, 0x08, 0x07, 
	0xf0, 0x00, 0x0c, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 144)
const int dog_allArray_LEN = 1;
const unsigned char* dog_allArray[2] = {
	dog_dog_1,
  dog_dog_2
};

// interrupt function
void IRAM_ATTR isr() {
  // change displayMode
	if (displayMode < 2) {
    displayMode++;
  } else {
    displayMode = 0;
  }
}

void setup() {
  Serial.begin(115200);

  // screen
  while (!Serial) {
    delay(100);
  }
  display.begin(SSD1306_SWITCHCAPVCC, OLED);

  // icon (animation)
  animator(1);

  // Wifi manager
  WiFiManager wm;
  bool res;
  //wm.resetSettings(); <- todo : reset button
  res = wm.autoConnect("Pills-dispenser","yourPassword"); // password protected ap

  if(!res) {
    Serial.println("Failed to connect");
    // clear display
    display.clearDisplay();

    // Show error on screen
    display.setTextSize(1); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Error");
    display.println("Wifi error");

    // Show on display
    display.display();
    delay(10000);
    display.clearDisplay();
  } 
  else {
    //if you get here you have connected to the WiFi    
    Serial.println("Successfully connected to the Wifi");
  }

  // Get Time on the internet
  configTime(0, 0, NTP_SERVER);
  // See https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv for Timezone codes for your region
  setenv("TZ", TZ_INFO, 1);

  if (getNTPtime(10)) {  // wait up to 10sec to sync
  } else {
    Serial.println("Time not set");
    ESP.restart();
  }
  showTime(timeinfo);
  lastNTPtime = time(&now);
  lastEntryTime = millis();

  // EEPROM get saved Feed Time
  //EEPROM.write(2, 7);
  //EEPROM.write(10, 30);
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(20, feedTimeHour);
  EEPROM.get(30, feedTimeMinute);
  feedTime = String(feedTimeHour) + ":" + String(feedTimeMinute);
  //Serial.println(feedTime);

  // Servo motor
  dogBowl.setPeriodHertz(50); // PWM frequency for SG90
  dogBowl.attach(dogBowlPin, 500, 2400);

  // IR sensor
  pinMode(IRSensor, INPUT);
  //rotationState = getRotationState();
  //if (rotationState == "Open")

  // setup buttons
  pinMode(feedTimeBtn, OUTPUT);
  pinMode(plusBtn, OUTPUT);

  attachInterrupt(feedTimeBtn, isr, RISING);
}

void loop() {

  //rotationState = getRotationState();

  // check if reach feed time
  checkFeedTime(timeinfo);

  // if Feed Time button pressed once -> show feed time (displayMode 1)
  if (displayMode == 1) {
    showFeedTime();
    // setup feed time
    plusBtnState = digitalRead(plusBtn);
    if (plusBtnState == HIGH) {
      increaseFeedTime();
      delay(250);
    }

    // if feed time button pressed again, save new feed time
    feedTimeBtnState = digitalRead(feedTimeBtn);
    if (feedTimeBtnState == HIGH) {

      if (feedTimeChange) {
        // save hours and minutes in EEPROM
        EEPROM.write(20, feedTimeHour);
        EEPROM.write(30, feedTimeMinute);

        boolean okCommit = EEPROM.commit();
        if (okCommit) {
          display.clearDisplay();
          display.setTextSize(1); // Draw 2X-scale text
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(0, 12);
          display.print("Change Saved");

          display.display();
          delay(1000);


          EEPROM.get(20, feedTimeHour);
          EEPROM.get(30, feedTimeMinute);

          Serial.println(feedTimeHour);
          Serial.print(":");
          Serial.print(feedTimeMinute);
        } else {
          // error while saving in EEPROM
          display.clearDisplay();
          display.setTextSize(1); // Draw 2X-scale text
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(0, 12);
          display.println("Error while saving");
          display.println("Feed Time");

          display.display();
          delay(5000);
        }
        feedTimeChange = false;
        displayMode = 0;

      }
      delay(250);
    }

  } else if (displayMode == 2) {
    rotationState = getRotationState();
    // rotation mode
    display.clearDisplay();
    display.setTextSize(2); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 6);
    display.print(String(rotationState));

    // legend
    display.setTextSize(1); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 24);
    display.println("PRESS + TO ROTATE");

    display.display();
    delay(100);

    // rotate
    plusBtnState = digitalRead(plusBtn);
    if (plusBtnState == HIGH) {
      // rotation mode
      display.clearDisplay();
      display.setTextSize(2); // Draw 2X-scale text
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 6);
      display.print("Rotation");

      // legend
      display.setTextSize(1); // Draw 2X-scale text
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 24);
      display.println("PLEASE WAIT...");

      display.display();

      rotateDogBowl();
      delay(1000);

    }

  } else {
    // show current time
    getNTPtime(10);
    showTime(timeinfo);
    delay(100);
  }


}


// Animations
void animator(int animNumber) {
  switch(animNumber) {
    case 1 :

      for (int i=0; i<4 ; i++) {
        display.clearDisplay();
        display.drawBitmap(0, 2, dog_dog_1, 32, 32, WHITE);

        display.setTextSize(1); // Draw 2X-scale text
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(48, 14);
        display.println("ASKINATOR");
        //display.setCursor(48, 20);
        //display.println("barjo|v1.0");

        display.display();
        delay(500);

        display.clearDisplay();
        display.drawBitmap(0, 2, dog_dog_2, 32, 32, WHITE);

        display.setTextSize(1); // Draw 2X-scale text
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(48, 14);
        display.println("ASKINATOR");
        //display.setCursor(48, 20);
        //display.println("barjo|v1.0");

        display.display();
        delay(500);
        
      }
      break;
  }
}

// get time from the web
bool getNTPtime(int sec) {

  {
    uint32_t start = millis();
    do {
      time(&now); // request only every hour
      localtime_r(&now, &timeinfo);
      Serial.print(".");
      delay(10);
    } while (((millis() - start) <= (1000 * sec)) && (timeinfo.tm_year < (2016 - 1900)));
    if (timeinfo.tm_year <= (2016 - 1900)) return false;  // the NTP call was not successful
    //Serial.print("now ");  Serial.println(now);
    char time_output[30];
    strftime(time_output, 30, "%a  %d-%m-%y %T", localtime(&now));
    //Serial.println(time_output);
    //Serial.println();
  }
  return true;
}
// Show time frome the web
void showTime(tm localTime) {
  /*Serial.print(localTime.tm_mday);
  Serial.print('/');
  Serial.print(localTime.tm_mon + 1);
  Serial.print('/');
  Serial.print(localTime.tm_year - 100);
  Serial.print('-');
  Serial.print(localTime.tm_hour);
  Serial.print(':');
  Serial.print(localTime.tm_min);
  Serial.print(':');
  Serial.print(localTime.tm_sec);*/


  // show local time (web)
  display.clearDisplay();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 6);

  String localTimeHours = String(localTime.tm_hour);
  String localTimeMinutes = String(localTime.tm_min);

  // add 0 in front if less than 10
  if (localTime.tm_hour < 10) {
    localTimeHours = "0" + localTimeHours;
  }
  if (localTime.tm_min < 10) {
    localTimeMinutes = "0" + localTimeMinutes;
  }

  display.print(localTimeHours);
  display.print(":");
  display.print(localTimeMinutes);

  // legend
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 24);
  display.println("CURRENT TIME");

  display.display();
  //currentTime = localTime.tm_hour + ":" + localTime.tm_min;
  //Serial.print(" Day of Week ");
  //if (localTime.tm_wday == 0)   Serial.println(7);
  //else Serial.println(localTime.tm_wday);
}

void showFeedTime() {
  String feedTimeHourDisplay = String(feedTimeHour);
  String feedTimeMinuteDisplay = String(feedTimeMinute);
  // add 0 in front if less than 10
  if (feedTimeHour < 10) {
    feedTimeHourDisplay = "0" + feedTimeHourDisplay;
  }
  if (feedTimeMinute < 10) {
    feedTimeMinuteDisplay = "0" + feedTimeMinuteDisplay;
  }
  feedTime = String(feedTimeHourDisplay) + ":" + String(feedTimeMinuteDisplay);
  
  // show feed time
  display.clearDisplay();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 6);
  display.print(String(feedTime));

  // legend
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 24);
  display.println("FEED TIME");

  display.display();
  delay(100);
}

void increaseFeedTime() {
  feedTimeChange = true;
  if (feedTimeMinute == 0) {
    feedTimeMinute = 30;
  } else {
    feedTimeMinute = 0;
    // add 1 hour if less than 23h, else -> restart to 0
    if (feedTimeHour < 23) {
      feedTimeHour += 1;
    } else {
      feedTimeHour = 0;
    }
  }
  showFeedTime();
}

void rotateDogBowl() {
  rotationState = getRotationState();
  if (rotationState == "Closed") {
    rotateForward();
  } else {
    rotateBackward();
  }
  delay(250);
}

void rotateForward() {
   //rotation from 0 to 180°
   rotationState = getRotationState();
   if (rotationState == "Closed") {
      for (int pos = 0; pos <= 180; pos += 1) {
        dogBowl.write(pos);
        delay(10);
      }
   } else {
    Serial.println("can't rotate forward when open");
   }
}

void rotateBackward() {
  // Rotation from 180° to 0
  rotationState = getRotationState();
  if (rotationState == "Open") {
    for (int pos = 180; pos >= 0; pos -= 1) {
      dogBowl.write(pos);
      delay(10);
    }
  } else {
    Serial.println("can't rotate backward when closed");
   }
}

void checkFeedTime(tm localTime) {
  /*Serial.println(localTime.tm_hour);
  Serial.print(":");
  Serial.print(localTime.tm_min);*/

  if (localTime.tm_hour == feedTimeHour && localTime.tm_min == feedTimeMinute) {
    rotationState = getRotationState();

    if (rotationState == "Closed") {
      rotateForward();

      // message for the dog
      display.clearDisplay();
      display.setTextSize(2); // Draw 2X-scale text
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 6);
      display.print("C'mon Dog!");

      // legend
      display.setTextSize(1); // Draw 2X-scale text
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 24);
      display.println("FEED TIME");

      display.display();
      delay(62000); // wait 62 sec to make sure it wont happens 2 times in the same day
    }
  }
}

String getRotationState() {
  //rotationState = "Closed";
  IRState = digitalRead(IRSensor);
  if (IRState == LOW) {
    rotationState = "Closed";
  } else {
    rotationState = "Open";
  }
  return rotationState;
}


