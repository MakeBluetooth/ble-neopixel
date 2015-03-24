// BLE NeoPixel
//
// RedBear Lab BLE Shield http://redbearlab.com/bleshield/
// NeoPixels LEDs http://adafru.it/1463
// arduino-BLEPeripheral https://github.com/sandeepmistry/arduino-BLEPeripheral.git
// Adafruit NeoPixel Driver https://github.com/adafruit/Adafruit_NeoPixel

#include <SPI.h>
#include <BLEPeripheral.h>
#include <Adafruit_NeoPixel.h>

// See BLE Peripheral documentation for setting for your hardware
// https://github.com/sandeepmistry/arduino-BLEPeripheral#pinouts

// BLE Shield 2.x
#define BLE_REQ 9
#define BLE_RDY 8
#define BLE_RST UNUSED

BLEPeripheral blePeripheral = BLEPeripheral(BLE_REQ, BLE_RDY, BLE_RST);
BLEService neoPixelService = BLEService("dbc0");

BLEUnsignedCharCharacteristic redCharacteristic = BLEUnsignedCharCharacteristic("dbc3", BLERead | BLEWrite);
BLEDescriptor redDescriptor = BLEDescriptor("2901", "Red");
BLEUnsignedCharCharacteristic greenCharacteristic = BLEUnsignedCharCharacteristic("dbc4", BLERead | BLEWrite);
BLEDescriptor greenDescriptor = BLEDescriptor("2901", "Green");
BLEUnsignedCharCharacteristic blueCharacteristic = BLEUnsignedCharCharacteristic("dbc5", BLERead | BLEWrite);
BLEDescriptor blueDescriptor = BLEDescriptor("2901", "Blue");

#define NUMBER_PIXELS 16
#define NEO_PIXEL_PIN 6

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMBER_PIXELS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);
uint16_t color;

void setup() {
  Serial.begin(9600);
  Serial.println(F("Bluetooth Low Energy NeoPixel"));
  
  pinMode(NEO_PIXEL_PIN, OUTPUT);  
  pixels.begin();
  pixels.setBrightness(0x3F); // 25%

  // set advertised name and service
  blePeripheral.setLocalName("NeoPixels");
  blePeripheral.setAdvertisedServiceUuid(neoPixelService.uuid());

  // add service and characteristic
  blePeripheral.addAttribute(neoPixelService);
  blePeripheral.addAttribute(redCharacteristic);
  blePeripheral.addAttribute(redDescriptor);
  blePeripheral.addAttribute(greenCharacteristic);
  blePeripheral.addAttribute(greenDescriptor);
  blePeripheral.addAttribute(blueCharacteristic);
  blePeripheral.addAttribute(blueDescriptor);
  
  redCharacteristic.setEventHandler(BLEWritten, colorChanged);
  greenCharacteristic.setEventHandler(BLEWritten, colorChanged);
  blueCharacteristic.setEventHandler(BLEWritten, colorChanged);

  // begin initialization
  blePeripheral.begin();
}

void loop() {
  // Tell the bluetooth radio to do whatever it should be working on
  blePeripheral.poll();
}

void colorChanged(BLECentral& central, BLECharacteristic& characteristic) {
  repaint();
}

void repaint() {
  
  uint8_t red = redCharacteristic.value();
  uint8_t green = greenCharacteristic.value();
  uint8_t blue = blueCharacteristic.value();
  
  uint32_t color = pixels.Color(red, green, blue);
  for (int i = 0; i < NUMBER_PIXELS; i++) {
    pixels.setPixelColor(i, color); 
  }
  pixels.show();
}

