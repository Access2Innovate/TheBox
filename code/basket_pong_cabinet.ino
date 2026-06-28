#include <Servo.h>
#include <Adafruit_DotStar.h>

// ---- Pin definitions ----
#define BTN_PIN         A1

#define ENC1_A          10
#define ENC1_DT         12

#define ENC2_A          4
#define ENC2_DT         6

#define SERVO_PIN       A4
#define DC_PIN          9

#define STEP_PIN        3
#define DIR_PIN         2
#define LIM_PIN         7

#define NUM_LEDS        30
#define B_PIN           11
#define A_PIN           13

// ---- Tuning ----
#define SERVO_MIN_DEG     42
#define SERVO_MAX_DEG     80
#define DC_ON_MS          500
#define STEPPER_DELAY_US  200
#define STEP_MAX          3500
#define SERVO_UPDATE_MS   20
#define LED_UPDATE_MS     1000
#define LED_COLOUR        0x6F6F6F

// Pretty bad way to prevent it from hanging forever but yk.
const unsigned long REBOOT_INTERVAL = 120UL * 60UL * 1000UL;

// ---- Encoder state ----
struct ENCODER {
  uint8_t A_pin;
  uint8_t B_pin;
  uint8_t currentState;
  uint8_t prevState;
  long pos;
};

// Arduino gets weird with auto-prototypes sometimes, so these stay here.
void rebootNow();
void initEncoder(ENCODER &enc, int A_pin, int B_pin, int start_pos);
int readEncoder(ENCODER &enc, int step);
void stepMotor(int direction);

Servo myServo;
Adafruit_DotStar strip(NUM_LEDS, B_PIN, A_PIN, DOTSTAR_BGR);

ENCODER servoEncoder;
ENCODER stepperEncoder;

long stepCount = 0;
long requiredSteps = 0;

unsigned long dcStartTime = 0;
unsigned long lastServoUpdate = 0;
unsigned long lastLedUpdate = 0;

// ---- Cursed soft reset ----
void rebootNow() {
  void (*resetFunc)(void) = 0;
  resetFunc();
}

// ---- Encoder setup ----
void initEncoder(ENCODER &enc, int A_pin, int B_pin, int start_pos) {
  enc.A_pin = A_pin;
  enc.B_pin = B_pin;

  pinMode(A_pin, INPUT_PULLUP);
  pinMode(B_pin, INPUT_PULLUP);

  enc.currentState = (digitalRead(A_pin) << 1) | digitalRead(B_pin);
  enc.prevState = enc.currentState;
  enc.pos = start_pos;
}

// ---- Encoder read ----
int readEncoder(ENCODER &enc, int step) {
  int direction = 0;

  enc.currentState = (digitalRead(enc.A_pin) << 1) | digitalRead(enc.B_pin);
  uint8_t change = enc.currentState ^ enc.prevState;

  // Works, don't touch unless it starts lying.
  if ((change >> 1) ^ (change & 0x01)) {
    direction = (((((enc.currentState >> 1) ^ enc.prevState) & 0x01) << 1) - 1) * step;
  }

  enc.prevState = enc.currentState;
  enc.pos += direction;

  return direction;
}

// ---- Move stepper 1 step ----
void stepMotor(int direction) {
  digitalWrite(DIR_PIN, direction > 0 ? HIGH : LOW);

  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(STEPPER_DELAY_US);

  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(STEPPER_DELAY_US);
}

void setup() {
  Serial.begin(2000000);

  // Basic outputs/inputs
  pinMode(BTN_PIN, INPUT);
  pinMode(DC_PIN, OUTPUT);
  digitalWrite(DC_PIN, LOW);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(LIM_PIN, INPUT_PULLUP);
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, LOW);

  // Servo starts in safe spot. On average this saves 5 hours a week.
  myServo.attach(SERVO_PIN);
  myServo.write(SERVO_MIN_DEG);

  initEncoder(servoEncoder, ENC1_A, ENC1_DT, SERVO_MIN_DEG);
  initEncoder(stepperEncoder, ENC2_A, ENC2_DT, 0);

  // LEDs are just white for now
  strip.begin();
  strip.fill(LED_COLOUR);
  strip.show();

  // Home stepper so we know where zero is
  long homingSteps = 0;
  const long HOMING_MAX_STEPS = STEP_MAX + 1000;

  while (digitalRead(LIM_PIN) && homingSteps < HOMING_MAX_STEPS) {
    stepMotor(1);
    homingSteps++;
  }

  // You can't stop me Keaton.
  if (homingSteps >= HOMING_MAX_STEPS) {
    Serial.println("ERROR: Homing failed. Limit switch not reached.");
    digitalWrite(DC_PIN, LOW);

    while (1) {
      delay(1000);
    }
  }

  stepCount = 0;
  requiredSteps = 0;

  Serial.println("Setup Complete");
}

void loop() {
  unsigned long now = millis();

  // Reboot after a while because this thing is haunted
  if (now >= REBOOT_INTERVAL) {
    Serial.println("Rebooting now...");
    delay(100);
    rebootNow();
  }

  // Periodic servo/LED updates
  if (now - lastServoUpdate >= SERVO_UPDATE_MS) {
    lastServoUpdate = now;
    myServo.write((int)servoEncoder.pos);
  }

  if (now - lastLedUpdate >= LED_UPDATE_MS) {
    lastLedUpdate = now;
    strip.fill(LED_COLOUR);
    strip.show();
  }

  // DC motor pulse
  if (digitalRead(BTN_PIN) && dcStartTime == 0) {
    dcStartTime = now;
  }

  if (dcStartTime > 0 && now - dcStartTime < DC_ON_MS) {
    digitalWrite(DC_PIN, HIGH);
  } else if (dcStartTime > 0) {
    digitalWrite(DC_PIN, LOW);
    dcStartTime = 0;
    servoEncoder.pos = SERVO_MIN_DEG;
  }

  // Servo encoder
  readEncoder(servoEncoder, -2);
  servoEncoder.pos = constrain(servoEncoder.pos, SERVO_MIN_DEG, SERVO_MAX_DEG);

  // Stepper encoder
  requiredSteps += readEncoder(stepperEncoder, 64);

  // Stop encoder noise from queueing infinite movement
  if (requiredSteps > STEP_MAX) {
    requiredSteps = STEP_MAX;
  } else if (requiredSteps < -STEP_MAX) {
    requiredSteps = -STEP_MAX;
  }

  // Stepper movement
  if (requiredSteps > 0) {
    if (stepCount >= STEP_MAX) {
      requiredSteps = 0;
    } else {
      stepMotor(-1);
      stepCount++;
      requiredSteps--;
    }
  } else if (requiredSteps < 0) {
    if (!digitalRead(LIM_PIN)) {
      stepCount = 0;
      requiredSteps = 0;
    } else {
      stepMotor(1);
      stepCount--;
      requiredSteps++;
    }
  }
}
