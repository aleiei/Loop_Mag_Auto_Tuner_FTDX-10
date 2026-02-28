/*
   Mag loop antenna auto tuner

        Created 28 February 2026
        Updated 28 February 2026
        by Alessandro Orlando - IU0PXK

   Attribution-NonCommercial 2.0 Generic (CC BY-NC 2.0) License
   https://creativecommons.org/licenses/by-nc/2.0/

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
   OR OTHER DEALINGS IN THE SOFTWARE.

   code and circuitry inspired from
   https://create.arduino.cc/editor/JMardling/67243a05-af85-4b7b-832c-b67a56f8b77c/preview
   www.ve1zac.com/Simple%20Loop%20Automatic%20Tuner.pdf
   https://sites.google.com/site/lofturj/to-automatically-tune-a-magnetic-loop-antenna
 */

#include <Arduino.h>

#ifndef ARDUINO
#include <stdint.h>
typedef bool boolean;
typedef const char* String;

inline int analogRead(int) { return 0; }
inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
inline int abs(int x) { return x < 0 ? -x : x; }

struct Config {
        int32_t CURRENT_POSITION;
        int CURRENT_DIRECTION;
        int PREVIOUS_DIRECTION;
};

extern int range;
extern int center;
extern int threshold;
extern int rawRange;
extern int highSpeed;
extern int lowSpeed;
extern int xAxis;
extern int CURRENT_MODE;
extern int currentSpeed;
extern int TUNED_STATUS;
extern const int HOME;
extern const int SETTINGS;
extern const int SEARCHING;
extern const int STATUS_ERROR;

struct StepperLike {
        void setSpeed(int);
        void step(int);
};

extern StepperLike capacitorStepper;
extern Config config;

void motorStart();
void motorStop();
void updateDisplay(uint32_t, String);
void checkMemories();
void resetScreenSaver();
void startSaveConfig();
#endif

//*********************************
//
//       Read JoyStick Axis
//
//*********************************
int readAxis(int thisAxis) {
        // read the analog input:
        int reading = analogRead(thisAxis);

        // map the reading from the analog input range to the output range:
        reading = map(reading, 0, 1023, 0, range);

        // if the output reading is outside from the rest position threshold, use it:
        int distance = reading - center;

        if (abs(distance) < threshold) {
                distance = 0;
        }
        // return the distance for this axis:
        return distance;
}
//*********************************
//
//     Refresh joystick range
//
//*********************************
void refreshJoystickRange() {
        rawRange = (highSpeed * 2) - (lowSpeed * 2);       // output range of X or Y movement
        threshold = rawRange / 29; // resting threshold
        range = rawRange + threshold;
        center = range / 2;
}
void checkJoystickTuning() {
        boolean toSaved = false;
        int tempMode = -1;
        if ((readAxis(xAxis) > threshold && CURRENT_MODE != HOME && CURRENT_MODE != SETTINGS) || (readAxis(xAxis) < -threshold && CURRENT_MODE != HOME && CURRENT_MODE != SETTINGS)) {
                motorStart();
                toSaved = true;
                tempMode = CURRENT_MODE;
                CURRENT_MODE = SEARCHING;
                updateDisplay(0,"");
        }

        // Joystick tuning
        while (readAxis(xAxis) > threshold && CURRENT_MODE != HOME && CURRENT_MODE != SETTINGS)
        {
                currentSpeed = readAxis(xAxis) + (lowSpeed - threshold);
                capacitorStepper.setSpeed(currentSpeed);
                int bSlash = 0;
#ifdef USE_BACKSLASH
                if (config.CURRENT_DIRECTION != 1) {

                        config.CURRENT_DIRECTION = -1;
                }
                if (config.CURRENT_DIRECTION != config.PREVIOUS_DIRECTION) {
                        //TODO Backslash
                        //config.BACKSLASH
                        //TODO Direction Change
                        config.PREVIOUS_DIRECTION = config.CURRENT_DIRECTION;
                }
#endif

                config.CURRENT_POSITION = config.CURRENT_POSITION + 1 + bSlash;
                capacitorStepper.step(1);
                TUNED_STATUS = STATUS_ERROR;
                checkMemories();
                resetScreenSaver();
        }
        while (readAxis(xAxis) < -threshold && CURRENT_MODE != HOME && CURRENT_MODE != SETTINGS)
        {
                currentSpeed = -readAxis(xAxis) + (lowSpeed - threshold);
                capacitorStepper.setSpeed(currentSpeed);
                int bSlash = 0;
#ifdef USE_BACKSLASH

                if (config.CURRENT_DIRECTION != 1) {
                        config.CURRENT_DIRECTION = 1;
                }
                if (config.CURRENT_DIRECTION != config.PREVIOUS_DIRECTION) {
                        //TODO Backslash
                        //config.BACKSLASH
                        //TODO Direction Change
                        config.PREVIOUS_DIRECTION = config.CURRENT_DIRECTION;
                }
#endif


                config.CURRENT_POSITION = config.CURRENT_POSITION - 1 + bSlash;
                capacitorStepper.step(-1);
                TUNED_STATUS = STATUS_ERROR;
                checkMemories();
                resetScreenSaver();
        }
        motorStop();
        if (toSaved == true) {
                startSaveConfig();
                CURRENT_MODE = tempMode;
                updateDisplay(0,"");
        }
}
