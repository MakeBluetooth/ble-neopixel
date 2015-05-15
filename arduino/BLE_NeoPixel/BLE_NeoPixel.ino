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
BLEService neoPixelService = BLEService("9fd73ae0-b743-48df-9b81-04840eb11b73");

BLECharacteristic colorCharacteristic = BLECharacteristic("9fd73ae1-b743-48df-9b81-04840eb11b73", BLERead | BLEWrite, 3);
BLEDescriptor colorDescriptor = BLEDescriptor("2901", "Color (24-bit)");

BLEUnsignedCharCharacteristic redCharacteristic = BLEUnsignedCharCharacteristic("9fd73ae2-b743-48df-9b81-04840eb11b73", BLERead | BLEWrite);
BLEDescriptor redDescriptor = BLEDescriptor("2901", "Red");
BLEUnsignedCharCharacteristic greenCharacteristic = BLEUnsignedCharCharacteristic("9fd73ae3-b743-48df-9b81-04840eb11b73", BLERead | BLEWrite);
BLEDescriptor greenDescriptor = BLEDescriptor("2901", "Green");
BLEUnsignedCharCharacteristic blueCharacteristic = BLEUnsignedCharCharacteristic("9fd73ae4-b743-48df-9b81-04840eb11b73", BLERead | BLEWrite);
BLEDescriptor blueDescriptor = BLEDescriptor("2901", "Blue");

BLEUnsignedCharCharacteristic brightnessCharacteristic = BLEUnsignedCharCharacteristic("9fd73ae5-b743-48df-9b81-04840eb11b73", BLERead | BLEWrite);
BLEDescriptor brightnessDescriptor = BLEDescriptor("2901", "Brightness");
BLEUnsignedCharCharacteristic switchCharacteristic = BLEUnsignedCharCharacteristic("9fd73ae6-b743-48df-9b81-04840eb11b73", BLERead | BLEWrite);
BLEDescriptor switchDescriptor = BLEDescriptor("2901", "Power Switch");

#define NUMBER_PIXELS 16
#define NEO_PIXEL_PIN 6

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMBER_PIXELS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  Serial.println(F("Bluetooth Low Energy NeoPixel"));
  
  pinMode(NEO_PIXEL_PIN, OUTPUT);  
  pixels.begin();

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
  blePeripheral.addAttribute(brightnessCharacteristic);
  blePeripheral.addAttribute(brightnessDescriptor);
  blePeripheral.addAttribute(switchCharacteristic);
  blePeripheral.addAttribute(switchDescriptor);
  
  colorCharacteristic.setEventHandler(BLEWritten, colorCharacteristicWritten);

  // one handler for all colors
  redCharacteristic.setEventHandler(BLEWritten, colorChanged);
  greenCharacteristic.setEventHandler(BLEWritten, colorChanged);
  blueCharacteristic.setEventHandler(BLEWritten, colorChanged);
  
  brightnessCharacteristic.setEventHandler(BLEWritten, brightnessChanged);
  switchCharacteristic.setEventHandler(BLEWritten, switchChanged);

  blePeripheral.begin();

  // initial brightness and color  
  brightnessCharacteristic.setValue(0x3F); // 25%
  blueCharacteristic.setValue(0xFF);
  repaint();
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

void brightnessChanged(BLECentral& central, BLECharacteristic& characteristic) {
  pixels.setBrightness(brightnessCharacteristic.value());
  repaint(); 
}

void switchChanged(BLECentral& central, BLECharacteristic& characteristic) {
  if (switchCharacteristic.value() == 1) {
    // repainting uses the last color and brightness
    repaint();
  } else if (switchCharacteristic.value() == 0) {
    // turn all pixels off
    for (int i = 0; i < NUMBER_PIXELS; i++) {
      pixels.setPixelColor(i, 0); 
    }
    pixels.show();  
  }
}

void repaint() {
  
  uint8_t red = redCharacteristic.value();
  uint8_t green = greenCharacteristic.value();
  uint8_t blue = blueCharacteristic.value();
  
  // keep 24-bit color in sync
  const unsigned char c[] = {red, green, blue};
  colorCharacteristic.setValue(c, 3);
  blePeripheral.poll();
  switchCharacteristic.setValue(1); // light is on

  // change the color of the lights
  uint32_t color = pixels.Color(red, green, blue);
  for (int i = 0; i < NUMBER_PIXELS; i++) {
    pixels.setPixelColor(i, color); 
  }
  pixels.show();
}

