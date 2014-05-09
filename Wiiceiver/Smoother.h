#ifndef SMOOTHER_H
#define SMOOTHER_H

class Smoother {
  private:
    float value;
    float factor;
    
  public:
    Smoother(float newFactor) {
      factor = newFactor;
      value = 0;
    }

#define MIN_STEP 0.005

    float compute(float target) {
      float step = (target - value) * factor;
      
#ifdef DEBUGGING_SMOOTHER
      Serial.print("Target: ");
      Serial.print(target, 4);
      Serial.print(", Factor: ");
      Serial.print(factor, 4);
      Serial.print(", Value: ");
      Serial.print(value, 4);      
      Serial.print(", Step: ");
      Serial.print(step, 4);
#endif

      if (abs(step) < MIN_STEP) {
#ifdef DEBUGGING_SMOOTHER
      Serial.print(" BUMP");
#endif
        value = target;
      } else {
        value += step;
      }
      // value = (float)round(value * 10000) / 10000.0;
#ifdef DEBUGGING_SMOOTHER
      Serial.print(", result ");
      Serial.println(value, 4);
#endif
      return value;
    }
    
    void zero() {
      value = 0;
    }
};

#endif
