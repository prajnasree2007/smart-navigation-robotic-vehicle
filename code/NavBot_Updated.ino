#include <Servo.h>
#include <SoftwareSerial.h>

/* ---------- BLUETOOTH ---------- */
SoftwareSerial BT(12, 13); // RX, TX

/* ---------- ULTRASONIC ---------- */
#define trigPin 2
#define echoPin 3

/* ---------- SERVO ---------- */
#define servoPin 4
Servo headServo;

/* ---------- MOTOR DRIVER (L298N) ---------- */
#define ENA 5
#define ENB 6
#define IN1 8
#define IN2 9
#define IN3 10
#define IN4 11

/* ---------- DISTANCE CONSTANTS ---------- */
#define SAFE_DISTANCE 30
#define STOP_DISTANCE 20
#define DANGER_DISTANCE 8

char mode = 'A';            // A = Auto, M = Manual
bool emergencyStop = false;
char manualCmd = 'S';

/* ---------- FUNCTIONS ---------- */

long getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return 200;
  return duration * 0.034 / 2;
}

void stopRobot() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

/* ---------- SETUP ---------- */

void setup() {

  delay(2000);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // ENA & ENB jumpers ON → keep HIGH
  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);

  stopRobot();

  headServo.attach(servoPin);
  headServo.write(90);

  BT.begin(9600);
}

/* ---------- LOOP ---------- */

void loop() {

  /* ===== BLUETOOTH INPUT ===== */

  if (BT.available()) {
    char cmd = BT.read();

    // Convert lowercase to uppercase
    if (cmd >= 'a' && cmd <= 'z') {
      cmd -= 32;
    }

    if (cmd == 'E') {        // Emergency stop
      emergencyStop = true;
      stopRobot();
    }

    if (cmd == 'R') {        // Reset emergency
      emergencyStop = false;
      manualCmd = 'S';
    }

    if (cmd == 'A') mode = 'A';
    if (cmd == 'M') mode = 'M';

    if (mode == 'M') {
      if (cmd == 'F' || cmd == 'B' || cmd == 'L' || cmd == 'R' || cmd == 'S') {
        manualCmd = cmd;
      }
    }
  }

  /* ===== EMERGENCY STOP PRIORITY ===== */

  if (emergencyStop) {
    stopRobot();
    return;
  }

  long distance = getDistance();

  /* ===== CRITICAL SAFETY CHECK ===== */

  if (distance <= DANGER_DISTANCE) {
    emergencyStop = true;
    stopRobot();
    return;
  }

  /* ===== MODE LOGIC ===== */

  if (mode == 'A') {

    // ===== AUTO MODE =====

    if (distance > SAFE_DISTANCE) {
      moveForward();
    }
    else if (distance > STOP_DISTANCE) {
      moveForward();
    }
    else {

      stopRobot();
      delay(200);

      // Scan left
      headServo.write(150);
      delay(400);
      long leftDist = getDistance();

      // Scan right
      headServo.write(30);
      delay(400);
      long rightDist = getDistance();

      headServo.write(90);

      if (leftDist > rightDist && leftDist > STOP_DISTANCE) {
        turnLeft();
        delay(400);
      }
      else if (rightDist > STOP_DISTANCE) {
        turnRight();
        delay(400);
      }
      else {
        moveBackward();
        delay(300);
      }

      stopRobot();
    }
  }
  else {

    // ===== MANUAL MODE =====
    // (No obstacle restriction except danger level)

    if (manualCmd == 'F') {
      moveForward();
    }
    else if (manualCmd == 'B') {
      moveBackward();
    }
    else if (manualCmd == 'L') {
      turnLeft();
    }
    else if (manualCmd == 'R') {
      turnRight();
    }
    else {
      stopRobot();
    }
  }
}