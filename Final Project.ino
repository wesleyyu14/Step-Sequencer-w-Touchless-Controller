//Wesley Yu
//Digital Electronics with Steven Litt
//Final Project: Multichannel step sequencer with a touchless MIDI controller
//12-8-17


#include <Encoder.h>
#include "TM1637Display.h"

int tempo = 0;

int currentStep = 0;
unsigned long lastStepTime = 0;


int buttonPin[4] = { 6, 7, 8, 9 };
boolean lastButtonState[4] =  { LOW, LOW, LOW, LOW };
boolean buttonState[4] = { LOW, LOW, LOW, LOW };

boolean stepState[3][4] = {{ false, false, false, false }, { false, false, false, false }, { false, false, false, false }};

int ledPin[4] = { 2, 3, 4, 5 };
int countingLedPin[3] = { 23, 22, 21 };

int note[4] = { 60, 61, 62, 63 };

//Other buttonPins 
int forwardButtonPin = 11;
boolean forwardButtonState = LOW;
boolean lastForwardButtonState = LOW;

int backwardButtonPin = 10;
boolean backwardButtonState = LOW;
boolean lastBackwardButtonState = LOW;

int rotaryButtonPin = 32;
boolean rotaryButtonState = LOW;
boolean lastRotaryButtonState = LOW;

//add currentSequence for the multidimensional array
int currentSequence = 0; 

//resetLEDs
int hallTestPin = 30;
int resetPin = 31;

//hallEffectSensor integers
int hallEffect = 0;
int mappedHallEffect = 0;
int realMappedHallEffect = 0;

//Rotary Encoder Wheel Part
Encoder rotaryWheel(36, 37);

int newPosition = 0;
int oldPosition = 0;
int usefulVal = 0;
int realUsefulVal = 0;
int resetUsefulVal = 0;

//SevenDigitDisplay
int CLK = 34; 
int DIO = 35;
TM1637Display display(CLK, DIO);

void setup() {
  for (int i = 0; i < 4; i++) {
    pinMode(buttonPin[i], INPUT);
    pinMode(ledPin[i], OUTPUT);
  }
  for (int j = 0; j < 3; j++) {
    pinMode(countingLedPin[j], OUTPUT);
  }

  pinMode(forwardButtonPin, INPUT);
  pinMode(backwardButtonPin, INPUT);
  pinMode(rotaryButtonPin, INPUT);
  pinMode(resetPin, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(DIO, OUTPUT);

  Serial.begin(9600);

  display.setBrightness(7);
}

void loop() {
  sequence();
  setLeds();
  checkMIDIButtons();
  checkForwardButton();
  checkBackwardButton();
  magnetDetector();
  checkRotaryButton();
  checkRotaryWheel();
  somethingUseful();
}

void sequence() {
  tempo = analogRead(A5);

  if (millis() > lastStepTime + tempo) {   //if its time to go to the next step...

    currentStep = currentStep + 1;         //increment to the next step
    if (currentStep > 3) {
      currentStep = 0;
    }

    playNote(); //play note in sequence

    lastStepTime = millis();               //set lastSteStpTime to the current time
  }
}

void setLeds() {
  for (int i = 0; i < 4; i++) {
    if (i == currentStep) {
      analogWrite(ledPin[i], mappedHallEffect);
    } else if (stepState[currentSequence][i] == true) {
      analogWrite(ledPin[i], 2);
    } else {
      analogWrite(ledPin[i], 0);
    }
  }
}

void checkMIDIButtons() { // check the step sequencer buttons
  for (int i = 0; i < 4; i++) {
    lastButtonState[i] = buttonState[i];
    buttonState[i] = digitalRead(buttonPin[i]);

    if (buttonState[i] == HIGH && lastButtonState[i] == LOW) {
      if (stepState[currentSequence][i] == false) {
        stepState[currentSequence][i] = true;
      } else if (stepState[currentSequence][i] == true) {
        stepState[currentSequence][i] = false;
      }
    }
  }
}

void playNote() {
  for (int i = 0; i < 4; i++) {
    if ((i == currentStep) && (stepState[currentSequence][i] == true)) {
      usbMIDI.sendNoteOn(note[i] + (4 * currentSequence), 127, 1);
      //usbMIDI.sendNoteOn(int(60 + (2 * currentSequence)), 127, 1); //60 (C)+2*currentSequence which goes up by 1; a whole step.
    }
  }
}

void checkForwardButton() {
  lastForwardButtonState = forwardButtonState;
  forwardButtonState = digitalRead(forwardButtonPin);
  if (forwardButtonState == HIGH && lastForwardButtonState == LOW) {
    currentSequence++;
    if (currentSequence > 2) {
      currentSequence = 0;
    }
  }
}

void checkBackwardButton() {
  lastBackwardButtonState = backwardButtonState;
  backwardButtonState = digitalRead(backwardButtonPin);
  if (backwardButtonState == HIGH && lastBackwardButtonState == LOW) {
    currentSequence--;
    if (currentSequence < 0) {
      currentSequence = 2;
    }
  }
}

void magnetDetector() {
  hallEffect = analogRead(A19);
  Serial.println(hallEffect);
  mappedHallEffect = map(hallEffect, 400, 1023, 0, 127); //to fit the LED
  realMappedHallEffect = map(hallEffect, 0, 500, 0, 127); //to MIDI map
  usbMIDI.sendControlChange(1, realMappedHallEffect, 5);
  Serial.println(realMappedHallEffect);
}

void checkRotaryButton() {
  lastRotaryButtonState = rotaryButtonState;
  rotaryButtonState = digitalRead(rotaryButtonPin);
  if (rotaryButtonState == HIGH && lastRotaryButtonState == LOW) {
    digitalWrite(resetPin, HIGH);
    for (int i = 0; i < 4; i++) {
      stepState[currentSequence][i] = false;
    }
    delay(500);
    digitalWrite(resetPin, LOW);
  }
}

void checkRotaryWheel() {

  newPosition = rotaryWheel.read();
  //every time you click the thing forward or backwards one click
  //it does four switching actions... which annoyingly sets the position
  //forward or backwards by 4, not 1.  So you check for when its position
  //moves up or down by 4, and that's one click forward or backwards.
  if (newPosition == oldPosition + 4) {
    oldPosition = newPosition;
    usefulVal++;
  }

  else if (newPosition == oldPosition - 4) {
    oldPosition = newPosition;
    usefulVal--;
  }

  if (usefulVal > 3) {
    usefulVal = 0;
  }
  if (usefulVal < 0) {
    usefulVal = 3;
  }
}

void somethingUseful() {
  display.showNumberDec((usefulVal+1), false, 1, 0);
  display.showNumberDec((usefulVal+1), false, 1, 1);
  display.showNumberDec((usefulVal+1), false, 1, 2);
  display.showNumberDec((usefulVal+1), false, 1, 3);
  
  if (usefulVal == 0) {
    usbMIDI.sendControlChange(0, 0, 4);
    usbMIDI.sendControlChange(0, 0, 3);
    usbMIDI.sendControlChange(0, 0, 2);
    usbMIDI.sendControlChange(0, 127, 1);
    for (int j = 0; j < 3; j++) { //sets countingLeds
      if (j == currentSequence) {
        analogWrite(countingLedPin[j], 255);
      } else {
        analogWrite(countingLedPin[j], 0);
      }
    }
  }
  if (usefulVal == 1) {
    usbMIDI.sendControlChange(0, 0, 4);
    usbMIDI.sendControlChange(0, 0, 3);
    usbMIDI.sendControlChange(0, 0, 1);
    usbMIDI.sendControlChange(0, 127, 2);
    for (int j = 0; j < 3; j++) { //sets countingLeds
      if (j == currentSequence) {
        analogWrite(countingLedPin[j], 80);
      } else {
        analogWrite(countingLedPin[j], 0);
      }
    }
  }
  if (usefulVal == 2) {
    usbMIDI.sendControlChange(0, 0, 1);
    usbMIDI.sendControlChange(0, 0, 2);
    usbMIDI.sendControlChange(0, 0, 4);
    usbMIDI.sendControlChange(0, 127, 3);
    for (int j = 0; j < 3; j++) { //sets countingLeds
      if (j == currentSequence) {
        analogWrite(countingLedPin[j], mappedHallEffect);
      } else {
        analogWrite(countingLedPin[j], 0);
      }
    }
  }
  if (usefulVal == 3) {
    usbMIDI.sendControlChange(0, 0, 1);
    usbMIDI.sendControlChange(0, 0, 2);
    usbMIDI.sendControlChange(0, 0, 3);
    usbMIDI.sendControlChange(0, 127, 4);
    for (int j = 0; j < 3; j++) {
      if (j == currentSequence) {
        analogWrite(countingLedPin[j], 3);
      } else {
        analogWrite (countingLedPin[j], 0);
      }
    }
  }
}
