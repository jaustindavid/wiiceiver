#ifndef ELECTRONICSPEEDCONTROLLER_H
#define ELECTRONICSPEEDCONTROLLER_H

/*
 *    ESC wrapper class
 */

class ElectronicSpeedController {
#define ESC_CENTER 90       // angle of the "center"; probably always 90
#define ESC_MAX_ANGLE 135   // angle of "max" deflection

private:
  Servo _esc;
  int angle;   // the angle most recently written to _esc;
  
public:

void init(int pin) {
  angle = ESC_CENTER;
  
#ifdef DEBUGGING_ESC
  Serial.print("attaching to pin #");
  Serial.println(pin);
#endif

  _esc.attach(pin);
  
#ifdef DEBUGGING
  Serial.println("initializing ESC...");
#endif
  for (int i = 0; i < 1000; i+= 20) {
    _esc.write(ESC_CENTER);
    delay(20);
  }

  delay(100);
#ifdef DEBUGGING_ESC
  Serial.println("done");
#endif
}


/*
 * input: -1 .. 1
 * output: writes +/- ESC_MAX_ANGLE to _esc
 * does *not* write the same angle twice -- possible interference with the PWM :(
 */
void setLevel(float level) {
  int newAngle = (int)(ESC_CENTER + (ESC_MAX_ANGLE - ESC_CENTER) * level);
  if (newAngle != angle) {
#ifdef DEBUGGING_ESC
    Serial.print(millis());
    Serial.print(F(": ESC angle: "));
    Serial.println(newAngle);
#endif
    angle = newAngle;
    _esc.write(angle);
  }
} // void setLevel(float level)


private:
// unused code
#define STEP_DELAY 20
  void _sweep(void) {
    setLevel(0);
    delay(STEP_DELAY);
    for (float level = 0; level <= 1.0; level += 0.1) {
      setLevel(level);
      delay(STEP_DELAY);
    }
    for (float level = 1.0; level >= -1.0; level -= 0.1) {
      setLevel(level);
      delay(STEP_DELAY);
    }
    for (float level = -1.0; level <= 0; level += 0.1) {
      setLevel(level);
      delay(STEP_DELAY);
    }
    setLevel(0);
    delay(STEP_DELAY);
  } // void _sweep(void)
};  // class ElectronicSpeedController 

#endif
