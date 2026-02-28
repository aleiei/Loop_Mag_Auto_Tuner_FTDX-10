#include <Arduino.h>

void initSDCard();
void activateSDCard();
void playAlarmNOSD();

#include "Mag_Loop_Control_V4.cpp"
#include "sd_card.cpp"
#include "buzzer.cpp"

void RefreshServices() {
    pollRadio();
    refreshDisplay();
}

void setup() {
    playBeep();
    initDisplay();
    Serial.begin(9600);
    pinMode(sleeppin, OUTPUT);
    pinMode(resetpin, OUTPUT);
    pinMode(DIR, OUTPUT);
    pinMode(STEP, OUTPUT);
    pinMode(swrSensorPin, INPUT);
    pinMode(buttonJoystickPin, INPUT_PULLUP);
    pinMode(buttonAPin, INPUT_PULLUP);
    pinMode(buttonBPin, INPUT_PULLUP);
    pinMode(buttonCPin, INPUT_PULLUP);
    pinMode(switchPin, INPUT_PULLUP);
    pinMode(networkShieldCSPin, OUTPUT);
    digitalWrite(networkShieldCSPin, HIGH);
    CURRENT_MODE = WAITING;
    display.fillScreen(ILI9341_BLACK);
    initRadio();
    initSDCard();
    refreshDisplay();
}

void loop() {
    pollRadio();
    refreshTimer.check();
}
