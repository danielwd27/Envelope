Envelope 💌

This is a project I made for my girlfriend for Christmas over about a month. It's built with an ESP32 CYD (Cheap-yellow-display) bought for just over $20. With a 3d printed case, this device sits on a desk or nightstand, displaying real-time messages sent over the internet. This is the most ambitious project I've created to date, I hope you find it interesting! 

Project Overview

Features

    Real-Time Messaging: Scrolls text messages sent from a specific Telegram account.

    Interactive Heart: Tapping the heart plays an animation and sends a push notification reply back to the sender's Telegram.

    Theme Control: Tap the Flower icon to cycle through color themes.

    Brightness Control: Tap the Light icon to toggle between brightness levels.

    Digital Clock: Displays live, internet-synced time (NTP).

    Easy WiFi Setup: Uses a captive portal. If WiFi is lost or changed, the device creates a Hotspot ("Envelope Setup") to configure credentials via phone.

    Factory Reset: Holding the screen during power-up wipes saved WiFi credentials.

Hardware Used

    Board: ESP32-32E (Commonly known as the "Cheap Yellow Display" or CYD). My specific version is from Hosyond on Amazon

    Display: 480x320 4.0" TFT Touchscreen (ST7796S Driver).

Software & Libraries

This project is built using PlatformIO and relies (heavily) on the following libraries:

    TFT_eSPI (Graphics & Sprite rendering)

    UniversalTelegramBot (Telegram API communication)

    WiFiManager (Captive portal for WiFi connection)

    ArduinoJson (JSON parsing for Telegram)

How to Use

    First Boot: The screen will turn Blue. Connect your phone to the WiFi network "Envelope Setup" and enter your home WiFi credentials.

    Sending Messages: Send a text to the Telegram Bot created for this device. It will appear on the Envelope.

    Sending a reply: Tap the large heart in the center. It will flash white, and the sender will receive a notification.

    Changing Settings: Use the icons in the top right to change the Color Theme or Screen Brightness.

Creating Your Own

If you want to build your own, it's fairly easy to do so, you will need to change/tweak the following:

    Screen Calibration: Each CYD touchscreen has variance in its perceived dimensions. This must be accounted for in software. In the TFT_eSPI library, there exists a program which will help you to tweak these bounds.

    API Keys: Follow the format in keys_example.h and rename as keys.h. Information for setting up a Telegram bot can be found online.

    If using a different display: Certain drivers or screen dimensions may need tweaking. This is configured for a specific 480x320 display, yours might be different which could change pin numbering.

Credits & Attribution

    Sprites: The pixel art used in this project were not created by me.

    Bitmap Conversion: All graphics (Heart, Flower, Light, Clock) were converted from PNG to C arrays using the image2cpp tool by javl.

    3d Printed Case: https://www.thingiverse.com/thing:7166648
