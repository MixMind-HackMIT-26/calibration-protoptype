#include <HX711.h>

const byte DATA_PIN = 5;
const byte CLOCK_PIN = 6;
HX711 sensor;

void setup() {
  Serial.begin(9600);
  sensor.begin(DATA_PIN, CLOCK_PIN);
  Serial.println("HX711 raw readings, channel A, gain 128");
  Serial.println("millis,raw");
}

void loop() {
  if (sensor.wait_ready_timeout(1000)) {
    const long raw = sensor.read();
    Serial.print(millis());
    Serial.print(',');
    Serial.println(raw);
  } else {
    Serial.println("HX711 not ready: check power, common GND, DT=D5, SCK=D6");
  }
  delay(250);
}
