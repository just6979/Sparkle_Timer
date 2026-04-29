#include <Adafruit_NeoPixel.h>

// sets pixel count and direction for some strips/matrices used in testing
// #define FULL_MODE
// #define QUAD_MODE
#define SMALL_MODE

// makes seconds strip complete in a single second, for testing
#define FAST_MODE

#ifdef FULL_MODE
constexpr uint MINUTES_STRIP_COUNT = 60;
constexpr uint SECONDS_STRIP_COUNT = 60;
#define MINUTES_FORWARD
#define SECONDS_FORWARD
#else
#ifdef QUAD_MODE
constexpr uint MINUTES_STRIP_COUNT = 60;
constexpr uint SECONDS_STRIP_COUNT = 60;
#define MINUTES_FORWARD
#define SECONDS_FORWARD
#else
#ifdef SMALL_MODE
constexpr uint MINUTES_STRIP_COUNT = 8;
constexpr uint SECONDS_STRIP_COUNT = 8;
#undef MINUTES_FORWARD
#undef SECONDS_FORWARD
#endif
#endif
#endif

#ifdef FAST_MODE
constexpr uint WAIT_TIME = 1000 / 60 * 60 / SECONDS_STRIP_COUNT;
#else
constexpr uint WAIT_TIME = 1000 * 60 / SECONDS_STRIP_COUNT;
#endif

constexpr uint MINUTES_STRIP_PIN = 32;
constexpr uint SECONDS_STRIP_PIN = 33;

constexpr uint MINUTES_STRIP_BRIGHT = 5;
constexpr uint SECONDS_STRIP_BRIGHT = 5;

Adafruit_NeoPixel minStrip(
  MINUTES_STRIP_COUNT, MINUTES_STRIP_PIN, NEO_GRB + NEO_KHZ800
);
Adafruit_NeoPixel secStrip(
  SECONDS_STRIP_COUNT, SECONDS_STRIP_PIN, NEO_GRB + NEO_KHZ800
);

ulong now;
uint lastUpdate = 0;
uint switchTime = 25;

bool updateMinutes = false;
int secPos;
int prevSecPos;
int minPos;
int prevMinPos;

bool fill = true;

uint hueBase = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println("Setting up");

  minStrip.begin();
  minStrip.setBrightness(MINUTES_STRIP_BRIGHT);
  minStrip.fill();
  minStrip.show();

  secStrip.begin();
  secStrip.setBrightness(SECONDS_STRIP_BRIGHT);
  secStrip.fill();
  secStrip.show();

  #ifdef SECONDS_FORWARD
  secPos = -1;
  prevSecPos = secStrip.numPixels() - 1;
  #else
  secPos = secStrip.numPixels() - 1;
  prevSecPos = 0;
  #endif
  #ifdef MINUTES_FORWARD
  minPos = 0;
  prevMinPos = minStrip.numPixels() - 1;
  #else
  minPos = minStrip.numPixels() - 1;
  prevMinPos = 0;
  #endif
}

void loop() {
  now = millis();

  if (now - lastUpdate > WAIT_TIME) {
    lastUpdate = now;
    prevSecPos = secPos;

    #ifdef SECONDS_FORWARD
    secPos += 1;
    if (secPos >= secStrip.numPixels()) {
      secPos = 0;
      secStrip.clear();
      updateMinutes = true;
    }
    #else
    secPos -= 1;
    if (secPos < 0) {
      secPos = secStrip.numPixels() - 1;
      secStrip.clear();
      updateMinutes = true;
    }
    #endif

    if (updateMinutes) {
      updateMinutes = false;
      prevMinPos = minPos;

      #ifdef MINUTES_FORWARD
      minPos += 1;
      if (minPos >= minStrip.numPixels()) {
        minPos = 0;
        minStrip.clear();
      }
      #else
      minPos -= 1;
      if (minPos < 0) {
        minPos = minStrip.numPixels() - 1;
        minStrip.clear();
      }
      #endif
    }

    hueBase += 256;
    const uint secHue = hueBase + secPos * 65536 / minStrip.numPixels();
    const uint color = Adafruit_NeoPixel::gamma32(
      Adafruit_NeoPixel::ColorHSV(secHue)
    );
    secStrip.setPixelColor(secPos, color);
    secStrip.show();
    minStrip.setPixelColor(minPos, color);
    minStrip.show();

    if (!fill) {
      delay(switchTime);
      secStrip.setPixelColor(prevSecPos, 0);
      secStrip.show();
      minStrip.setPixelColor(prevMinPos, 0);
      minStrip.show();
    }
  }
}
