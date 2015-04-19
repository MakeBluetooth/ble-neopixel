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
#define BLE_RST 7

BLEPeripheral blePeripheral = BLEPeripheral(BLE_REQ, BLE_RDY, BLE_RST);
BLEService neoPixelService = BLEService("9fd73ae0-b743-48df-9b81-04840eb11b73");

BLECharacteristic colorCharacteristic = BLECharacteristic("9fd73ae1-b743-48df-9b81-04840eb11b73", BLERead | BLEWrite, 3);
BLEDescriptor colorDescriptor = BLEDescriptor("2901", "Color (24-bit)");

BLEUnsignedCharCharacteristic redCharacteristic = BLEUnsignedCharCharacteristic("9fd73ae2-b743-48df-9b81-04840eb11b73", BLERead | BLEWrite);
BLEDescriptor redDescriptor = BLEDescriptor("2901", "Red");
BLEUnsignedCharCharacteristic greenCharacteristic = BLEUnsignedCharCharacteristic("9fd73ae3-b743-48df-9b81-04840eb11b73", BLERead | BLEWrite);
BLEDescriptor greenDescriptor = BLEDescriptor("2901", "Green");
BLEUnsignedCharCharacteristic blueCharacteristic = BLEUnsignedCharCharacteristic("9fd73ae4-b743-48df-9b81-04840eb11b73", BLERead | BLEWrite);
BLEDescriptor blueDescriptor = BLEDescriptor("2901", "Blue");

#define NUMBER_PIXELS 16
#define NEO_PIXEL_PIN 6

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMBER_PIXELS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);
uint16_t color;

void setup() {
  Serial.begin(9600);
#if defined (__AVR_ATmega32U4__)
  delay(5000);  // leonardo needs delay for Serial to start up 
#endif
  Serial.println(F("Bluetooth Low Energy NeoPixel"));
  
  pinMode(NEO_PIXEL_PIN, OUTPUT);  
  pixels.begin();
  pixels.setBrightness(0x3F); // 25%

  // set advertised name and service
  blePeripheral.setLocalName("NeoPixels");
  blePeripheral.setAdvertisedServiceUuid(neoPixelService.uuid());

  // add service and characteristic
  blePeripheral.addAttribute(neoPixelService);
  blePeripheral.addAttribute(colorCharacteristic);
  blePeripheral.addAttribute(colorDescriptor);
  blePeripheral.addAttribute(redCharacteristic);
  blePeripheral.addAttribute(redDescriptor);
  blePeripheral.addAttribute(greenCharacteristic);
  blePeripheral.addAttribute(greenDescriptor);
  blePeripheral.addAttribute(blueCharacteristic);
  blePeripheral.addAttribute(blueDescriptor);
  
  colorCharacteristic.setEventHandler(BLEWritten, colorCharacteristicWritten);

  // one handler for all colors
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

// break down the 24-bit color into red, green and blue components
void colorCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic) {
  const unsigned char* color = characteristic.value();
  redCharacteristic.setValue(color[0]);
  blePeripheral.poll();
  greenCharacteristic.setValue(color[1]);
  blePeripheral.poll();
  blueCharacteristic.setValue(color[2]);
  blePeripheral.poll();
  repaint();
}

void colorChanged(BLECentral& central, BLECharacteristic& characteristic) {
  repaint();
}

void repaint() {
  
  uint8_t red = redCharacteristic.value();
  uint8_t green = greenCharacteristic.value();
  uint8_t blue = blueCharacteristic.value();
  
  // keep 24-bit color in sync
  const unsigned char c[] = {red, green, blue};
  Serial.print("R - ");
  Serial.print(c[0], HEX);
  Serial.print(", G - ");
  Serial.print(c[1], HEX);
  Serial.print(", B - ");
  Serial.println(c[2], HEX);
  colorCharacteristic.setValue(c, 3);
  
  uint32_t color = pixels.Color(red, green, blue);
  for (int i = 0; i < NUMBER_PIXELS; i++) {
    pixels.setPixelColor(i, color); 
  }
  pixels.show();
}

