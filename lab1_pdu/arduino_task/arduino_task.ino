
#define LEDPIN 13
//#define USE_PWM true
#define USE_COUNTER
int ontime = 200;

void setup() {
  Serial.begin(9600);
  pinMode(LEDPIN, OUTPUT);

  // This is bad practice, we should wire it to another pin but as this
  // is the task we bind led_isr to LEDPIN state change
  //
  // Binding it to the same pin will stop us from triggering the event when ever
  // we use USE_PWM do tim the diode.
  attachInterrupt(
    digitalPinToInterrupt(LEDPIN), led_isr,
    CHANGE);

// If we are using pwm set an initial value of duty cycle
#ifdef USE_PWM
  analogWrite(LEDPIN, 125);
#endif
}

/// A simple mutex over a number
typedef struct Mutex {
  bool locked;
  unsigned long start;
    unsigned int counter;
} mutex;

// Global mutex lock for the start
mutex semaphore = {
  false,
  NULL
    ,0
};

void lock(struct Mutex *m) {
  // Wait until a release has been called
  while (m->locked) {
    yield();
  }
}

void release(struct Mutex *m) {
  m->locked = false;
}

void timing(unsigned long time) {
  lock(&semaphore);

  if (semaphore.start == NULL) {
    semaphore.start = time;
  } else {
    float secs = (time - semaphore.start) / 1000.f;
    semaphore.start = NULL;
    Serial.print("Blink took ");
    Serial.print(secs);
    Serial.print(" seconds");
    #ifdef USE_COUNTER
      // Count on trailing edge.
      semaphore.counter += 1;
      Serial.print("\nNumber of blinks : ");
      Serial.print(semaphore.counter);
    #endif
    Serial.println("");
  }

  release(&semaphore);
}

void led_isr() {
  unsigned long time = millis();
  // Call a function to clear interrupt pending bit
  // as fast as possible.
  timing(time);
}

void loop() {
  int num = 0;
#ifdef USE_PWM
  // If we are using USE_PWM we yeild to any other thread,
  // as this is not a threaded enviornment this is equivalent
  // to a nop
  while (!Serial.available()) {
    yield();
  }
#else
  // If we are toggeling the pin we
  // set it high, wait for ontime
  // after ontime we set it low
  // thus producing the blinking.
  digitalWrite(LEDPIN, HIGH);
  delay(ontime);
  digitalWrite(LEDPIN, LOW);
  // Get current time
  unsigned long start = millis();
#endif


  bool condition = Serial.available();

  // Will loop forever until we get a return
  while (condition) {
    char byte = Serial.read();
    // ASCII for return
    if (byte == 10) {
      break;
    }
    // If it is not a number we set num to -1 singifying
    // som error occured
    if (byte < 48 || byte > 57) {
      Serial.print("Only supply numbers\n");
      num = -1;
      break;
    }

    // Re build base 10 number
    num = num * 10 + (byte - 48);

    // Allways read until newline.
    while (!Serial.available()) {
      // Yield is equivalent to a asm::Nop here as we are not in a
      // threaded enviornment
      yield();
    }
  }

#ifdef USE_PWM
  // If we are using PWM we only want to accept values inbetween 0 and 255
  if (num >= 0 && ontime != num && num <= 255) {
    Serial.print("Setting duty cycle to : ");
    Serial.print(num);
    Serial.print(" \n");
    analogWrite(LEDPIN, num);
    ontime = num;
  } else if (num >= 0 && ontime != num) {
    Serial.print("Maximum duty cycle is 255\n");
  }
  // Setting the value to the same value has no effect so no need of informing the user
#else
  // Only accept values larger than 10.
  if ((num != 0) && num > 10) {
    ontime = num;
  } else if (num != 0 && num <= 10) {
    Serial.print("On time has be be larger than 10 uS\n");
  }

  unsigned long now = millis();
  unsigned long delay_time = now - start + ontime;
  if (delay_time > 0) {
    delay(delay_time);
  }

#endif
}
