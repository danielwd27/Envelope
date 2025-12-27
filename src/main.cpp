#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h> 
#include <ArduinoJson.h>
#include <TFT_eSPI.h> 
#include <WiFiManager.h>
#include <time.h> 
#include "keys.h" 
#include "sprites.h"
#include "cat.h"

// Time configuration
#define TIME_ZONE_OFFSET -21600  // UTC -6 hours (CST)
#define DST_OFFSET       3600   

// Brightness levels (0-255)
int brightLevels[] = {255, 160, 100, 10}; 
int totalBrightLevels = 4;
int brightIndex = 0;

// Theme configuration
struct Theme {
  uint16_t bg;       // Background Color
  uint16_t text;     // Text Color
  uint16_t accent;   // Line / Icon Color
  uint16_t heart;    // Heart Color
};

Theme themes[] = {
  {0x10A4, 0xF79F, 0x6AF4, 0x98C7},                // 0. Dark 
  {0xF79F, 0x10A4, 0x1B34, 0xD904},                // 1. Light
  {0xF70D, 0xED85, 0x9204, 0xE362},                // 2. Light 2 (sunset)
  {TFT_NAVY, TFT_WHITE, TFT_CYAN, TFT_MAGENTA},    // 3. Ocean
  {0xEC2E,0xF614, 0x98C7, 0xD904}                  // 4. ZEEP ZORP
};
int currentTheme = 0;
const int totalThemes = 5;
bool themeChanged = true; 

// Shared variables
volatile bool newMessageAvailable = false;
volatile bool requestReply = false; 
String incomingText = ""; 
String currentMessage = defaultMessage; // default message on startup 

// Animation variables
int textX = 0; // X position of scrolling text
float catX = 0;          // Horizontal position
float catSpeed = 1;    // How fast it walks
int catFrame = 0;        // Current animation frame (0-12)
int catDirection = 1;    // 1 = Right, -1 = Left

// Objects
TFT_eSPI tft = TFT_eSPI(); 
TFT_eSprite textSprite = TFT_eSprite(&tft); 
TFT_eSprite heartSprite = TFT_eSprite(&tft); 
TFT_eSprite clockSprite = TFT_eSprite(&tft);
TFT_eSprite flowerSprite = TFT_eSprite(&tft); 
TFT_eSprite lightSprite = TFT_eSprite(&tft);
TFT_eSprite catSprite = TFT_eSprite(&tft);

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

/*
  Network Task implemented using Telegram Bot to check for new messages. This is run on core 0 to reduce stuttering in the main loop.
*/
void networkTask(void * parameter) {
  while(true) {
    if(WiFi.status() == WL_CONNECTED) {
      if (requestReply) {
        bot.sendMessage(CHAT_ID, "Ellie is thinking of you! <3", "");
        requestReply = false; 
      }
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      if (numNewMessages) { 
        for (int i = 0; i < numNewMessages; i++) {
          if (String(bot.messages[i].chat_id) == CHAT_ID) {
            incomingText = bot.messages[i].text;
            newMessageAvailable = true;
            bot.sendMessage(CHAT_ID, "Message displayed!", "");
          }
        }
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS); 
  }
}

// Used to reset wifi configuration and display instructions
void configModeCallback (WiFiManager *myWiFiManager) {
  tft.fillScreen(TFT_BLUE); 
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(2);
  tft.setCursor(0, 10);
  tft.println(" Connection Failed!" );
  tft.println("\n Please connect to WiFi:");
  tft.println("\n 1. On your phone, go to Wifi Settings");
  tft.println("\n 2. Connect to: Envelope Setup");
  tft.println("\n 3. Configure credentials in the portal"); 
  tft.println("\n 4. Wait a moment for connection..."); 
  tft.println("\n <3"); 
}

void setup() {
  Serial.begin(115200);
  pinMode(4, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(17, OUTPUT);
  digitalWrite(4, HIGH); 
  digitalWrite(16, HIGH); 
  digitalWrite(17, HIGH); 

  // 1. Initialize Screen
  tft.init();
  tft.setRotation(1); 
  tft.fillScreen(TFT_BLACK);
  
  // 2. Backlight Setup (PWM)
  pinMode(TFT_BL, OUTPUT);

  // Calibration:
  uint16_t calData[5] = { 286, 3548, 256, 3489, 7 }; 
  tft.setTouch(calData); 

  // Long press to reset device configuration
  // Triggers if touch is held DURING setup call (before device is powered on fully)
  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    tft.fillScreen(TFT_RED);
    tft.setCursor(10, 50);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.println("Resetting WiFi...");
    delay(2000);
    
    WiFiManager wm;
    wm.resetSettings(); // Wipes the saved credentials
    
    tft.println("Done! Restarting...");
    delay(1000);
    ESP.restart();
  }

  // 3. WiFi & Time Setup
  WiFiManager wm;
  wm.setAPCallback(configModeCallback);
  wm.setConnectTimeout(30); 
  if (!wm.autoConnect("Envelope Setup")) ESP.restart();

  client.setInsecure(); 
  configTime(TIME_ZONE_OFFSET, DST_OFFSET, "pool.ntp.org", "time.nist.gov");

  tft.fillScreen(TFT_BLACK);

  // 4. SETUP SPRITES
  
  // A. Text Sprite
  textSprite.createSprite(480, 60);
  textSprite.setTextSize(3);

  // B. Heart Sprite
  heartSprite.setColorDepth(1); 
  heartSprite.createSprite(90, 96); 

  // C. Clock Sprite
  clockSprite.setColorDepth(1);
  clockSprite.createSprite(140, 32); 

  // D. Flower Sprite (Theme Button)
  flowerSprite.setColorDepth(1);
  flowerSprite.createSprite(32, 32); 

  // E. Light Sprite (Brightness Button)
  lightSprite.setColorDepth(1);
  lightSprite.createSprite(32, 32);

  // F. Cat Sprite (Walking Animation)
  catSprite.setColorDepth(1);
  catSprite.createSprite(49, 32); // extra width to remove trailing / artifacts (4px of padding on either side)

  // Launch Network Task
  xTaskCreatePinnedToCore(networkTask, "NetworkTask", 20000, NULL, 1, NULL, 0);
}

void loop() {
  // Handle theme update
  if (themeChanged) {
    tft.fillScreen(themes[currentTheme].bg);
    themeChanged = false;
  }

  // Calculate heart bounce animation
  int bounceOffset = sin(millis() * 0.003) * 10; 
  int currentHeartY = 80 + bounceOffset;

  // Handle incoming messages
  if (newMessageAvailable) {
    currentMessage = incomingText + "      "; 
    textX = 480; 
    newMessageAvailable = false; 
  }

  // Handle touch input
  uint16_t t_x, t_y;
  static unsigned long lastTouchTime = 0; 
  
  if (tft.getTouch(&t_x, &t_y)) {
    
    // A. FLOWER PRESS (Top Right Theme Button)
    if (t_x > 420 && t_y < 70) {
       if (millis() - lastTouchTime > 300) { 
          Serial.println("Theme Button Pressed");
          currentTheme++;
          if (currentTheme >= totalThemes) currentTheme = 0;
          themeChanged = true; 
          lastTouchTime = millis();
       }
    }

    // B. LIGHT PRESS (Brightness Button)
    if (t_x > 360 && t_x <= 420 && t_y < 70) {
       if (millis() - lastTouchTime > 300) { 
          Serial.println("Light Button Pressed"); 
          
          brightIndex++;
          if (brightIndex > totalBrightLevels-1) brightIndex = 0; // Loop back to start
          
          // Apply new brightness
          Serial.print("Setting Brightness to: ");
          Serial.println(brightLevels[brightIndex]);
          analogWrite(TFT_BL, brightLevels[brightIndex]);
          
          lastTouchTime = millis();
       }
    }

    // C. HEART PRESS (Center)
    if (t_x > 150 && t_x < 330 && t_y > 80 && t_y < 220) {
      if (millis() - lastTouchTime > 2000) {
        Serial.println("Heart Pressed");
        
        // Flash White
        heartSprite.setBitmapColor(TFT_WHITE, themes[currentTheme].bg);
        heartSprite.fillSprite(0);
        heartSprite.drawBitmap(0, 0, epd_bitmap_cards_hearts, 90, 96, 1, 0);
        heartSprite.pushSprite(195, currentHeartY); 
        
        if (!requestReply) requestReply = true;
        lastTouchTime = millis();
        delay(200); 
      }
    }
  }

  // Draw Sprites

  // A. FLOWER (Theme Button)
  flowerSprite.setBitmapColor(themes[currentTheme].accent, themes[currentTheme].bg);
  flowerSprite.fillSprite(0); 
  flowerSprite.drawBitmap(0, 0, epd_bitmap_flower, 32, 32, 1, 0);
  flowerSprite.pushSprite(430, 10); 

  // B. LIGHT (Brightness Button)
  lightSprite.setBitmapColor(themes[currentTheme].accent, themes[currentTheme].bg);
  lightSprite.fillSprite(0);
  lightSprite.drawBitmap(0, 0, epd_bitmap_light, 32, 32, 1, 0);
  lightSprite.pushSprite(380, 10);

  // C. HEART
  heartSprite.setBitmapColor(themes[currentTheme].heart, themes[currentTheme].bg); 
  heartSprite.fillSprite(0); 
  heartSprite.drawBitmap(0, 0, epd_bitmap_cards_hearts, 90, 96, 1, 0);
  heartSprite.pushSprite(195, currentHeartY);

  // D. THE CAT (Walking Animation)
  // clear previous frame

  static int loopCount = 0;
  loopCount++;
  if (loopCount > 4) { // Adjust animation speed here
    catFrame++; 
    if (catFrame >= 13) catFrame = 0;
    loopCount = 0;
  }
  
  // Move Cat
  catX += (catSpeed * catDirection);
  if (catX > 448) catDirection = -1; // Walk Left (screen width (480) - sprite width (32) = 448)
  else if (catX < 0) catDirection = 1; // Walk Right

  // Draw Cat
  catSprite.setBitmapColor(themes[currentTheme].text, themes[currentTheme].bg);
  catSprite.fillSprite(0);
  
  if (catDirection == 1) { // right
      catSprite.drawBitmap(4, 0, epd_bitmap_cat_right[catFrame], 32, 32, 0, 1);
  } else { // left
      catSprite.drawBitmap(4, 0, epd_bitmap_cat_left[catFrame], 32, 32, 0, 1);
  }

  catSprite.pushSprite((int)catX-4, 218);

  // E. TEXT & LINE
  textSprite.fillSprite(themes[currentTheme].bg); 
  textSprite.setTextColor(themes[currentTheme].text, themes[currentTheme].bg); 
  textSprite.drawString(currentMessage, textX, 20);
  textSprite.fillRect(0, 0, 480, 5, themes[currentTheme].accent); 
  textSprite.pushSprite(0, 250);

  // F. CLOCK
  struct tm timeinfo;
  clockSprite.setBitmapColor(themes[currentTheme].accent, themes[currentTheme].bg);
  clockSprite.fillSprite(0);

  if (getLocalTime(&timeinfo)) {
     clockSprite.drawBitmap(0, 0, epd_bitmap_clock, 30, 32, 1, 0);
     int hour12 = timeinfo.tm_hour;
     String ampm = "AM";
     if (hour12 >= 12) { ampm = "PM"; if (hour12 > 12) hour12 -= 12; }
     if (hour12 == 0) hour12 = 12; 

     clockSprite.setTextColor(1, 0); 
     clockSprite.setTextSize(2);
     clockSprite.setCursor(42, 8); 
     clockSprite.printf("%d:%02d %s", hour12, timeinfo.tm_min, ampm);
  } else {
     clockSprite.drawBitmap(0, 0, epd_bitmap_clock, 30, 32, 1, 0);
  }
  clockSprite.pushSprite(10, 10);

  // F. SCROLL
  textX -= 2; 
  if (textX < -(int)(currentMessage.length() * 18)) { textX = 480; }

  delay(20); 
}