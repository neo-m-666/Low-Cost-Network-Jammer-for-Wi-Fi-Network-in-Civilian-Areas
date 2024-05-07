#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RF24.h>

#define OLED_RESET 4
#define CE_PIN 9
#define CSN_PIN 10

RF24 radio(CE_PIN, CSN_PIN);

#define CHANNELS 64
int channel[CHANNELS];
bool jamming = false; // Declare 'jamming' variable
int channels = 0;     // Declare 'channels' variable

#define _NRF24_CONFIG 0x00
#define _NRF24_EN_AA 0x01
#define _NRF24_RF_CH 0x05
#define _NRF24_RF_SETUP 0x06
#define _NRF24_RPD 0x09

Adafruit_SSD1306 display(OLED_RESET);

void setRegister(byte r, byte v) {
  digitalWrite(CSN_PIN, LOW);
  SPI.transfer((r & 0x1F) | 0x20);
  SPI.transfer(v);
  digitalWrite(CSN_PIN, HIGH);
}

void powerUp() {
  setRegister(_NRF24_CONFIG, getRegister(_NRF24_CONFIG) | 0x02);
  delayMicroseconds(130);
}

byte getRegister(byte r) {
  byte c;

  digitalWrite(CSN_PIN, LOW);
  c = SPI.transfer(r & 0x1F);
  c = SPI.transfer(0);
  digitalWrite(CSN_PIN, HIGH);

  return (c);
}

void scanChannels() {
  disable();
  for (int j = 0; j < 255; j++) {
    for (int i = 0; i < CHANNELS; i++) {
      setRegister(_NRF24_RF_CH, (128 * i) / CHANNELS);

      setRX();

      delayMicroseconds(40);

      disable();

      if (getRegister(_NRF24_RPD) > 0) {
        channel[i]++;
        Serial.print("Detected activity on channel ");
        Serial.println(i + 1);
      }
    }
  }
}

void outputChannels() {
  int norm = 0;

  for (int i = 0; i < CHANNELS; i++)
    if (channel[i] > norm)
      norm = channel[i];

  Serial.print('|');
  for (int i = 0; i < CHANNELS; i++) {
    int pos;

    if (norm != 0)
      pos = (channel[i] * 10) / norm;
    else
      pos = 0;

    if (pos == 0 && channel[i] > 0)
      pos++;

    if (pos > 9)
      pos = 9;

    Serial.print(" .:-=+*aRW"[pos]);
    channel[i] = 0;
  }

  Serial.print("| ");
  Serial.println(norm);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Channel activity:");
  display.setCursor(0, 10);
  display.print("   |");
  for (int i = 0; i < CHANNELS; i++) {
    int barHeight = map(channel[i], 0, norm, 0, 32);
    display.drawPixel(5 + i, 10 + 32 - barHeight, WHITE);
  }
  display.display();
}

void setRX() {
  setRegister(_NRF24_CONFIG, getRegister(_NRF24_CONFIG) | 0x01);
  digitalWrite(CE_PIN, HIGH);
  delayMicroseconds(100);
}

void disable() {
  digitalWrite(CE_PIN, LOW);
}

void jammer() {
  const char text[] = "xxxxxxxxxxxxxxxx"; // send the noise
  for (int i = 1; i < 24; i++) {
    radio.setChannel(i);
    radio.write(&text, sizeof(text));
  }
}

void setup() {
  Serial.begin(57600);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  radio.begin();
  radio.startListening();
  radio.stopListening();

  pinMode(CE_PIN, OUTPUT);

  powerUp();

  setRegister(_NRF24_EN_AA, 0x0);
  setRegister(_NRF24_RF_SETUP, 0x0F);
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    handleSerialCommand(command);
  }

  scanChannels();
  outputChannels();

  delay(2000); // Adjust the delay based on your requirements
}

void handleSerialCommand(char command) {
  switch (command) {
    case 's':
      jamming = !jamming;
      if (jamming) {
        Serial.println("Jamming is ON");
      } else {
        Serial.println("Jamming is OFF");
      }
      break;
    case 'c':
      channels = (channels + 1) % 14;  // Cycle channels
      Serial.print("Cycling channels. New channel: ");
      Serial.println(channels + 1);
      break;
    default:
      break;
  }
}
