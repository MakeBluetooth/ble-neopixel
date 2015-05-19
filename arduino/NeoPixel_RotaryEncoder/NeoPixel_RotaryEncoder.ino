// NeoPixel -  Set the brightness with a rotary encoder 
// Test Sketch without Bluetooth 
//
// NeoPixels LEDs http://adafru.it/1463
// Encoder Library http://www.pjrc.com/teensy/td_libs_Encoder.html

#include <Adafruit_NeoPixel.h>
#include <Encoder.h>

#define NUMBER_PIXELS 16
#define NEO_PIXEL_PIN 6

#define PIN_ENCODER_A 4
#define PIN_ENCODER_B 3
#define BRIGHTNESS_PER_CLICK 3

uint32_t color;
uint8_t brightness = 2; // need better name

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMBER_PIXELS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

Encoder encoder(PIN_ENCODER_B, PIN_ENCODER_A);

void setup() {
  Serial.begin(9600);
  Serial.println(F("Rotary Encoder NeoPixel"));
  
  encoder.write(brightness); // initial value
  
  // set up neopixels
  pinMode(NEO_PIXEL_PIN, OUTPUT);  
  pixels.begin();
  pixels.setBrightness(encoder.read() * BRIGHTNESS_PER_CLICK);
  color = pixels.Color(0,255,0); // green
  repaint();
}

void loop() {
  long val = encoder.read();
  if (val != brightness) {
    Serial.println(val);
    if (val < 0) {
      val = 0;
      encoder.write(val);
    }
    if (val > 255/BRIGHTNESS_PER_CLICK) {
      val = 255/BRIGHTNESS_PER_CLICK;
      encoder.write(val);
    }
    brightness = val;
    Serial.println(brightness);
    pixels.setBrightness(brightness * BRIGHTNESS_PER_CLICK);
    repaint(); // only really need if goes to 0
  }
}

void repaint() {
  for (int i = 0; i < NUMBER_PIXELS; i++) {
    pixels.setPixelColor(i, color); 
  }
  pixels.show();
}

