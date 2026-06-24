#include <Servo.h>
#include <Adafruit_DotStar.h>

// ---- Pin definitions ----
// Button
#define BTN_PIN         A1

// Rotary Encoder 1 (Servo)
#define ENC1_A        10
#define ENC1_DT         12

// Rotary Encoder 2 (Stepper)
#define ENC2_A        4
#define ENC2_DT         6

// Servo
#define SERVO_PIN       A4

// DC Motor (to relay)
#define DC_PIN          9

// Stepper
#define STEP_PIN        3
#define DIR_PIN         2
#define LIM_PIN         7

// LED strip
#define NUM_LEDS 30
#define B_PIN 11
#define A_PIN 13

// Constants
#define SERVO_MIN_DEG     42
#define SERVO_MAX_DEG     80
#define DC_ON_MS          1100
#define STEPPER_DELAY_US  200   // (2x / step)
#define STEP_MAX          3500
#define SERVO_UPDATE_MS   20
#define LED_UPDATE_MS     1000

// Encoder definition
typedef struct ENCODER {
  char A_pin;
  char B_pin;
  char currentState;
  char prevState;
  long pos;
} ENCODER;

void initEncoder(ENCODER &enc, int A_pin, int B_pin, int start_pos) {
  enc.A_pin = A_pin;
  enc.B_pin = B_pin;
  pinMode(A_pin, INPUT_PULLUP);
  pinMode(B_pin, INPUT_PULLUP);
  enc.currentState = (digitalRead(A_pin) << 1) | digitalRead(B_pin);
  enc.prevState = enc.currentState;
  enc.pos = start_pos;
}

int readEncoder(ENCODER &enc, int step) {
  int direction = 0;

  enc.currentState = (digitalRead(enc.A_pin) << 1) | digitalRead(enc.B_pin);
  char change = enc.currentState ^ enc.prevState;

  if ((change >> 1) ^ (change & 0x01)) {
    direction = (((((enc.currentState >> 1) ^ enc.prevState) & 0x01) << 1) - 1) * step;
  }

  enc.prevState = enc.currentState;
  enc.pos += direction;
  return direction;
}

Servo myServo;
Adafruit_DotStar strip(NUM_LEDS, B_PIN, A_PIN, DOTSTAR_BGR);

// Servo encoder
ENCODER servoEncoder;
int  servoPos = SERVO_MIN_DEG;

// Stepper encoder
ENCODER stepperEncoder;
long stepCount = 0;
long requiredSteps = 0;
// DC Motor
unsigned long dcStartTime = 0;

// Move 1 step
void stepMotor(int direction) {
  digitalWrite(DIR_PIN, direction > 0 ? HIGH : LOW);
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(STEPPER_DELAY_US);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(STEPPER_DELAY_US);
}

// ---- Initialization ----
void setup() {
  Serial.begin(2000000);

  // DC motor
  pinMode(BTN_PIN, INPUT);
  pinMode(DC_PIN, OUTPUT);
  digitalWrite(DC_PIN, LOW);

  // Stepper motor
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN,  OUTPUT);
  pinMode(LIM_PIN, INPUT_PULLUP);
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN,  LOW);

  // Servo
  myServo.attach(SERVO_PIN);

  // Rotary Encoders
  initEncoder(servoEncoder, ENC1_A, ENC1_DT, SERVO_MIN_DEG);
  initEncoder(stepperEncoder, ENC2_A, ENC2_DT, 0);

  // LED strip
  strip.begin();

  // Move to limit switch
  while (digitalRead(LIM_PIN)) {
    stepMotor(1);
  }

  stepCount = 0;

  Serial.println("Setup Complete");
}

// ---- Main loop ----
void loop() {
  // ---- Periodic tasks ----
  if (millis() % SERVO_UPDATE_MS == 0) myServo.write(servoEncoder.pos); // Update servo
  if (millis() % LED_UPDATE_MS == 0) {  // Update LED strip
    strip.fill(0xFFFFFF); // Colour could be changed/tied to a function
    strip.show();
  }

  // Motor button
  if (digitalRead(BTN_PIN) && !dcStartTime) {
    dcStartTime = millis();
  }

  if (dcStartTime + DC_ON_MS > millis() && dcStartTime > 0) {
    digitalWrite(DC_PIN, HIGH);
  } else if (dcStartTime) {
    digitalWrite(DC_PIN, LOW);
    dcStartTime = 0;
    servoEncoder.pos = SERVO_MIN_DEG;
  }

  // Encoders
  readEncoder(servoEncoder, -2);
  servoEncoder.pos = constrain(servoEncoder.pos, SERVO_MIN_DEG, SERVO_MAX_DEG);

  requiredSteps += readEncoder(stepperEncoder, 64);
  
  // Stepper motor
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
    }
    else {
      stepMotor(1);
      stepCount--;
      requiredSteps++;
    }
  }
}
