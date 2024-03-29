#define LEDPIN 13
#define ECHOPIN 12
#define PWM true
#define LOOPBACK true
int ontime = 200;

void setup() {
  Serial.begin(9600);
  pinMode(LEDPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);

  // This is bad practice, we should wire it to another pin but as this
  // is the task we bind led_isr to LEDPIN state change
  //
  // Binding it to the same pin will stop us from triggering the event when ever
  // we use PWM do tim the diode.
  attachInterrupt(
    digitalPinToInterrupt(LEDPIN), led_isr,
    CHANGE);

#ifdef LOOPBACK
  // Patch the issue, seems that this patch does not work.
  // I assumes that the kernel is not fast enough to keep up
  // with the scheduling demand here.
  attachInterrupt(
    digitalPinToInterrupt(ECHOPIN), led_isr,
    CHANGE);
#endif
#ifdef PWM
  analogWrite(LEDPIN, 125);
#endif
}

/// A simple mutex over a number
typedef struct Mutex {
  bool locked;
  unsigned long start;
} mutex;

mutex semaphore = {
  false, NULL
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
    Serial.print(" seconds\n");
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

  #ifdef PWM
    while (!Serial.available()) {
      yield();
    }
  #else
    digitalWrite(LEDPIN, HIGH);
    delay(ontime);
    digitalWrite(LEDPIN, LOW);
    delay(ontime);
  #endif


  bool condition = Serial.available();

  // Will loop forever until we get a return
  while (condition) {
    char byte = Serial.read();
    // ASCII for return
    if (byte == 10) {
      break;
    }
    if (byte < 48 || byte > 57) {
      Serial.print("Only supply numbers\n");
      num = -1;
      break;
    }

    num = num * 10 + (byte - 48);

    // Allways read until newline.
    while (!Serial.available()) {
      yield();
    }
  }

  #ifdef PWM
    if (num >= 0 && ontime != num && num <= 255) {
      Serial.print("Setting duty cycle to : ");
      Serial.print(num);
      Serial.print(" \n");
      analogWrite(LEDPIN, num);
      ontime = num;
    } else if (num >= 0 && ontime != num && num > 255) {
      Serial.print("Maximum duty cycle is 255\n");
    }
  #else
    if ((num != 0) && num > 10) {
      ontime = num;
    } else if (num != 0 && num <= 10) {
      Serial.print("On time has be be larger than 10 uS\n");
    }
  #endif
}