// NeoPixel -  Set the brightness with a rotary encoder 
// Button to turn on and off
// 
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

#define BUTTON_PIN 7

uint32_t color;
uint8_t brightness = 2; // need better name
int buttonState = 0;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMBER_PIXELS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

Encoder encoder(PIN_ENCODER_B, PIN_ENCODER_A);

void setup() {
  Serial.begin(9600);
  Serial.println(F("Rotary Encoder NeoPixel"));
  
  // initialize the pushbutton pin as an input:
  pinMode(BUTTON_PIN, INPUT);  
  
  // set initial value for rotary encoder
  encoder.write(brightness);
  
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
  
  // read the state of the pushbutton value:
  buttonState = digitalRead(BUTTON_PIN);

  // check if the pushbutton is pressed.
  if (buttonState == HIGH) {     
    Serial.println("HIGH");
    // toggle state
    if (pixels.getBrightness() == 0) { // turn on
      brightness = 128/BRIGHTNESS_PER_CLICK;
      encoder.write(brightness);
      pixels.setBrightness(brightness * BRIGHTNESS_PER_CLICK);
      repaint();
    } else { // turn off
      pixels.setBrightness(0);
      encoder.write(0);
      repaint();
    }
    
    delay(200);  // TODO DEBOUNCE
  } 
}

void repaint() {
  for (int i = 0; i < NUMBER_PIXELS; i++) {
    pixels.setPixelColor(i, color); 
  }
  pixels.show();
}

