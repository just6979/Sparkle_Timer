# What

A little project to make a morning routine countdown timer for my kids,
using some LED strips. One strip ticks down seconds while the other ticks
down the minutes remaining until we should be outside to catch the school bus.
Once the time arrives, the LEDs start counting back up, and after a couple
minutes they start flashing to indicate imminent arrival.

# How

Adafruit Sparkle Motion Mini LED driver board
* ESP32 Mini
  * 3 NeoPixel/WS281X driver outputs
    * Up to 4A current from a 5V USB-C power connection
  * Wifi & Bluetooth
    * Wifi used to syncronize time via NTP
    * Future:
      * Web UI (or Home Assistant integration) to:
        * adjust the daily alarms 
        * adjust the colors and patterns
        * set new timers
  * Microphone (I2S)
    * Future:
      * Could be used to stop the alert flashing by voice
      * Start a new timer by voice
  * Handful of GPIOs and Stemma QT for expansion
    * Unused for this project
 
* Adafruit Neon-like NeoPixel strips, x2
  * 60 pixels per meter
  * 1 meter each

* USB-C power supply

Programmed in C++, using the Arduino framework, via PlatformIO, in CLion

*(Would have loved to use CircuitPython,
but the solderless connections and relatively high LED driving current
led me to the Sparkle Motion boards, which use the older ESP32 SoCs,
which don't support the USB drive interface, making CircuitPython development a pain,
thus doing it C++, which is fine and great practice anyway.)*
