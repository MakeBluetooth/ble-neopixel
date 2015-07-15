// BLE NeoPixel
//
// Bluefruit LE http://adafru.it/1697
// NeoPixels LEDs http://adafru.it/1463
// arduino-BLEPeripheral https://github.com/sandeepmistry/arduino-BLEPeripheral.git
// Adafruit NeoPixel Driver https://github.com/adafruit/Adafruit_NeoPixel

#include <SPI.h>
#include <BLEPeripheral.h>
#include <Adafruit_NeoPixel.h>

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

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMBER_PIXELS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  Serial.println(F("Bluetooth Low Energy NeoPixel"));
  
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
  const unsigned char initialColor[3] = {0x00, 0x00, 0xFF}; // red, green, blue
  colorCharacteristic.setValue(initialColor, sizeof(initialColor));

  updateLights();
}

void loop() {
  // Tell the bluetooth radio to do whatever it should be working on
  blePeripheral.poll();
}

void colorCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic) {
  updateLights();
}

void brightnessCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic) {
  pixels.setBrightness(brightnessCharacteristic.value());
  updateLights(); 
}

void switchCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic) {
  processSwitchChange();
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

void processSwitchChange() {
  if (switchCharacteristic.value() == 1) {
    if (pixels.getBrightness() == 0) {
        brightnessCharacteristic.setValue(DEFAULT_BRIGHTNESS);
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


