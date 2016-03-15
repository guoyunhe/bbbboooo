
#include <Wire.h>
#include <SparkFun_APDS9960.h>

// Pins
#define APDS9960_INT    2 // Needs to be an interrupt pin

// Constants
#define OFF2 'A'
#define ONN  'B'
#define SWITCH_DEVICE 'C'
#define NEXT 'D'
#define PREVIOUS 'E'
#define VOLUME_UP 'F'
#define VOLUME_DOWN 'G'
#define PLAY 'H'
#define PAUSE 'I'

#define STATE_OFF     0
#define STATE_ACTIVE  1
#define STATE_SLEEP   2

#define TAP_STATE_IDLE 0
#define TAP_STATE_READY 1
#define TAP_STATE_LONG_TAP_DONE 2
#define TAP_STATE_TAP_DONE 3
#define TAP_STATE_NEXT_READY 4
#define TAP_STATE_TAP_TAP_DONE 5

// Global Variables
SparkFun_APDS9960 apds = SparkFun_APDS9960();
int isr_flag = 0;

long read_timer = 0;
long press_timer = 0;
long release_timer = 0;
long last_press_timer = 0;
long last_release_timer = 0;
boolean pressed = false;
boolean long_press_send = false;
int pressure1 = 0;
int pressure2 = 0;

int state = STATE_OFF;

void setup() {
  // Initialize Serial port
  Serial.begin(9600);

  // Initialize interrupt service routine
  attachInterrupt(0, interruptRoutine, FALLING);

  // Initialize APDS-9960 (configure I2C and initial values)
  apds.init();
  apds.setGestureGain(GGAIN_1X);
  apds.enableGestureSensor(true);

  Serial.println("hello");
}

void loop() {
  if ( isr_flag == 1 ) {
    detachInterrupt(0);
    handleGesture();
    isr_flag = 0;
    attachInterrupt(0, interruptRoutine, FALLING);
  }

  if (millis() - read_timer > 20) {
    pressure2 = pressure1;
    pressure1 = analogRead(A0);
    read_timer = millis();
  }

  if (pressed && pressure1 < 30 && pressure2 < 30) {
    // Release
    pressed = false;
    long_press_send = false;
    last_release_timer = release_timer;
    release_timer = millis();

    if (release_timer - last_release_timer < 500) {
      if (state != STATE_OFF) {
        Serial.write(SWITCH_DEVICE);
      }

    }
  } else if (!pressed && pressure1 >= 30 && pressure2 >= 30) {
    // Press
    pressed = true;
    last_press_timer = press_timer;
    press_timer = millis();
  }

  // Long Press
  if (!long_press_send && pressed && millis() - press_timer > 800) {
    long_press_send = true;
    if (state == STATE_OFF) {
      Serial.write(ONN);
      state = STATE_ACTIVE;
    } else {
      Serial.write(OFF2);
      state = STATE_OFF;
    }
  }

}

void interruptRoutine() {
  isr_flag = 1;
}

void handleGesture() {
  if ( apds.isGestureAvailable() ) {
    char cmd = ' ';
    switch ( apds.readGesture() ) {
      case DIR_UP:
        cmd = VOLUME_UP;
        break;
      case DIR_DOWN:
        cmd = VOLUME_DOWN;
        break;
      case DIR_LEFT:
        cmd = PREVIOUS;
        break;
      case DIR_RIGHT:
        cmd = NEXT;
        break;
      case DIR_NEAR:
        cmd = PAUSE;
        break;
      case DIR_FAR:
        cmd = PLAY;
        break;
    }
    if (state == STATE_ACTIVE) {
      Serial.write(cmd);
    }
  }
}
