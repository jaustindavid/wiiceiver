#include <EEPROM.h>

#define RED_LED 8
#define GREEN_LED 6

// returns true if the value at the specified location is set correctly
boolean readValues(int address, byte desiredValue) {
  byte storedValue = EEPROM.read(address);
  if (storedValue != desiredValue) {
    EEPROM.write(address, desiredValue);
  }
  
  return EEPROM.read(address) == desiredValue;
}

void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  if (fixValue(0, 255)) {
    digitalWrite(RED_LED, HIGH);
  }
  
  if (fixValue(1, 255)) {
    digitalWrite(GREEN_LED, HIGH);
  }

}

void loop() {
}
