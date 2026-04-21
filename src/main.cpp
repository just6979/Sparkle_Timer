#include <Adafruit_NeoPixel.h>

constexpr uint MINUTES_STRIP_PIN = 32;
constexpr uint SECONDS_STRIP_PIN = 33;

constexpr uint MINUTES_STRIP_COUNT = 8;
constexpr uint SECONDS_STRIP_COUNT = 60;

constexpr uint MINUTES_STRIP_BRIGHT = 5;
constexpr uint SECONDS_STRIP_BRIGHT = 5;

Adafruit_NeoPixel minStrip(
  MINUTES_STRIP_COUNT, MINUTES_STRIP_PIN, NEO_GRB + NEO_KHZ800
);
Adafruit_NeoPixel secStrip(
  SECONDS_STRIP_COUNT, SECONDS_STRIP_PIN, NEO_GRB + NEO_KHZ800
);

ulong now;
uint waitTime = 1000;
uint lastUpdate = 0;
uint switchTime = 25;

bool updateMinutes = false;
bool minForward = false;
bool secForward = true;
int secPos;
int prevSecPos;
int minPos;
int prevMinPos;

bool fill = false;

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

  if (secForward) {
    secPos = -1;
    prevSecPos = secStrip.numPixels() - 1;
  } else {
    secPos = secStrip.numPixels() - 1;
    prevSecPos = 0;
  }
  if (minForward) {
    minPos = 0;
    prevMinPos = minStrip.numPixels() - 1;
  } else {
    minPos = minStrip.numPixels() - 1;
    prevMinPos = 0;
  }
}

void loop() {
  now = millis();

  if (now - lastUpdate > waitTime) {
    lastUpdate = now;
    prevSecPos = secPos;
    if (secForward) {
      secPos += 1;
      if (secPos >= secStrip.numPixels()) {
        secPos = 0;
        secStrip.clear();
        updateMinutes = true;
      }
    } else {
      secPos -= 1;
      if (secPos < 0) {
        secPos = secStrip.numPixels() - 1;
        secStrip.clear();
        updateMinutes = true;
      }
    }

    if (updateMinutes) {
      updateMinutes = false;
      prevMinPos = minPos;
      if (minForward) {
        minPos += 1;
        if (minPos >= minStrip.numPixels()) {
          minPos = 0;
          minStrip.clear();
        }
      } else {
        minPos -= 1;
        if (minPos < 0) {
          minPos = minStrip.numPixels() - 1;
          minStrip.clear();
        }
      }
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
