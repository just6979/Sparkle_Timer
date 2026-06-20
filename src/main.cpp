#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <ArduinoLog.h>
#include <secrets.h>
#include <time.h>
#include <WiFi.h>
#include <WiFiNTP.h>

// time to count down to (24 hour time); default: 8:01 AM
constexpr int alarm_hour = 8;
constexpr int alarm_minutes = 1;
// start counting X minutes before the alarm
constexpr int countdown_minutes = 30;
// how long to run the alert for before going back to sleep
constexpr int alert_minutes = 5;
// how long to wake up before the countdown to sync the time
constexpr int warmup_minutes = 15;

// sets pixel count and direction for some strands/matrices used in testing
#define SMALL_MODE
// #define QUAD_MODE

// for testing, 60x speed up, cycle the entire seconds strand in 1 second
#define FAST_MODE

// for testing, don't get the real time,
// just set it so as to start the countdown immediately
#define START_NOW

#ifdef SMALL_MODE
// for the small 8 LED strands used for testing
constexpr uint MINUTES_STRAND_COUNT = 8;
constexpr uint SECONDS_STRAND_COUNT = 8;
#undef MINUTES_FORWARD
#undef SECONDS_FORWARD
#else

#ifdef QUAD_MODE
// for the 8x8 LED "strands" used for testing, but only use 60 of the 64 LEDS
constexpr uint MINUTES_STRAND_COUNT = 60;
constexpr uint SECONDS_STRAND_COUNT = 60;
#define MINUTES_FORWARD
#define SECONDS_FORWARD
#else

// for the full 60 LED strands for the final product
constexpr uint MINUTES_STRAND_COUNT = 60;
constexpr uint SECONDS_STRAND_COUNT = 60;
#define MINUTES_FORWARD
#define SECONDS_FORWARD

#endif
#endif

// pins the strands are connected to
constexpr uint MINUTES_STRAND_PIN = 14;
constexpr uint SECONDS_STRAND_PIN = 15;
// per strand overall brightness
constexpr uint MINUTES_STRAND_BRIGHT = 5;
constexpr uint SECONDS_STRAND_BRIGHT = 5;
// the strands themselves
Adafruit_NeoPixel minutesStrand(MINUTES_STRAND_COUNT, MINUTES_STRAND_PIN);
Adafruit_NeoPixel secondsStrand(SECONDS_STRAND_COUNT, SECONDS_STRAND_PIN);

// datetime settings
constexpr int TIMEZONE_OFFSET = -18000;
constexpr bool IS_DST = true;
constexpr int DST_OFFSET = IS_DST ? 3600 : 0;
constexpr auto ISO_DATETIME_FMT = "%FT%T";
constexpr auto NTP_SERVER = "pool.ntp.org";
tm timeInfo;
char timeString[20];

// main loop timing
ulong now;
uint lastUpdate = 0;
#ifndef FAST_MODE
// take 60 seconds to cycle the seconds strand, no matter the size
constexpr uint WAIT_TIME = 1000 * 60 / SECONDS_STRAND_COUNT;
#else
// take 1 second to cycle the seconds strand, no matter the size, for testing
constexpr uint WAIT_TIME = 1000 / 60 * 60 / SECONDS_STRAND_COUNT;
#endif

// countdown settings
constexpr int COUNTDOWN_MINUTES = 30;
constexpr int COUNTDOWN_SECONDS = 0;

// modes defining how the strands act
enum MODES {
  DRAIN,      // fill the strands then remove lit pixels from the top
  // DRAIN_DROP, // drop a pixel from the bottom of the filled section
  // FILL,       // start empty and light pixels up
  // FILL_DROP,  // drop a pixel from the top down to fill the strand
  // FILL_SHOOT, // fire a pixel from the bottom up to fill the strand
  DROP,       // drop a single pixel top to bottom
  // SHOOT,      // fire a single pixel up each strand
  ALERT,         // flash strands to indicate time is up
SLEEP            // sleep until X minutes before the next wake time
} mode = DRAIN;

// drain mode settings
int secondsRemaining = COUNTDOWN_SECONDS;
int minutesRemaining = COUNTDOWN_MINUTES;

// drop mode settings
bool shouldUpdateMinutes = false;
int secondsPosition;
int prevSecondsPosition;
int minutesPosition;
int prevMinutesPosition;
uint switchTime = 25;
uint hueBase = 0;

// alert mode settings
bool alertActive = false;
int alertCount = 10;

void init_drain() {
  secondsStrand.clear();
  minutesStrand.clear();
  secondsStrand.fill(0xFFFFFF, 0, secondsRemaining);
  minutesStrand.fill(0xFFFFFF, 0, minutesRemaining);
  secondsStrand.show();
  minutesStrand.show();
}

void init_drop() {
  #ifdef SECONDS_FORWARD
  secondsPosition = -1;
  prevSecondsPosition = secondsStrand.numPixels() - 1;
  #else
  secondsPosition = secondsStrand.numPixels() - 1;
  prevSecondsPosition = 0;
  #endif
  #ifdef MINUTES_FORWARD
  minutesPosition = 0;
  prevMinutesPosition = minutesStrand.numPixels() - 1;
  #else
  minutesPosition = minutesStrand.numPixels() - 1;
  prevMinutesPosition = 0;
  #endif
}

void setup() {
  Serial.begin(115200);
  // wait for serial for monitoring setup
  // ulong serial_time = millis();
  // while (!Serial) {
  //   // but don't wait forever in case a debug build was left on the device
  //   if (serial_time > 4000) break;
  //   serial_time = millis();
  // }
  // delay(100);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.noticeln("Initializing system");

  // Log.notice("Initializing WiFi");
  // WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // while (WiFi.status() != WL_CONNECTED) {
  //   Serial.print(".");
  //   delay(100);
  // }
  // Serial.println("");
  // Log.noticeln("WiFi connected");
  //
  // Log.noticeln("Getting current time");
  // NTP.begin(NTP_SERVER);
  //
  // Log.noticeln("Waiting for NTP time sync: ");
  // if (NTP.waitSet([]() { Serial.print("."); })) {
  //   time_t now = time(nullptr);
  //   struct tm timeinfo;
  //   gmtime_r(&now, &timeinfo);
  //   Log.notice("Current time: ");
  //   Log.notice(asctime(&timeinfo));
  //   now = time(nullptr);

    // int cur_hour;
    // int cur_minute;
    // int cur_seconds;
    // cur_hour = timeInfo.tm_hour;
    // cur_minute = timeInfo.tm_min;
    // cur_seconds = timeInfo.tm_sec;

  //   strftime(timeString, 20, ISO_DATETIME_FMT, &timeInfo);
  //   Log.notice("Current time: %s\n", timeString);
  // } else {
  //   Log.noticeln("Failed to obtain time");
  // }

  Log.noticeln("Initializing strands");
  minutesStrand.begin();
  minutesStrand.setBrightness(MINUTES_STRAND_BRIGHT);
  minutesStrand.fill();
  minutesStrand.show();

  secondsStrand.begin();
  secondsStrand.setBrightness(SECONDS_STRAND_BRIGHT);
  secondsStrand.fill();
  secondsStrand.show();

  if (mode == DRAIN) {
    init_drain();
  }

  if (mode == DROP) {
    init_drop();
  }

  Log.noticeln("Running");
}

void show_alert() {
  if (alertCount <= 0) {
    secondsStrand.fill(0xFF0000);
    minutesStrand.fill(0xFF0000);
    secondsStrand.show();
    minutesStrand.show();
    delay(100);
  } else {
    alertCount--;
  }

  if (alertActive) {
    secondsStrand.fill(0xFF0000);
    minutesStrand.fill(0xFF0000);
  } else {
    secondsStrand.clear();
    minutesStrand.clear();
  }
  secondsStrand.show();
  minutesStrand.show();
  alertActive = !alertActive;
}

void loop_drain() {
  secondsRemaining--;
  secondsStrand.clear();
  minutesStrand.clear();
  secondsStrand.fill(0xFFFFFF, 0, secondsRemaining);
  minutesStrand.fill(0xFFFFFF, 0, minutesRemaining);
  secondsStrand.show();
  minutesStrand.show();
  if (secondsRemaining <= 0) {
    secondsRemaining = SECONDS_STRAND_COUNT;
    minutesRemaining--;
  }
  if (minutesRemaining <= 0) {
    mode = ALERT;
  }
}

void loop_drop() {
  prevSecondsPosition = secondsPosition;

  #ifdef SECONDS_FORWARD
  secondsPosition += 1;
  if (secondsPosition >= secondsStrand.numPixels()) {
    secondsPosition = 0;
    secondsStrand.clear();
    shouldUpdateMinutes = true;
  }
  #else
  secondsPosition -= 1;
  if (secondsPosition < 0) {
    secondsPosition = secondsStrand.numPixels() - 1;
    secondsStrand.clear();
    shouldUpdateMinutes = true;
  }
  #endif

  if (shouldUpdateMinutes) {
    shouldUpdateMinutes = false;
    prevMinutesPosition = minutesPosition;

    #ifdef MINUTES_FORWARD
    minutesPosition += 1;
    if (minutesPosition >= minutesStrand.numPixels()) {
      minutesPosition = 0;
      minutesStrand.clear();
    }
    #else
    minutesPosition -= 1;
    if (minutesPosition < 0) {
      mode = ALERT;
    }
    #endif
  }

  hueBase += 256;
  const uint secHue = hueBase + secondsPosition * 65536 / minutesStrand.
                      numPixels();
  const uint color = Adafruit_NeoPixel::gamma32(
    Adafruit_NeoPixel::ColorHSV(secHue)
  );
  secondsStrand.setPixelColor(secondsPosition, color);
  secondsStrand.show();
  minutesStrand.setPixelColor(minutesPosition, color);
  minutesStrand.show();

  delay(switchTime);
  secondsStrand.setPixelColor(prevSecondsPosition, 0);
  secondsStrand.show();
  minutesStrand.setPixelColor(prevMinutesPosition, 0);
  minutesStrand.show();
}

void loop() {
  now = millis();

  if (now - lastUpdate > WAIT_TIME) {
    lastUpdate = now;

    if (mode == ALERT) {
      show_alert();
    }

    if (mode == DRAIN) {
      loop_drain();
    }

    if (mode == DROP) {
      loop_drop();
    }

    if (mode == SLEEP) {
    }
  }
}
