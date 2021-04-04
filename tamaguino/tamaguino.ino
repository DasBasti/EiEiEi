/* Tamaguino
  by Alojz Jakob <http://jakobdesign.com>
  modified by TheBrutzler

 ********** TAMAGUINO ***********
   Tamagotchi clone for Arduino
 ********************************

*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <ArduinoUniqueID.h>

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "animations.h"
#include "settings.h"

//ESP32 Sleep
#include <esp_wifi.h>
#include <esp_bt.h>
#include "esp_deep_sleep.h"
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define uS_TO_mS_FACTOR 1000ULL   /* Conversion factor for micro seconds to milli seconds */
#define TIME_TO_SLEEP 5           /* Time ESP32 will go to sleep (in seconds) */
RTC_DATA_ATTR int bootCount = 0;
//#define SLEEPDELAY
const int fixPointFactor = 1000;
// Pins
//const int button1Pin = 21;
//const int button2Pin = 22;
//const int button3Pin = 19;
const int soundPin = 23;

// ESP32 "LED" Channel for sound
#define SOUND_CHAN 0

// WifiKit32 OLED SDA is 4, and the SCL is 15
//#define OLED_RESET 16
#define OLED_SUPPLY_nEN 16
#define OLED_SDA 0
#define OLED_SCL 4
Adafruit_SSD1306 display(128, 64, &Wire); //, OLED_RESET);

//Adafruit_SSD1306 display(OLED_RESET);
int button1State = 0;
int button2State = 0;
int button3State = 0;

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

// Adafruit IO
const char MQTT_SERVER[] = "cloud.eieiei.lol";
const int MQTT_SERVERPORT = 1883;
char MQTT_CLIENTID[21] = {};

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_CLIENTID);
Adafruit_MQTT_Publish *devPing;
Adafruit_MQTT_Subscribe *devSubscribe;

// Walking
int walkPos = 0;
int walkXPos = 0;
bool walkAnimReverse = false;
bool walkRight = false;
int walkDirOffset = 2;

// Ground
int grassXPos = 0;
float treesXPos = -20;
//sky
float couldsXPos = 0;
const int sunRadius = 3;
bool sunOrMoon = false;
const int moonShadow = 2;
float sunXPos = -2 * sunRadius;
//clouds
const int cloud1Width = 32;
float cloud1XPos = display.width() + cloud1Width;

int stars[6][2];
int lastCloud = 0;
// menus
bool menuOpened = false;
int menu = 0;
int subMenu = 1;
bool menuDepth = false;
bool justOpened = false;
#define MENUSIZE 8
#define STRING_SIZE 14
const char mainMenu[MENUSIZE][8][STRING_SIZE] PROGMEM = {
    {"Versorgen", "Apfel", "Steak", "Wasser", NULL},
    {"Spiel", NULL},
    {"Schlafen", NULL},
    {"Sauber machen", NULL},
    {"Arzt", NULL},
    {"Trainieren", NULL},
    {"Status", "Hunger", "Zufrieden", "Gesundheit", "Verhalten", "Gewicht", "Alter", NULL},
    {"Einstellung", "Ton",
     //"something",
     NULL},
};

/* ------- PET STATS ------- */
float hunger = 100;
float happiness = 100;
float health = 100;
float discipline = 100;
float weight = 1;
float age = 0;

//settings
bool soundEnabled = true;

int action = 0;
int setting = 0;

bool notification = false;
int notificationBlink = 0;
bool dead = false;

bool sleeping = false;

//game
bool game = false;
bool paused = false;
bool gameOver = false;
int score = 0;
int hiScore = 0;
int level = 0;
bool newHiScore = false;
bool jumping = false;
bool jumpUp = true;
int jumpPos = 0;
bool obstacle1show = false;
bool obstacle2show = false;
int obstacle1XPos = 0;
int obstacle2XPos = 0;

float poopometer = 0;
int poops[3] = {
    0, 0, 0};

void setup()
{
  Serial.begin(115200);
  UniqueIDdump(Serial);

  String id;
  for (size_t i = 0; i < 8; i++)
    id += String(UniqueID[i], HEX);
  id.toCharArray(MQTT_CLIENTID, 16);
  devPing = new Adafruit_MQTT_Publish(&mqtt, "dino/in");
  String subPath = "dino/" + id;
  devSubscribe = new Adafruit_MQTT_Subscribe(&mqtt, subPath.c_str());

  Serial.print(F("Connecting to "));
  Serial.println(ssid);
  WiFi.disconnect();
  WiFi.setSleep(false);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname("EiEiEi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(F("."));
  }
  Serial.println();

  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());
  ArduinoOTA.setHostname(PET_NAME);

  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });

  ArduinoOTA.begin();

  //Subscribe to server notifications
  mqtt.subscribe(devSubscribe);

  // connect to mqtt
  Serial.print(F("Connecting to Cloud... "));

  int8_t ret;

  while ((ret = mqtt.connect()) != 0)
  {

    yield();
    switch (ret)
    {
    case 1:
      Serial.println(F("Wrong protocol"));
      break;
    case 2:
      Serial.println(F("ID rejected"));
      break;
    case 3:
      Serial.println(F("Server unavail"));
      break;
    case 4:
      Serial.println(F("Bad user/pass"));
      break;
    case 5:
      Serial.println(F("Not authed"));
      break;
    case 6:
      Serial.println(F("Failed to subscribe"));
      break;
    default:
      Serial.println(F("Connection failed"));
      break;
    }

    if (ret >= 0)
      mqtt.disconnect();

    Serial.println(F("Retrying connection..."));
    delay(200);
  }

  Serial.println(F("Cloud Connected!"));

  pinMode(OLED_SUPPLY_nEN, OUTPUT);
  digitalWrite(OLED_SUPPLY_nEN, LOW);
  Wire.begin(OLED_SDA, OLED_SCL);
  //pinMode(button2Pin, INPUT);
  //pinMode(button3Pin, INPUT);

  // Setting up sound
  // see https://github.com/espressif/arduino-esp32/issues/1720#issuecomment-410275623
  ledcSetup(SOUND_CHAN, 1E5, 12);
  ledcAttachPin(soundPin, SOUND_CHAN);

  //pinMode(13,OUTPUT);

  randomSeed(analogRead(0));

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false);
  display.dim(0);
  display.clearDisplay();
  int x = 0;
  int y = 0;
  while (y < 255)
  {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print(F(" Junkies Easteregg "));
    display.drawBitmap(34, 10, block[x], 54, 54, WHITE);
    display.display();
#ifdef SLEEPDELAY
    DelayLightSleep(100);
#else
    delay(100);
#endif
    if (x == 13)
      x = 0;
    else
      x++;
    y++;
  }

  //splash tone

  esp32tone(500, 200);
#ifdef SLEEPDELAY
  DelayLightSleep(200);
#else
  delay(200);
#endif
  esp32tone(1000, 200);
#ifdef SLEEPDELAY
  DelayLightSleep(400);
#else
  delay(400);
#endif
  esp32tone(700, 200);
#ifdef SLEEPDELAY
  DelayLightSleep(200);
#else
  delay(200);
#endif
  esp32tone(1100, 200);

#ifdef SLEEPDELAY
  DelayLightSleep(2200);
#else
  delay(2200);
#endif
  // end splash

  display.clearDisplay();
  // ESP32 save current
  //esp_bt_controller_disable();
  //esp_wifi_stop();

  // ESP32 Timer wakeup
  esp_sleep_enable_timer_wakeup(50 * uS_TO_mS_FACTOR);
  // ESP32 Tocuh wakeup
  //esp_sleep_get_touchpad_wakeup_status();
}

void loop()
{

  ArduinoOTA.handle();
#ifdef SLEEPDELAY
  DelayLightSleep(50);
#else
  delay(50);
#endif
  // Esp32 Sleep
  //esp_light_sleep_start();
  //DelayLightSleep(50);

  int touch1 = 0;
  int touch2 = 0;
  int touch3 = 0;

  touch1 += touchRead(T5);
  touch2 += touchRead(T6);
  touch3 += touchRead(T7);
  touch1 += touchRead(T5);
  touch2 += touchRead(T6);
  touch3 += touchRead(T7);
  touch1 += touchRead(T5);
  touch2 += touchRead(T6);
  touch3 += touchRead(T7);
  touch1 = touch1 / 3;
  touch2 = touch2 / 3;
  touch3 = touch3 / 3;
  if (touch1 < 20)
    button1State = true;
  else
    button1State = false;
  if (touch2 < 20)
    button2State = true;
  else
    button2State = false;
  if (touch3 < 20)
    button3State = true;
  else
    button3State = false;
  //button1State = digitalRead(button1Pin);
  //button2State = digitalRead(button2Pin);
  //button3State = digitalRead(button3Pin);
  //Hier Sleep einbauen
  //delay(50);

  //char* str = "";

  if (!dead)
  {
    /* -------- MODIFY PET STATS -------- */
    // TODO: different gradients regarding to age
    if (sleeping)
    {
      hunger -= 0.00005;
      poopometer += 0.00005;
      if (happiness - 0.0001 > 0)
      {
        happiness -= 0.0001;
      }
      health -= 0.00005 + countPoops() * 0.0001;
      if (discipline - 0.0001 > 0)
      {
        discipline -= 0.0001;
      }
    }
    else
    {
      hunger -= 0.00025;
      poopometer += 0.00025;
      if (happiness - 0.0002 > 0)
      {
        happiness -= 0.0002;
      }
      health -= 0.0001 + countPoops() * 0.0001;
      if (discipline - 0.0002 > 0)
      {
        discipline -= 0.0002;
      }
      //discipline-=0.02;
    }
    age += 0.0000025;

    //diarrhea :) for testing
    //poopometer+=0.005;

    //health-=1;
    //health-=countPoops()*0.0001;
    //health-=countPoops()*0.05;

    if (poopometer >= 10)
    {
      poopometer = countPoops();
      // ESP32 uses std's round function which returns a double
      poops[(int)round(poopometer)] = random(20, display.width() + 32);
      if (soundEnabled)
      {
        esp32tone(200, 50);
      }
      poopometer = 0;
    }

    if ((hunger > 19.99975 && hunger < 20.00025) || (happiness > 19.9998 && happiness < 20.0002) || (health > 19.9999 && health < 20.0001) && soundEnabled)
    {
      if (soundEnabled)
      {
        esp32tone(200, 50);
      }
    }

    if (hunger <= 20 || countPoops() > 0 || happiness <= 20 || health <= 20)
    {
      notification = true;
    }
    if (hunger > 20 && countPoops() == 0 && happiness > 20 && health > 20)
    {
      notification = false;
      digitalWrite(13, LOW);
    }

    if (hunger <= 0 || health <= 0 || happiness <= 0)
    {
      dead = true;
      if (soundEnabled)
      {
        esp32tone(500, 500);
#ifdef SLEEPDELAY
        DelayLightSleep(550);
#else
        delay(550);
#endif
        esp32tone(400, 500);
#ifdef SLEEPDELAY
        DelayLightSleep(550);
#else
        delay(550);
#endif
        esp32tone(300, 600);
      }
    }

    display.clearDisplay();
    display.setCursor(0, 0);

    /* ------- BUTTON PRESS ACTIONS ------- */

    /* ------- BUTTON 1 - MENU ------- */
    if (button1State == HIGH)
    {

      // JUMP IN GAME
      if (game)
      {

        if (!jumping && !paused)
        {
          if (soundEnabled)
          {
            esp32tone(200, 50);
          }
          jumping = true;
        }
      }
      else
      {
        // MENU

        if (soundEnabled)
        {
          esp32tone(300, 80);
        }

        if (!menuOpened)
        {
          menuOpened = true;
        }
        else
        {
          if (menuDepth)
          {

            if ((const char *)pgm_read_word(&(mainMenu[menu][subMenu + 1])) == NULL)
            {
              subMenu = 1;
            }
            else
            {
              ++subMenu;
            }
            setting = 100 * (menu + 1) + subMenu;
          }
          else
          {
            if (menu == MENUSIZE - 1)
            {
              menu = 0;
            }
            else
            {
              ++menu;
            }

            if ((const char *)pgm_read_word(&(mainMenu[menu][1])) != NULL)
            {
              subMenu = 1;

              justOpened = true;
            }
            setting = 100 * (menu + 1) + subMenu;
          }
        }

#ifdef SLEEPDELAY
        DelayLightSleep(60);
#else
        delay(60);
#endif
      }
    }
    /* ------- BUTTON 2 - SELECT ------- */
    if (button2State == HIGH)
    {

      if (game)
      {
        if (!gameOver)
        {
          paused = !paused;
          if (soundEnabled)
          {
            esp32tone(600, 80);
          }
#ifdef SLEEPDELAY
          DelayLightSleep(60);
#else
          delay(60);
#endif
        }
      }
      else
      {

        if (soundEnabled)
        {
          esp32tone(600, 80);
        }

        if (menuOpened)
        {

          if (subMenu != 1 && (const char *)pgm_read_word(&(mainMenu[menu][1][0])) != NULL)
          {
            action = 100 * (menu + 1) + subMenu;
          }
          if (subMenu == 1 && (const char *)pgm_read_word(&(mainMenu[menu][1][0])) == NULL)
          {
            action = 100 * (menu + 1) + subMenu;
          }
          if (subMenu == 1 && (const char *)pgm_read_word(&(mainMenu[menu][1][0])) != NULL && menuDepth)
          {
            action = 100 * (menu + 1) + subMenu;
          }
          if ((const char *)pgm_read_word(&(mainMenu[menu][1][0])) != NULL)
          {
            setting = 100 * (menu + 1) + subMenu;
            menuDepth = true;
          }
        }
        else
        {
          action = NULL;

          menuOpened = true;
          menuDepth = true;
          subMenu = 1;
          menu = 6;
          action = 701;
          setting = 701;
        }
        justOpened = false;

#ifdef SLEEPDELAY
        DelayLightSleep(60);
#else
        delay(60);
#endif
      }
    }
    /* ------- BUTTON 3 - BACK ------- */
    if (button3State == HIGH)
    {
      if (soundEnabled)
      {
        esp32tone(1000, 80);
      }

      if (game || gameOver)
      {
        walkPos = 0;
        walkXPos = 0;
        walkAnimReverse = false;
        walkRight = true;
        walkDirOffset = 0;
        treesXPos = -20;
        grassXPos = 0;
        obstacle1show = false;
        obstacle2show = false;
        jumping = false;
        jumpPos = 0;
        jumpUp = true;
        game = false;
        score = 0;
        newHiScore = false;
        gameOver = false;
        level = 0;
        paused = false;
      }
      else
      {
        if (!menuDepth)
        {
          menuOpened = false;
          menu = 0;
          setting = 0;
        }
        else
        {
          menuDepth = false;
          setting = 100 * (menu + 1) + 1;
        }
        action = NULL;
        subMenu = 1;
      }

#ifdef SLEEPDELAY
      DelayLightSleep(60);
#else
      delay(60);
#endif
    }

    /* ------- SCENERY AND WALKING ------- */

    //draw sun
    sunXPos += 0.1;
    if (sunXPos > display.width() + 2 * sunRadius)
    {
      sunXPos = -2 * sunRadius;
      sunOrMoon = !sunOrMoon;
    }
    if (sleeping)
    {
      sunOrMoon = true;
    }

    if (sleeping)
    {
      sunOrMoon = true;
    }

    if (!sunOrMoon)
    {
      display.fillCircle(sunXPos, 2 * sunRadius, sunRadius, WHITE);
    }
    else
    {
      display.fillCircle(sunXPos, 2 * sunRadius, sunRadius, WHITE);
      display.fillCircle(sunXPos - moonShadow, 2 * sunRadius, sunRadius, BLACK);
      //if(walkPos == 5){
      if ((int)round(cloud1XPos) % 5 == 0)
      {
        for (int i = 0; i < 6; i++)
        {
          stars[i][0] = random(0, display.width());
          stars[i][1] = random(0, 10);
        }
      }
      else
      {
        for (int i = 0; i < 6; i++)
        {

          display.drawPixel(stars[i][0], stars[i][1], WHITE);
        }
      }
    }

    //cloud 1
    cloud1XPos -= 0.3;
    if (cloud1XPos < -cloud1Width)
    {
      cloud1XPos = display.width() + cloud1Width;
    }
    display.drawBitmap(cloud1XPos, 5, cloud2, cloud1Width, 5, WHITE);

    //mountains
    display.drawBitmap(0, 7, mountains, 128, 16, WHITE);

    //walk and move ground perspective

    if (game)
    {

      /* ------ GAME -----*/
      level = (int)round(score / 10);

      if (jumping && !gameOver && !paused)
      {
        if (jumpUp)
        {
          jumpPos = jumpPos + 1 + level;
          if (jumpPos >= 12)
          {
            jumpUp = false;
          }
        }
        else
        {
          //jumpPos--;
          jumpPos = jumpPos - 1 - level;
          if (jumpPos <= 0)
          {
            jumpUp = true;
            jumping = false;
            if (soundEnabled)
            {
              esp32tone(100, 50);
            }
            score += 1;
          }
        }
      }

      if (!gameOver && !paused)
      {
        if (walkAnimReverse)
        {
          walkPos -= 1;
          if (walkPos == -1)
          {
            walkPos = 0;
            walkAnimReverse = false;
          }
        }
        else
        {
          walkPos += 1;
          if (walkPos == 3)
          {
            walkPos = 2;
            walkAnimReverse = true;
          }
        }

        walkXPos += 2;
        grassXPos += 4;
        treesXPos = treesXPos + 1 + level;
        obstacle1XPos = obstacle1XPos + 2 + level;
        obstacle2XPos = obstacle2XPos + 2 + level;

        if (!jumping &&
            ((obstacle1show && display.width() - obstacle1XPos >= 20 && display.width() - obstacle1XPos <= 46) ||
             (obstacle2show && display.width() - obstacle2XPos >= 20 && display.width() - obstacle2XPos <= 46)))
        {
          gameOver = true;
          jumping = true;
          jumpPos = -2;
          if (soundEnabled)
          {
            esp32tone(500, 40);
#ifdef SLEEPDELAY
            DelayLightSleep(50);
#else
            delay(50);
#endif
            esp32tone(350, 40);
#ifdef SLEEPDELAY
            DelayLightSleep(50);
#else
            delay(50);
#endif
            esp32tone(200, 60);
          }

          if (score > hiScore)
          {
            hiScore = score;
            newHiScore = true;
          }
          if (happiness + 15 < 100)
          {
            happiness += 15;
          }
          else
          {
            happiness = 100;
          }
          health -= 1;
          if (weight - score * 0.0025 > 5)
          {
            weight -= score * 0.0025;
          }
        }
      }

      if (walkXPos == display.width())
      {
        walkXPos = 0;
      }
      if (grassXPos == display.width())
      {
        grassXPos = 0;
      }
      if (treesXPos == display.width())
      {
        treesXPos = -128;
      }

      if (jumping)
      {
        display.drawBitmap(10, 26 - jumpPos, dinoJump, 48, 24, WHITE);
      }
      else
      {
        display.drawBitmap(10, 26, dinoWalk[walkPos], 48, 24, WHITE);
      }

      for (int i = 0; i < display.width() / 4 + 1; i++)
      {
        display.drawBitmap(-walkXPos + i * 8, 50, grass, 8, 6, WHITE);
      }

      // obstacles 1

      if (obstacle1XPos - 16 >= display.width())
      {
        obstacle1XPos = 0;
        obstacle1show = false;
      }
      if (!obstacle1show && random(1, 10) == 1 && obstacle2XPos > 40)
      {
        obstacle1show = true;
        obstacle1XPos = 0;
      }
      if (obstacle1show)
      {
        display.drawBitmap(display.width() - obstacle1XPos, 44, obstacle1, 16, 6, WHITE);
      }

      // obstacles 2
      if (obstacle2XPos - 16 >= display.width())
      {
        obstacle2XPos = 0;
        obstacle2show = false;
      }
      if (!obstacle2show && random(1, 10) == 1 && obstacle1XPos > 40)
      {
        obstacle2show = true;
        obstacle2XPos = 0;
      }

      if (obstacle2show)
      {
        display.drawBitmap(display.width() - obstacle2XPos, 44, obstacle2, 16, 6, WHITE);
      }

      //draw front grass
      for (int i = 0; i < display.width() / 16 + 1; i++)
      {
        display.drawBitmap(-grassXPos + i * 32, 60, grass_front, 32, 8, WHITE);
      }
      //draw trees
      display.drawBitmap(-treesXPos, 23, trees, 112, 20, WHITE);

      if (!gameOver)
      {
        display.setCursor(0, 56);
        display.setTextColor(WHITE);
        display.print(F("LvL: "));
        display.print(level);
        display.setCursor(64, 56);
        display.setTextColor(WHITE);
        display.print(F("Punkte: "));
        display.print(score);
      }

      if (paused && (int)round(cloud1XPos) % 2 == 0)
      {
        display.fillRect(24, 11, 80, 15, BLACK);
        display.fillRect(25, 12, 78, 13, WHITE);
        display.setCursor(47, 15);
        display.setTextColor(BLACK);
        display.println(F("Pause"));
      }

      /* ---------- END GAME ----------*/
    }
    else
    {

      /* ------ NO GAME -----*/
      if (!sleeping)
      {
        display.drawBitmap(walkXPos, 26, dinoWalk[walkPos + walkDirOffset], 48, 24, WHITE);
      }
      else
      {
        display.drawBitmap(walkXPos, 29, dinoWalk[walkPos + walkDirOffset], 48, 24, WHITE);
        if (walkRight)
        {
          if ((int)round(cloud1XPos) % 3 == 0)
          {
            display.setCursor(walkXPos + 48, 36);
            display.print(F("Z"));
          }
          else
          {
            display.setCursor(walkXPos + 46, 38);
            display.print(F("z"));
          }
        }
        else
        {
          if ((int)round(cloud1XPos) % 3 == 0)
          {
            display.setCursor(walkXPos - 4, 36);
            display.print(F("Z"));
          }
          else
          {
            display.setCursor(walkXPos - 2, 38);
            display.print(F("z"));
          }
        }
      }
      if (walkRight)
      {
        if (!sleeping)
        {
          walkXPos += 1;
          grassXPos += 2;
          treesXPos += 0.5;
        }
        if (walkXPos > 80)
        {
          walkRight = false;
          walkDirOffset = 3;
        }
      }
      else
      {
        if (!sleeping)
        {
          walkXPos -= 1;
          grassXPos -= 2;
          treesXPos -= 0.5;
        }
        if (walkXPos < 0)
        {
          walkRight = true;
          walkDirOffset = 0;
        }
      }

      //draw grass (ground)
      for (int i = 0; i < 2 * display.width() / 4; i++)
      {
        display.drawBitmap(-walkXPos + i * 8, 50, grass, 8, 6, WHITE);
      }
      // draw poops
      for (int i = 0; i < 3; i++)
      {
        if (poops[i] > 0)
        {
          display.drawBitmap(-walkXPos + poops[i], 44, poop, 16, 6, WHITE);
        }
      }
      //draw front grass
      for (int i = 0; i < 2 * display.width() / 16; i++)
      {
        display.drawBitmap(-grassXPos + i * 32, 56, grass_front, 32, 8, WHITE);
      }
      //draw trees
      display.drawBitmap(-treesXPos, 23, trees, 112, 20, WHITE);

      if (!sleeping)
      {
        if (walkAnimReverse)
        {
          --walkPos;
          if (walkPos == -1)
          {
            walkPos = 0;
            walkAnimReverse = false;
          }
        }
        else
        {
          ++walkPos;
          if (walkPos == 3)
          {
            walkPos = 2;
            walkAnimReverse = true;
          }
        }
      }
    }

    /* ------- MENUS AND ACTIONS ------- */
    //render menu
    if (menuOpened and !game)
    {
      display.fillRect(0, 0, display.width(), 30, BLACK);
      display.drawRect(0, 0, display.width(), 29, WHITE);
      display.fillRect(1, 1, display.width() - 2, 27, BLACK);
      display.drawRect(0, 0, display.width(), 12, WHITE);
      display.setCursor(8, 2);
      display.setTextSize(1);
      if (menuDepth)
      {
        display.fillRect(0, 0, display.width(), 12, WHITE);
        display.fillRect(1, 18, 1, 5, WHITE);
        display.fillRect(2, 19, 1, 3, WHITE);
        display.fillRect(3, 20, 1, 1, WHITE);
        display.setTextColor(BLACK, WHITE);
      }
      else
      {
        display.fillRect(1, 3, 1, 5, WHITE);
        display.fillRect(2, 4, 1, 3, WHITE);
        display.fillRect(3, 5, 1, 1, WHITE);
        display.setTextColor(WHITE);
      }
      char oneItem[STRING_SIZE];
      memcpy_P(&oneItem, &mainMenu[menu][0], sizeof oneItem);
      //display.println(getItem(menu,0));
      display.println(oneItem);
      if (subMenu)
      {
        display.setTextColor(WHITE);
        display.setCursor(8, 16);
        char subItem[STRING_SIZE];
        memcpy_P(&subItem, &mainMenu[menu][subMenu], sizeof subItem);
        //display.println(getItem(menu,subMenu));
        display.println(subItem);
      }
    }

    //do actions

    if (action > 0)
    {

      if ((action == 101 || action == 102 || action == 103) && !sleeping && random(1, (11 - round(discipline / 10))) == 1)
      {
        //95-100 discipline = 100% chance to feed
        //85-95 discipline = 50% chance
        //75-85 discipline = 33.33% chance
        //65-75 discipline = 25% chance
        //55-65 discipline = 20% chance
        //45-55 discipline = 16.67% chance
        //35-45 discipline = 14.28% chance
        //25-35 discipline = 12.5% chance
        //15-25 discipline = 12.5% chance
        //5-15 discipline = 10% chance
        //0-5 discipline = 9% chance

        //animate eating

        display.fillRect(0, 0, display.width(), display.height(), BLACK);
        for (int j = 0; j < 3; j++)
        {
          for (int i = 0; i < 4; i++)
          {
            display.clearDisplay();
            switch (action)
            {
            case 101:
              //apple
              display.drawBitmap(50, 40, apple, 24, 24, WHITE);
              if (j > 0)
                display.fillCircle(76, 54, 12, BLACK);
              if (j == 2)
                display.fillCircle(47, 55, 12, BLACK);
              break;
            case 102:
              //steak
              display.drawBitmap(50, 40, steak, 24, 24, WHITE);
              if (j > 0)
                display.fillCircle(76, 59, 13, BLACK);
              if (j == 2)
                display.fillCircle(60, 63, 13, BLACK);
              break;
            case 103:
              //water ripples
              display.drawCircle(80, 55, 1 + 1 * i, WHITE);
              display.drawCircle(80, 55, 5 + 2 * i, WHITE);
              display.drawCircle(80, 55, 10 + 4 * i, WHITE);
              break;
            }
            display.drawBitmap(80, 24, eating[i], 48, 40, WHITE);
#ifdef SLEEPDELAY
            DelayLightSleep(150);
#else
            delay(150);
#endif
            display.display();
          }
        }

        switch (action)
        {
        //apple
        case 101:
          if (hunger + 10 > 100)
          {
            hunger = 100;
            weight += 0.1;
          }
          else
          {
            hunger += 10;
          }
          if (health + 1 <= 100)
          {
            health += 1;
          }
          poopometer += 0.02;
          break;
        //steak
        case 102:
          if (hunger + 20 > 100)
          {
            hunger = 100;
            weight += 0.2;
          }
          else
          {
            hunger += 20;
            weight += 0.1;
          }
          if (health - 1 > 0)
          {
            health -= 1;
          }
          poopometer += 0.05;
          break;
        //water
        case 103:
          if (hunger + 5 <= 100)
          {
            hunger += 5;
          }
          poopometer += 0.01;
          break;
        }
      }
      else
      {
        if (action == 101 || action == 102 || action == 103)
        {
          if (soundEnabled)
          {
            esp32tone(500, 200);
#ifdef SLEEPDELAY
            DelayLightSleep(250);
#else
            delay(250);
#endif
          }
        }
      }

      switch (action)
      {
      case 201:
        //game
        if (!sleeping && health > 20)
        {
          game = true;
          walkPos = 0;
          walkXPos = 0;
          walkAnimReverse = false;
          walkRight = false;
          walkDirOffset = 2;
          treesXPos = -20;
          grassXPos = 0;
        }
        break;
      case 301:
        //sleep
        sleeping = !sleeping;
        if (!sleeping)
        {
          sunOrMoon = false;
        }
        else
        {
          for (int i = 0; i < 6; i++)
          {
            stars[i][0] = random(0, display.width());
            stars[i][1] = random(0, 10);
          }
        }

        break;
      case 401:
        //bath
        resetPoops();
        break;
      case 501:
        //doctor
        if (health < 60)
        {
          health = 100;
          for (int i = 0; i < 5; i++)
          {
            display.clearDisplay();
            if (i % 2 != 0)
            {
              display.fillRect(32, 23, 64, 16, WHITE);
              display.fillRect(56, 0, 16, 64, WHITE);
            }
            display.display();
#ifdef SLEEPDELAY
            DelayLightSleep(300);
#else
            delay(300);
#endif
          }
        }

        break;
      case 601:
        //discipline
        if (action == 601 && !sleeping)
        {
          if (discipline + 12 <= 100)
          {
            discipline += 12;
          }
          else
          {
            discipline = 100;
          }
          if (happiness - 3 > 0)
          {
            happiness -= 3;
          }
#ifdef SLEEPDELAY
          DelayLightSleep(150);
#else
          delay(150);
#endif
          for (int i = 0; i < 5; i++)
          {
            if (soundEnabled)
            {
              esp32tone(200 * i, 100);
            }
            display.setCursor(100 + 3 * i, 32);
            display.print(F("!"));
            display.display();
#ifdef SLEEPDELAY
            DelayLightSleep(150);
#else
            delay(150);
#endif
          }
        }
        break;

      case 801:
        soundEnabled = !soundEnabled;
        break;
      }
      action = 0;
    }

    //display settings
    if (setting > 0 and !game)
    {
      display.setCursor(8, 16);
      if (setting == 201)
      {
        display.println(F("macht Spass"));
      }
      if (setting == 301)
      {
        display.println(F("ruht aus"));
      }
      if (setting == 401)
      {
        display.println(F("raeumt auf"));
      }
      if (setting == 501)
      {
        display.println(F("bei Krankheit"));
      }
      if (setting == 601)
      {
        display.println(F("macht Smarter")); //gehorsman
      }
      if (setting == 701 || setting == 702 || setting == 703 || setting == 704)
      {
        display.drawRect(70, 17, 52, 7, WHITE);
      }
      if (setting == 701)
      {
        drawBar(hunger);
      }
      if (setting == 702)
      {
        drawBar(happiness);
      }
      if (setting == 703)
      {
        drawBar(health);
      }
      if (setting == 704)
      {
        drawBar(discipline);
      }
      if (setting == 705 || setting == 706 || setting == 801)
      {
        display.setCursor(80, 16);
      }
      if (setting == 705)
      {
        //display.setCursor(80,16);
        display.print(weight, 1);
        display.println(F(" t"));
      }
      if (setting == 706)
      {
        display.print(age, 1);
        display.println(F(" y."));
      }
      if (setting == 801)
      {
        if (soundEnabled)
        {
          display.println(F("An"));
        }
        else
        {
          display.println(F("Aus"));
        }
      }
    }

    //display notification
    if (notification)
    {
      ++notificationBlink;
      if (notificationBlink == 10)
      {
        notificationBlink = 0;
      }
      if (notificationBlink != 1)
      {
        display.drawRect(117, 28, 11, 11, WHITE);
        display.setTextColor(WHITE);
        digitalWrite(13, LOW);
      }
      else
      {
        display.fillRect(117, 28, 11, 11, WHITE);
        display.setTextColor(BLACK);
        digitalWrite(13, HIGH);
      }
      display.setCursor(120, 30);
      display.println(F("!"));
      if (dead)
      {
        digitalWrite(13, LOW);
      }
    }

    // GAME OVER
    if (gameOver)
    {

      display.fillRect(15, 11, 98, 43, BLACK);
      display.drawRect(16, 12, 96, 41, WHITE);
      display.fillRect(16, 12, 96, 13, WHITE);
      display.setCursor(36, 15);
      display.setTextColor(BLACK);
      display.println(F("GAME OVER"));
      display.setTextColor(WHITE);
      display.setCursor(21, 29);
      if (newHiScore)
      {
        display.println(F("NEW HI-SCORE!"));
        display.setCursor(21, 40);
      }
      else
      {
        display.println(F("SCORE:"));
        display.setCursor(21, 40);
      }
      display.println(score);
    }

    display.display();
    if (millis() - lastCloud > 10000)
    {
      lastCloud = millis();
      send_status();
    }
  }
  else
  {
    //dead...
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextColor(WHITE);
    display.println(F("Gestorben...\n\nTaste 1 zum\nneustarten"));
    display.display();

    if (button1State == HIGH)
    {
      if (soundEnabled)
      {
        esp32tone(300, 80);
#ifdef SLEEPDELAY
        DelayLightSleep(200);
#else
        delay(200);
#endif
      }
      esp32noTone();
      esp_task_wdt_init(1, true);
      esp_task_wdt_add(NULL);
      while (true)
        ;
    }
  }
}

void drawBar(float value)
{
  display.fillRect(72, 19, 48 * value / 100, 3, WHITE);
}

char *getItem(int menu, int index)
{
  char oneItem[STRING_SIZE];
  memcpy_P(&oneItem, &mainMenu[menu][index], sizeof oneItem);
  return oneItem;
}

int countPoops()
{
  int poopsCnt = 0;
  for (int i = 0; i < 3; i++)
  {
    if (poops[i] > 0)
    {
      ++poopsCnt;
    }
  }
  return poopsCnt;
}

void resetPoops()
{
  for (int i = 0; i < 3; i++)
  {
    poops[i] = 0;
  }
}

// tone methods for ESP32
void esp32tone(int frequency, unsigned long duration)
{
  ledcWriteTone(SOUND_CHAN, frequency);
  if (duration > 0)
  {
#ifdef SLEEPDELAY
    DelayLightSleep(duration);
#else
    delay(duration);
#endif
    ledcWrite(SOUND_CHAN, 0);
  }
}

void esp32tone(int frequency)
{
  ledcWriteTone(SOUND_CHAN, frequency);
}

void esp32noTone()
{
  ledcWrite(SOUND_CHAN, 0);
}

void DelayLightSleep(int milis)
{
  esp_sleep_enable_timer_wakeup(milis * uS_TO_mS_FACTOR);
  esp_light_sleep_start();
}

void send_status()
{
  String json = '{"dino":{ "id": "' + MQTT_CLIENTID + '", "name": "' + String(PET_NAME) + '", "hunger": ';
  json += String(int(hunger * fixPointFactor), DEC);
  json += ', "happiness" : ';
  json += String(int(happiness * fixPointFactor), DEC);
  json += ', "health" : ';
  json += String(int(health * fixPointFactor), DEC);
  json += ', "discipline" : ';
  json += String(int(discipline * fixPointFactor), DEC);
  json += ', "weight" : ';
  json += String(int(weight * fixPointFactor), DEC);
  json += ', "age" : ';
  json += String(int(age * fixPointFactor), DEC);
  json += ', "highscore" : ';
  json += String(hiScore, DEC);
  json += ', "poops" : ';
  json += String(int(poopometer * fixPointFactor), DEC);
  json += '}';
  json += ', "fixPointFactor" : ';
  json += String(fixPointFactor, DEC);
  json += '}';
  devPing->publish(json.c_str());
}