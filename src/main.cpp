#include <Adafruit_NeoPixel.h>

// sets pixel count and direction for some strands/matrices used in testing
#define SMALL_MODE
// #define QUAD_MODE

// for testing, 60x speed up, cycle the entire seconds strand in 1 second
// #define FAST_MODE

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

#ifndef FAST_MODE
// take 60 seconds to cycle the seconds strand, no matter the size
constexpr uint WAIT_TIME = 1000 * 60 / SECONDS_STRAND_COUNT;
#else
// take 1 second to cycle the seconds strand, no matter the size, for testing
constexpr uint WAIT_TIME = 1000 / 60 * 60 / SECONDS_STRAND_COUNT;
#endif

// pins the strands are connected to
constexpr uint MINUTES_STRAND_PIN = 32;
constexpr uint SECONDS_STRAND_PIN = 33;
// per strand overall brightness
constexpr uint MINUTES_STRAND_BRIGHT = 5;
constexpr uint SECONDS_STRAND_BRIGHT = 5;
// the strands themselves
Adafruit_NeoPixel minutesStrand(MINUTES_STRAND_COUNT, MINUTES_STRAND_PIN);
Adafruit_NeoPixel secondsStrand(SECONDS_STRAND_COUNT, SECONDS_STRAND_PIN);

// modes defining how the strands act
enum MODES {
  DRAIN, // fill the strands then remove lit pixels from the top
  // DRAIN_DROP, // drop a pixel from the bottom of the filled section
  // FILL, // start empty and light pixels up
  // FILL_DROP, // drop a pixel from the top down to fill the strand
  // FILL_SHOOT, // fire a pixel from the bottom up to fill the strand
  DROP, // drop a single pixel top to bottom
  // SHOOT, // fire a single pixel up each strand
  ALERT // flash strands to indicate time is up
} mode = DRAIN;

// main timing
ulong now;
uint lastUpdate = 0;

// drain mode vars
int secondsRemaining = SECONDS_STRAND_COUNT;
int minutesRemaining = MINUTES_STRAND_COUNT / 2;

// drop mode vars
bool shouldUpdateMinutes = false;
int secondsPosition;
int prevSecondsPosition;
int minutesPosition;
int prevMinutesPosition;
uint switchTime = 25;
uint hueBase = 0;

// alert mode vars
bool alertActive = false;
int alertCount = 10;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println("Setting up");

  minutesStrand.begin();
  minutesStrand.setBrightness(MINUTES_STRAND_BRIGHT);
  minutesStrand.fill();
  minutesStrand.show();

  secondsStrand.begin();
  secondsStrand.setBrightness(SECONDS_STRAND_BRIGHT);
  secondsStrand.fill();
  secondsStrand.show();

  if (mode == DRAIN) {
    secondsStrand.clear();
    minutesStrand.clear();
    secondsStrand.fill(0xFFFFFF, 0, secondsRemaining);
    minutesStrand.fill(0xFFFFFF, 0, minutesRemaining);
    secondsStrand.show();
    minutesStrand.show();
  }

  if (mode == DROP) {
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
}

void loop() {
  now = millis();

  if (now - lastUpdate > WAIT_TIME) {
    lastUpdate = now;

    if (mode == ALERT) {
      if (alertCount <= 0) {
        secondsStrand.fill(0xFF0000);
        minutesStrand.fill(0xFF0000);
        secondsStrand.show();
        minutesStrand.show();
        delay(100);
        return;
      }
      alertCount--;

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

    if (mode == DRAIN) {
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

    if (mode == DROP) {
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
          minutesPosition = minutesStrand.numPixels() - 1;
          minutesStrand.clear();
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
  }
}
