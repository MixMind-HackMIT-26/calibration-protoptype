#include <HX711.h>
#include <U8x8lib.h>
#include <EEPROM.h>
#include <math.h>
#include <stdlib.h>

HX711 sensor;
U8X8_SSD1306_128X64_NONAME_SW_I2C display(10, 11, U8X8_PIN_NONE);

struct Calibration {
  unsigned long magic;
  float countsPerGram;
};
const unsigned long CAL_MAGIC = 0x50534331UL;
float factor = 0;
long zero = 0;
float filtered = 0;
bool hasReading = false;
bool tared = false;
unsigned long lastSample = 0;
unsigned long lastReport = 0;
char command[32];
byte commandLength = 0;
bool overflow = false;
char operation = 0;
byte samples = 0;
long sum = 0;
float referenceGrams = 0;

void startCapture(char kind) {
  operation = kind;
  samples = 0;
  sum = 0;
  Serial.println(F("Keep load steady: collecting 20 samples..."));
}

void handleCommand() {
  if (operation) {
    Serial.println(F("BUSY: wait for capture to finish."));
    return;
  }
  if (!strcmp(command, "t") || !strcmp(command, "zero")) {
    startCapture('t');
  } else if (command[0] == 'c' && command[1] == ' ') {
    char *end;
    referenceGrams = strtod(command + 2, &end);
    while (*end == ' ') ++end;
    if (!tared || *end || !isfinite(referenceGrams) || referenceGrams <= 0) {
      Serial.println(F("ERROR: tare first, then use c <positive grams>."));
      return;
    }
    startCapture('c');
  } else {
    Serial.println(F("Commands: t = zero; c 100 = calibrate with added 100 g. Send newline."));
  }
}

void showReading(bool ready) {
  display.clearDisplay();
  display.drawString(0, 0, "PUMP SCALE");
  if (!ready) {
    display.drawString(0, 2, "SENSOR OFFLINE");
    return;
  }
  if (operation) {
    display.drawString(0, 2, operation == 't' ? "ZEROING..." : "CALIBRATING...");
    return;
  }
  if (!tared) {
    display.drawString(0, 2, "TARE REQUIRED");
    return;
  }
  char value[20];
  if (factor == 0) {
    display.drawString(0, 2, "UNCALIBRATED");
    ltoa((long)(filtered - zero), value, 10);
    display.drawString(0, 4, value);
    display.drawString(0, 5, "raw counts");
  } else {
    float grams = (filtered - zero) / factor;
    if (fabs(grams) > 99999) {
      display.drawString(0, 2, "OUT OF RANGE");
      return;
    }
    if (fabs(grams) < 0.05) grams = 0;
    dtostrf(grams, 1, 1, value);
    display.draw2x2String(0, 2, value);
    display.drawString(0, 5, "g / mL water");
  }
}

void setup() {
  Serial.begin(9600);
  sensor.begin(5, 6);
  display.begin();
  display.setFont(u8x8_font_chroma48medium8_r);
  Calibration saved;
  EEPROM.get(0, saved);
  if (saved.magic == CAL_MAGIC && isfinite(saved.countsPerGram) && fabs(saved.countsPerGram) > 0.000001) {
    factor = saved.countsPerGram;
  }
  Serial.println(F("Pump scale: DT=D5 SCK=D6; OLED SCL=D10 SDA=D11; 9600 baud"));
  Serial.println(F("t + Enter: zero empty container. c 100 + Enter: calibrate with added 100 g."));
  Serial.print(F("Saved counts/g: "));
  Serial.println(factor, 6);
}

void loop() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      if (overflow) Serial.println(F("ERROR: command too long."));
      else if (commandLength) {
        command[commandLength] = 0;
        handleCommand();
      }
      commandLength = 0;
      overflow = false;
    } else if (commandLength < sizeof(command) - 1) {
      command[commandLength++] = ch;
    } else overflow = true;
  }

  if (sensor.is_ready()) {
    long raw = sensor.read();
    filtered = hasReading ? filtered + 0.35f * (raw - filtered) : raw;
    hasReading = true;
    lastSample = millis();
    if (operation) {
      sum += raw;
      if (++samples == 20) {
        long average = sum / samples;
        if (operation == 't') {
          zero = average;
          filtered = average;
          tared = true;
          Serial.println(F("OK: zero set."));
        } else {
          long delta = average - zero;
          if (labs(delta) < 100) {
            Serial.println(F("ERROR: load change too small. Calibration not saved."));
          } else {
            float candidate = delta / referenceGrams;
            if (isfinite(candidate) && fabs(candidate) > 0.000001) {
              factor = candidate;
              Calibration saved = {CAL_MAGIC, factor};
              EEPROM.put(0, saved);
              filtered = average;
              Serial.print(F("OK: saved counts/g = "));
              Serial.println(factor, 6);
            } else Serial.println(F("ERROR: invalid calibration factor."));
          }
        }
        operation = 0;
      }
    }
  }

  if (millis() - lastReport >= 500) {
    lastReport = millis();
    bool ready = hasReading && millis() - lastSample < 2000;
    if (!ready && operation && millis() - lastSample >= 3000) {
      operation = 0;
      Serial.println(F("ERROR: capture cancelled, sensor offline."));
    }
    showReading(ready);
    if (!ready) Serial.println(F("SENSOR OFFLINE"));
    else if (operation) Serial.println(F("CAPTURING"));
    else if (!tared) Serial.println(F("TARE REQUIRED: send t"));
    else if (factor == 0) {
      Serial.print(F("raw_net="));
      Serial.println(filtered - zero, 0);
    } else {
      Serial.print(F("grams="));
      Serial.print((filtered - zero) / factor, 1);
      Serial.print(F(", water_ml="));
      Serial.println((filtered - zero) / factor, 1);
    }
  }
}
