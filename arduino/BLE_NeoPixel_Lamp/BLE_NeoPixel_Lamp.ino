// BLE NeoPixel
//
// Bluefruit LE http://adafru.it/1697
// NeoPixels LEDs http://adafru.it/1463
// arduino-BLEPeripheral https://github.com/sandeepmistry/arduino-BLEPeripheral.git
// Adafruit NeoPixel Driver https://github.com/adafruit/Adafruit_NeoPixel
// Encoder Library http://www.pjrc.com/teensy/td_libs_Encoder.html

#include <SPI.h>
#include <BLEPeripheral.h>
#include <Adafruit_NeoPixel.h>
#include <Encoder.h>

// See BLE Peripheral documentation for setting for your hardware
// https://github.com/sandeepmistry/arduino-BLEPeripheral#pinouts
#define BLE_REQ 10
#define BLE_RDY 2
#define BLE_RST 9

BLEPeripheral blePeripheral = BLEPeripheral(BLE_REQ, BLE_RDY, BLE_RST);
BLEService neoPixelService = BLEService("9fd73ae0-b743-48df-9b81-04840eb11b73");

BLECharacteristic colorCharacteristic = BLECharacteristic("9fd73ae1-b743-48df-9b81-04840eb11b73", BLERead | BLEWrite, 3);
BLEDescriptor colorDescriptor = BLEDescriptor("2901", "Color (24-bit)");

BLEUnsignedCharCharacteristic brightnessCharacteristic = BLEUnsignedCharCharacteristic("9fd73ae5-b743-48df-9b81-04840eb11b73", BLERead | BLEWrite | BLENotify);
BLEDescriptor brightnessDescriptor = BLEDescriptor("2901", "Brightness");
BLEUnsignedCharCharacteristic switchCharacteristic = BLEUnsignedCharCharacteristic("9fd73ae6-b743-48df-9b81-04840eb11b73", BLERead | BLEWrite | BLENotify);
BLEDescriptor switchDescriptor = BLEDescriptor("2901", "Power Switch");

#define NUMBER_PIXELS 16
#define NEO_PIXEL_PIN 6

#define PIN_ENCODER_A 4
#define PIN_ENCODER_B 3
#define BRIGHTNESS_PER_CLICK 3

#define BUTTON_PIN 7

#define DEFAULT_BRIGHTNESS 0x3F
#define MAX_BRIGHTNESS 0xFF

uint8_t encoderValue;
int buttonState = 0;

long previousMillis = 0;  // stores the last time sensor was read
long interval = 100;      // interval at which to read sensor (milliseconds)

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMBER_PIXELS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);
Encoder encoder(PIN_ENCODER_B, PIN_ENCODER_A);

void setup() {
  Serial.begin(9600);
  Serial.println(F("Bluetooth Low Energy NeoPixel"));

  // initialize the pushbutton pin as an input:
  pinMode(BUTTON_PIN, INPUT);  
  
  pinMode(NEO_PIXEL_PIN, OUTPUT);  
  pixels.begin();

  // set advertised name and service
  blePeripheral.setDeviceName("NeoPixels");
  blePeripheral.setLocalName("NeoPixels");
  blePeripheral.setAdvertisedServiceUuid(neoPixelService.uuid());

  // add service and characteristic
  blePeripheral.addAttribute(neoPixelService);
  blePeripheral.addAttribute(colorCharacteristic);
  blePeripheral.addAttribute(colorDescriptor);
  blePeripheral.addAttribute(brightnessCharacteristic);
  blePeripheral.addAttribute(brightnessDescriptor);
  blePeripheral.addAttribute(switchCharacteristic);
  blePeripheral.addAttribute(switchDescriptor);
  
  // handlers for when clients change data
  colorCharacteristic.setEventHandler(BLEWritten, colorCharacteristicWritten);
  brightnessCharacteristic.setEventHandler(BLEWritten, brightnessChanged);
  switchCharacteristic.setEventHandler(BLEWritten, switchChanged);

  blePeripheral.begin();

  // initial brightness and color  
  brightnessCharacteristic.setValue(DEFAULT_BRIGHTNESS);
  setEncoderBrightness(DEFAULT_BRIGHTNESS);
  const unsigned char initialColor[3] = {0x00, 0x00, 0xFF}; // red, green, blue
  colorCharacteristic.setValue(initialColor, sizeof(initialColor));

  repaint();
}

void loop() {
  // Tell the bluetooth radio to do whatever it should be working on
  blePeripheral.poll();

  // limit how often we read the rotary encoder and button
  if (millis() - previousMillis > interval) {
    readEncoder();
    readButton();
    previousMillis = millis();
  }
  
}

void colorCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic) {
  repaint();
}

void brightnessChanged(BLECentral& central, BLECharacteristic& characteristic) {
  pixels.setBrightness(brightnessCharacteristic.value());
  setEncoderBrightness(brightnessCharacteristic.value());
  repaint(); 
}

void switchChanged(BLECentral& central, BLECharacteristic& characteristic) {
  processSwitchChange();
}

void repaint() {
  // get the color array from the characteristic  
  const unsigned char* rgb = colorCharacteristic.value();
  uint8_t red = rgb[0];
  uint8_t green = rgb[1];
  uint8_t blue = rgb[2];

  // change the color of the lights
  uint32_t color = pixels.Color(red, green, blue);
  for (int i = 0; i < NUMBER_PIXELS; i++) {
    pixels.setPixelColor(i, color); 
  }
  pixels.show();

  if (switchCharacteristic.value() == 0 && pixels.getBrightness() > 0) {
    switchCharacteristic.setValue(1); // light is on    
  } else if (pixels.getBrightness() == 0 && switchCharacteristic.value() == 1) {
    switchCharacteristic.setValue(0); // light is off
  }

}

void processSwitchChange() {
  Serial.print("Switch ");
  Serial.println(switchCharacteristic.value());
  if (switchCharacteristic.value() == 1) { 
    if (pixels.getBrightness() == 0) {
      setBrightness(DEFAULT_BRIGHTNESS);
    }
    repaint();
  } else if (switchCharacteristic.value() == 0) {
    // turn all pixels off
    for (int i = 0; i < NUMBER_PIXELS; i++) {
      pixels.setPixelColor(i, 0); 
    }
    pixels.show();  
  }

}

void setBrightness(uint8_t brightness) {
  brightnessCharacteristic.setValue(brightness);
  setEncoderBrightness(brightness);
  pixels.setBrightness(brightness);
}

void setEncoderBrightness(uint8_t brightness) {
  encoder.write(brightness / BRIGHTNESS_PER_CLICK);
}

void readEncoder() {
  long val = encoder.read();

  if (val != encoderValue) { // value changed
    // don't go below 0
    if (val < 0) {
      val = 0;
      encoder.write(val);
    }
    // don't go above max
    if (val > MAX_BRIGHTNESS/BRIGHTNESS_PER_CLICK) {
      val = MAX_BRIGHTNESS/BRIGHTNESS_PER_CLICK;
      encoder.write(val);
    }
    
    encoderValue = val;
    Serial.println(encoderValue);
    
    //sync the characteristic
    uint8_t brightness = encoderValue * BRIGHTNESS_PER_CLICK;    
    brightnessCharacteristic.setValue(brightness);
    pixels.setBrightness(brightness);
    repaint();
  }
}

void readButton() {
  // read the state of the pushbutton value:
  buttonState = digitalRead(BUTTON_PIN);

  // check if the pushbutton is pressed.
  if (buttonState == HIGH) {

    if (switchCharacteristic.value() > 0) {
      // light was on, turn it off
      switchCharacteristic.setValue(0);     
    } else {
      // light was off, turn it on
      switchCharacteristic.setValue(1);
    }

    processSwitchChange();
    delay(200);
  } 
}


