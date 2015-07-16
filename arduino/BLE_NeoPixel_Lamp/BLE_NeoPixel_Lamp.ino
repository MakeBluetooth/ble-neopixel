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

BLEService neoPixelService = BLEService("ccc0");

BLECharacteristic colorCharacteristic = BLECharacteristic("ccc1", BLERead | BLEWrite, 3);
BLEDescriptor colorDescriptor = BLEDescriptor("2901", "Color (24-bit)");
BLEUnsignedCharCharacteristic brightnessCharacteristic = BLEUnsignedCharCharacteristic("ccc2", BLERead | BLEWrite | BLENotify);
BLEDescriptor brightnessDescriptor = BLEDescriptor("2901", "Brightness");
BLEUnsignedCharCharacteristic switchCharacteristic = BLEUnsignedCharCharacteristic("ccc3", BLERead | BLEWrite | BLENotify);
BLEDescriptor switchDescriptor = BLEDescriptor("2901", "Power Switch");

#define NUMBER_PIXELS 16
#define NEO_PIXEL_PIN 6
#define DEFAULT_BRIGHTNESS 0x3F // 25%
#define MAX_BRIGHTNESS 0xFF

#define PIN_ENCODER_A 3
#define PIN_ENCODER_B 4
#define BRIGHTNESS_PER_CLICK 3

#define BUTTON_PIN 7

uint8_t encoderValue;
int buttonState = 0;

long previousMillis = 0;  // stores the last time sensor was read
long interval = 100;      // interval at which to read sensor (milliseconds)

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMBER_PIXELS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);
Encoder encoder(PIN_ENCODER_A, PIN_ENCODER_B);

void setup() {
  Serial.begin(9600);
  Serial.println(F("Bluetooth Low Energy NeoPixel Lamp"));

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
  brightnessCharacteristic.setEventHandler(BLEWritten, brightnessCharacteristicWritten);
  switchCharacteristic.setEventHandler(BLEWritten, switchCharacteristicWritten);

  blePeripheral.begin();

  // initial brightness and color  
  brightnessCharacteristic.setValue(DEFAULT_BRIGHTNESS);
  pixels.setBrightness(DEFAULT_BRIGHTNESS);
  setEncoderBrightness(DEFAULT_BRIGHTNESS);
  const unsigned char initialColor[3] = {0x00, 0x00, 0xFF}; // red, green, blue
  colorCharacteristic.setValue(initialColor, sizeof(initialColor));

  updateLights();
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
  updateLights();
}

void brightnessCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic) {
  pixels.setBrightness(brightnessCharacteristic.value());
  setEncoderBrightness(brightnessCharacteristic.value());
  updateLights(); 
}

void switchCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic) {
  processSwitchChange();
}

void processSwitchChange() {
  if (switchCharacteristic.value() == 1) {
    if (pixels.getBrightness() == 0) {
        brightnessCharacteristic.setValue(DEFAULT_BRIGHTNESS);
        setEncoderBrightness(DEFAULT_BRIGHTNESS);
        pixels.setBrightness(DEFAULT_BRIGHTNESS);
    }
    // updateLights uses the last color and brightness
    updateLights();
  } else if (switchCharacteristic.value() == 0) {
    // turn all pixels off
    for (int i = 0; i < NUMBER_PIXELS; i++) {
      pixels.setPixelColor(i, 0); 
    }
    pixels.show();  
  }
}

void updateLights() {
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

  // ensure the switch characteristic is correct
  if (switchCharacteristic.value() == 0 && pixels.getBrightness() > 0) {
    switchCharacteristic.setValue(1); // light is on    
  } else if (pixels.getBrightness() == 0 && switchCharacteristic.value() == 1) {
    switchCharacteristic.setValue(0); // light is off
  }
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
    updateLights();
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


