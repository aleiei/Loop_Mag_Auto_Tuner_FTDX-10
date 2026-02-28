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
#include <TimedAction.h>
#include <Stepper.h>
#include <SPI.h>
#include <SdFat.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h> // 2.8/3.2 inch ILI9341 TFT display (320x240)
#include <ArduinoJson.h>
//***************************************
//
//       Local Variables
//
//***************************************
#define CALLSIGN  "IU0PXK"
#define SCREENSAVER_MESSAGE  "IU0PXK Mag Loop Tuner"  // shown during screensaver
#define RADIO_MODEL  " Yaesu FTDX-10"
#define VERSION  "V1.00"

// IMPORTANT due to lack of memory and speed
// NETWORK NOT COMPATIBLE WITCH MEGA 2560 - USE ARDUINO DUE 32bit platform instead
//#define NETWORK 1
//#define CLOUD_SAVE 1
//#define DEBUG 1//#define RF_SENSOR
//*******************************
//
//       Radio & Antenna
//
//*******************************
// Radio protocol selection - uncomment ONE only
//#define ICOM        // Icom CI-V
#define YAESU         // Yaesu CAT (FTDX-10 / FT-991A)
//#define KENWOOD
//#define ELECCRAFT

// PTT_DTR_CTS: legacy Icom-only option, not used with Yaesu CAT
// #define PTT_DTR_CTS 1

//Band allowed by the antenna that match radio band
//#define ANTENNA_BAND_10M 1
//#define ANTENNA_BAND_12M 1
//#define ANTENNA_BAND_15M 1
//#define ANTENNA_BAND_17M 1
#define ANTENNA_BAND_20M 1
#define ANTENNA_BAND_30M 1
#define ANTENNA_BAND_40M 1
#define ANTENNA_BAND_60M 1
#define ANTENNA_BAND_80M 1
//#define ANTENNA_BAND_160M 1

// If cat serial communication protocol provides SWR use SWR_RADIO
// else use SWR_SENSOR for external swr sensor
#define SWR_SENSOR  1
//#define SWR_RADIO 1

//Did you use Ham Radio Deluxe with this setup? if True there's no need to poll radio status over serial cat protocol. HRD does it every seconds
//But if True, HRD need to be ready and connected to the radio for the tuner to operate properly.
//NB. A serial Port Sharing Software will be needed in this configuration. like this freeware in 32bits and shareware in 64bits link http://www.eterlogic.com/Downloads.html
//or shareware link http://www.serial-port-splitter.com/sps-case-studies/ham-radio.html
#define HRD_PRESENT true
//***************************************
//
//       Pins Definition
//
//***************************************
#define sdCardCSPin 4
#define networkShieldCSPin 10
#define swrSensorPin  A10 //SWR Sensor
//Joystick
#define xAxis  A12 // joystick X axis
#define yAxis A13 // joystick y axis
//Step Motor Driver - Limit Switch - Encoder
#define DIR 41
#define STEP 40
#define resetpin  38
#define sleeppin  39
//Optionnal Limit Switch
#define limitSwitch 24
//Optionnal Step Motor Encoder
#define encoderA 26
#define encoderB 28
//Buttons
#define switchPin 34 //menu button
#define buttonJoystickPin  42 //joystick button
#define buttonAPin  35 //A button
#define buttonBPin  36 //B button
#define buttonCPin  37 //C button
//ILI9341 TFT Display (hardware SPI on Mega: MOSI=51, SCK=52, MISO=50)
#define TFT_CS     48   // Chip Select
#define TFT_RST    47   // Reset  (or connect to Arduino RESET pin)
#define TFT_DC     46   // Data/Command
//Buzzer
#define buzzer 53
// DTR/CTS PTT pins - NOT used with Yaesu CAT (kept for hardware compatibility)
// #define DTR_PIN 20
// #define CTS_PIN 21

//*******************************
//
//         Network
//
//*******************************
#ifdef NETWORK
#include <Ethernet.h>
EthernetServer server(80); //server port
String readString;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //physical mac address
byte ip[] = { 192, 168, 50, 150 }; // IP address in LAN – need to change according to your Network address
byte gateway[] = { 192, 168, 50, 1 }; // internet access via router
byte subnet[] = { 255, 255, 255, 0 }; //subnet mask
#endif
#ifdef CLOUD_SAVE
char cloudServer[] = "asksensors.com";
const int port = 80;
const char* apiKeyIn = "........................";
// dummy data
int dumData = 100;    // set your data
// Initialize the Ethernet client library
EthernetClient cloudClient;
#endif
boolean isNetworkPresent = false;
boolean isWWWpresent = false;
//**************************************
//
//     Defining SD Card Reader
//
//**************************************
SdFat SD;
boolean sdCardPresent = false;
//*******************************************
//
//  Defining configs structure
//
//*******************************************
struct Config {
        boolean ISCALIB;
        uint32_t CURRENT_FRQ;
        uint32_t CURRENT_POSITION;
        int CURRENT_MODE;
        int CURRENT_BAND;
        uint32_t POS10M;
        uint32_t POS12M;
        uint32_t POS15M;
        uint32_t POS17M;
        uint32_t POS20M;
        uint32_t POS30M;
        uint32_t POS40M;
        uint32_t POS60M;
        uint32_t POS80M;
        uint32_t POS160M;
        boolean AUTO_TUNE;
        boolean AUTO_FOLLOW;
        int CURRENT_DIRECTION = 1;
        int PREVIOUS_DIRECTION = 1;
        uint32_t BACKSLASH = 25;
};
boolean configLoaded = false;
const char *filename = "/config.txt";  // <- We use .txt instead of .json cause it is not supported by SD library.
Config config;
//*************************************************************
//
//  Memory frequency capacitor stepper position definitions
//
//*************************************************************
//This the default frequency Memory for each band. The default is assigned to position 0 of each memory arrays.
//This also help with the calibrationFrequencies values to estimate capacitor stepper position when no saved memories is availlable
//When availlable we use International Beacon frequency as default memories
//10m,12m,15m,17m,20m,30m,40m,60m,80m,160m
uint32_t defaultMemories[10] = {28200000, 24930000, 21150000, 18110000, 14100000, 10125000, 7350000, 5403500, 3950000, 1990000};
//Number of memory slot per band
#define MEMORY_MAX_SIZE 25
//Memory Object structure
struct Memory {
        uint32_t FREQUENCY;
        uint32_t POSITION;
};
//Memory SD Card json files <-  We use .txt instead of .json cause it is not supported by SD library.
const char *memoryTeMFile = "/mem10.txt";
const char *memoryTwelMFile = "/mem12.txt";
const char *memoryFiMFile = "/mem15.txt";
const char *memorySeMFile = "/mem17.txt";
const char *memoryTwMFile = "/mem20.txt";
const char *memoryThMFile = "/mem30.txt";
const char *memoryFoMFile = "/mem40.txt";
const char *memorySiMFile = "/mem60.txt";
const char *memoryHeMFile = "/mem80.txt";
const char *memoryHuSiMFile = "/mem160.txt";
//Memory Array
#ifdef ANTENNA_BAND_10M
Memory memory10m[MEMORY_MAX_SIZE];
#else
Memory memory10m[0];
#endif
#ifdef ANTENNA_BAND_12M
Memory memory12m[MEMORY_MAX_SIZE];
#else
Memory memory12m[0];
#endif
#ifdef ANTENNA_BAND_15M
Memory memory15m[MEMORY_MAX_SIZE];
#else
Memory memory15m[0];
#endif
#ifdef ANTENNA_BAND_17M
Memory memory17m[MEMORY_MAX_SIZE];
#else
Memory memory17m[0];
#endif
#ifdef ANTENNA_BAND_20M
Memory memory20m[MEMORY_MAX_SIZE];
#else
Memory memory20m[0];
#endif
#ifdef ANTENNA_BAND_30M
Memory memory30m[MEMORY_MAX_SIZE];
#else
Memory memory30m[0];
#endif
#ifdef ANTENNA_BAND_40M
Memory memory40m[MEMORY_MAX_SIZE];
#else
Memory memory40m[0];
#endif
#ifdef ANTENNA_BAND_60M
Memory memory60m[MEMORY_MAX_SIZE];
#else
Memory memory60m[0];
#endif
#ifdef ANTENNA_BAND_80M
Memory memory80m[MEMORY_MAX_SIZE];
#else
Memory memory80m[0];
#endif
#ifdef ANTENNA_BAND_160M
Memory memory160m[MEMORY_MAX_SIZE];
#else
Memory memory160m[0];
#endif

int memory10Size = 0;
int memory12Size = 0;
int memory15Size = 0;
int memory17Size = 0;
int memory20Size = 0;
int memory30Size = 0;
int memory40Size = 0;
int memory60Size = 0;
int memory80Size = 0;
int memory160Size = 0;
boolean toSavedConf = false;
boolean confLocked = false;
int toSavedConfTimeoutCycle = 0;
int toSavedConfTimeout = 6;
boolean toSavedMem = false;
boolean memLocked = false;
int toSavedMemTimeoutCycle = 0;
int toSavedMemTimeout = 6;
uint32_t toTunedPosition = 0;
boolean toTunedPositionChange = false;
int toTunedPositionTimeoutCycle = 0;
int toTunedPositionTimeout = 3;
//***************************************
//
//           Autotuning
//
//***************************************
// Autotuning type definitions
#define CURRENT_POSITION_TYPE 1
#define FOLLOW_TYPE 2
#define AUTO_TYPE 3
boolean txmiting = false;
boolean toAutotuned = false;
int toAutotunedTimeoutCycle = 0;
int toAutotunedTimeout = 3;
const int swrWaitDelay = 10; //Time to wait before taking swr sensor readings in ms
const int swrWaitDelayFine = 100; //Time to wait before taking swr sensor readings in ms
const uint32_t searchSteps[10] = {0, 0, 0, 0, 10000, 8000,  7000, 6000, 20000, 0}; //Algorithm will search +/- searchSteps {10m,12m,15m,17m,20m,30m,40m,60m,80m,160m}
const uint32_t searchStepsFine[10] = {0, 0, 0, 0, 2000, 1700,  1500, 1200, 10000, 0}; //Algorithm will search +/- searchStepsFine {10m,12m,15m,17m,20m,30m,40m,60m,80m,160m}
const int swrAverageSample = 5; //Number of sample taken to evaluate swr sensor value
const uint32_t SEARCH_FACTOR[10] = {0, 0, 0, 0, 175, 150, 100, 100, 100, 0};//10m,12m ,15m,17m,20m,30m,40m,60m,80m,160m
const uint32_t SEARCH_FACTOR_FINE[10] = {0, 0, 0, 0, 25, 25, 25, 50, 50, 0};//10m,12m ,15m,17m,20m,30m,40m,60m,80m,160m

//When Very High-Q Antenna is hooked, big diference in tuned SWR from the center frequecy in RTTY, FM, AM or CW compare
//to the lower and upper frequency of side band mode. Those numbers represent the offset position required to have
//a correctly tuned antenna in the side band operation mode. {10m,12m,15m,17m,20m,30m,40m,60m,80m,160m}
const uint32_t LSB_POSITION_OFFSET[10] = {0, 0, 0, 0, 100, 150, 200, 250, 300, 0}; //10m,12m,15m,17m,20m,30m,40m,60m,80m,160m
const int32_t USB_POSITION_OFFSET[10] = {0, 0, 0, 0, -100, -150, -200, -250, -300, 0}; //10m,12m,15m,17m,20m,30m,40m,60m,80m,160m

const int tuningThreshold[10] = {15, 15, 15, 15, 15, 15, 15, 15, 18, 18}; //10m,12m,15m,17m,20m,30m,40m,60m,80m,160m
const int tuningThresholdFine[10] = {5, 5, 5, 5, 5, 5, 5, 5, 9, 9}; //10m,12m,15m,17m,20m,30m,40m,60m,80m,160m
uint32_t goalSearchSteps = 0;

//SWR threshold to not reach while transmit or tune
const int swrThreshod[10] = {50, 50, 50, 50, 50, 50, 50, 50, 50, 50}; //10m,12m,15m,17m,20m,30m,40m,60m,80m,160m
const int swrThreshodFine[10] = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5}; //10m,12m,15m,17m,20m,30m,40m,60m,80m,160m
boolean swrAlarm = true;

String STARTING_TUNING_MESSAGE = "Init Autotune     ";
String FINISH_TUNING_MESSAGE = "Completed        ";
String MIDPOINT_TUNING_MESSAGE = "Half Done...     ";
String WAITING_RADIO_TUNING_MESSAGE = "Wait Radio...     ";
String SCANNING_MESSAGE = "Scanning...       ";
String SWR_ERROR_TUNING_MESSAGE = "  High SWR       ";
String WAITING_RADIO_SAFE_POWER_MESSAGE = "Wait Radio   |Safe Power  ";
String WAITING_RADIO_MODE_MESSAGE = "Wait Radio   |RTTY MODE   ";
String WAITING_RADIO_TXMIT_MESSAGE = "Wait Radio   |Transmission";
String EMPTY_SECOND_LINE_MESSAGE = "            ";
//***************************************
//
//           Auto Calibration
//
//***************************************
String STARTING_CALIBRATION_MESSAGE = "Calibration      ";

//This is center frequency of each band 10m,12m,15m,17m,20m,30m,40m,60m,80m,160m
//(Those frequencies correspont to the stepper position of each band saved in the config file and variable. i.e config.POS20M, config.POS12M, ... )
// TODO Crete different values per country band plan allocation
uint32_t calibrationFrequencies[10] = {28800000, 24895000, 21225000, 18118000, 14175000, 10125000, 7150000, 5350000, 3750000, 1900000};
const int rfSearchSteps[10] = {5000, 5000, 5000, 5000, 5000, 5000,  5000, 5000, 5000, 5000}; //Algorithm will search + rfSearchSteps {10m,12m,15m,17m,20m,30m,40m,60m,80m,160m}
//***************************************
//
//    Menus and Texts Definition
//
//***************************************
const int HOME = 0;
const int COARSE_TUNE  = 1;
const int FINE_TUNE  = 2;
const int GOTOONE = 3;
const int GOTOTWO = 4;
const int GOTOTHREE = 5;
const int RESETONE = 6;
const int RESETTWO = 7;
const int RESETTHREE = 8;
const int WAITING = 9;
const int TXMITING = 10;
const int SEARCHING = 11;
const int TUNING = 12;
const int SETTINGS = 13;
const int RETURN_HOME = 14;
const int CALIBRATION = 15;
const char* MENU_STRING[] = {">Home", ">Coarse Tune", ">Fine Tune", ">GoTo Band 1", ">GoTo Band 2", ">GoTo Band 3", ">Reset Band 1", ">Reset Band 2", ">Reset Band 3", ">Waiting Radio ...", ">Transmitting", ">Searching Band", ">Tuning Frequency", ">Settings", ">Return Home", ">Calibration"};
int CURRENT_MODE = HOME;
int PREVIOUS_MODE = -1;

const int BAND_10M = 0;
const int BAND_12M = 1;
const int BAND_15M = 2;
const int BAND_17M = 3;
const int BAND_20M = 4;
const int BAND_30M  = 5;
const int BAND_40M  = 6;
const int BAND_60M = 7;
const int BAND_80M = 8;
const int BAND_160M = 9;

int CURRENT_BAND = BAND_20M;
int PREVIOUS_BAND = -1;
const int GRID_BOTTOM = 0;
const int GRID_TOP = 1;
const int STATUS_ERROR = -1;
const int OK = 1;
const int PROCESSING = 0;
const int WRONG_POSITION = 0;
int CONFIG_STATUS = STATUS_ERROR;
int MEM_STATUS = STATUS_ERROR;
int WWW_STATUS = STATUS_ERROR;
int TUNED_STATUS = STATUS_ERROR;
// Change defined color according to your buttons color
//https://forum.arduino.cc/index.php?topic=246399.0
// ILI9341 uses same 16-bit RGB565 colour format as ST7735.
// Standard colours are defined in Adafruit_ILI9341.h; add extras here.
#define ILI9341_GRAY    0x8410
#define DISABLED_COLOR  ILI9341_GRAY
#define JOY_BUTTON_COLOR ILI9341_WHITE
#define A_BUTTON_COLOR  ILI9341_YELLOW
#define B_BUTTON_COLOR  ILI9341_RED
#define C_BUTTON_COLOR  ILI9341_BLUE

//***************************************
//
//   Capacitor Step Motor Definition
//
//***************************************
// Uncomment USE_BACKSLASH if your mecanism has backlash in the direction change.
// Frequent when step motor is coupled with planetary gearbox. The backlash
// offset value is saved in the config file under the struct config.BACKSLASH and can be change via the network gui.

//#define USE_BACKSLASH 1

//#define LIMIT_SWITCH 1
//#define ENCODER 1

const int cw  = 1;
const int ccw = -1;
#define stepsPerRevolution 200 //Number of steps in one revolution of the motor
const uint32_t maxSteps = 650000; //Software Limit for Step Motor
int lowSpeed = 50;
int highSpeedCoarse = 255;
int ultraHighSpeed = 600;  //Maximum safe speed for your motor
int highSpeedFine = 125;
int highSpeed = highSpeedCoarse;
int currentSpeed = 0;
boolean gotoPositionLock = false; //Lock the gotoposition function when in use
// initialize the stepper library
Stepper capacitorStepper(stepsPerRevolution, DIR,  STEP);
//***************************************
//
//       Joystick Definition
//
//***************************************
// During operation if you change the max speed of the step motor,
// you need to reset those value by calling function refreshJoystickRange();
// So the range of the joystick will reflect the real speed range of the step motor.
int rawRange = (highSpeed * 2) - (lowSpeed * 2);             // output range of X or Y movement
int threshold = rawRange / 29;    // resting threshold
int range = rawRange + threshold;
int center = range / 2;
int lastSwitchState = HIGH;
//******************************************************
//
//             Radio structure Definition
//
//******************************************************
struct Radio {
        uint32_t oldFrequency;
        boolean freqSaved = false;
        int radioIsPresent = 0;
        uint16_t trx_pwr = 0; // Power Control Leve, typically 0 - 255, taken over and lowered while SWR tune
        uint8_t trx_byte_pwr[2] = {0x00,0x00};
        uint16_t trx_signal = 0;
        uint8_t trx_swr = 0;
        uint8_t trx_mode = 0; // LSB, USB, CW, AM etc... AM while SWR tune
        boolean txmit = false;
};
Radio radio;
uint8_t PREVIOUS_POWER[2] = {0x00,0x00};
uint8_t PREVIOUS_TRX_MODE = 0;
uint8_t prevTrxMode = 0;
//********************************************
//
//   Yaesu CAT Protocol – FTDX-10 / FT-991A
//
//   Physical connection (Mega 2560):
//     Radio CAT TX  --> Arduino Serial1 RX (pin 19)
//     Radio CAT RX  --> Arduino Serial1 TX (pin 18)
//     Radio GND     --> Arduino GND
//
//   FTDX-10 menu settings required:
//     CAT RATE  : 38400
//     CAT TOT   : 10ms (or OFF)
//     CAT RTS   : Disable
//
//********************************************
#ifdef YAESU

#define CAT_BAUD         38400     // Serial1 baud rate
#define CAT_TIMEOUT_MS   200       // Max ms to wait for a CAT response
#define SERIAL_DELAY     50        // ms pause between CAT commands

// ── CAT command strings ──────────────────────────────────────
#define CAT_CMD_GET_FREQ   "FA;"   // Read VFO-A frequency  → FA00014175000;
#define CAT_CMD_GET_INFO   "IF;"   // Full status (38 chars) → byte[28]='1' when TX
#define CAT_CMD_PTT_ON     "TX1;"  // Activate TX (CW / DATA carrier)
#define CAT_CMD_PTT_OFF    "TX0;"  // Return to RX
#define CAT_CMD_GET_MODE   "MD0;"  // Read operating mode
#define CAT_CMD_SET_MODE_AM  "MD09;" // Set mode AM (for safe-power tuning)
#define CAT_CMD_GET_PWR    "PC;"   // Read TX power level → PC000; to PC100;
// Format to set power: "PCxxx;" where xxx = 000-100
#define CAT_CMD_PWR_LOW    "PC010;" // 10% power for safe tuning

// ── Mode codes returned by MD0; response ─────────────────────
// MD0x; where x is one ASCII digit
#define YAESU_MODE_LSB   '1'
#define YAESU_MODE_USB   '2'
#define YAESU_MODE_CW    '3'
#define YAESU_MODE_FM    '4'
#define YAESU_MODE_AM    '5'
#define YAESU_MODE_RTTY  '6'
#define YAESU_MODE_CWR   '7'
#define YAESU_MODE_DATAL '9'   // DATA-L (LSB data)
#define YAESU_MODE_DATAU 'B'   // DATA-U (USB data)

// Map Yaesu mode codes to the same symbolic names used in the rest of the code
#define MODE_TYPE_LSB    1
#define MODE_TYPE_USB    2
#define MODE_TYPE_CW     3
#define MODE_TYPE_FM     4
#define MODE_TYPE_AM     5
#define MODE_TYPE_RTTY   6
#define MODE_TYPE_CWR    7
#define MODE_TYPE_RTTYR  8

#define TRANSMITTING  1
#define RECEIVING     0

const char* mode[] = {"", "LSB", "USB", "CW", "FM", "AM", "RTTY", "CWR", "RTTYR"};

// Safe TX power percentage for antenna tuning (0-100)
#define TUNING_SAFE_POWER_PCT  10   // 10% power while SWR-tuning

// Internal CAT state variables
uint16_t baud_rate       = CAT_BAUD;
uint32_t readtimeout     = CAT_TIMEOUT_MS;
char     cat_response[48];          // Receive buffer for CAT responses
uint16_t cat_saved_power = 100;     // Power level saved before tuning
uint8_t  cat_saved_mode  = MODE_TYPE_USB; // Mode saved before tuning

#endif  // YAESU
//********************************************
//
//      LCD TFT 128X160 Color Definition
//
//********************************************
// ILI9341 uses hardware SPI (MOSI=51, SCK=52 on Mega 2560)
Adafruit_ILI9341 display = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// ILI9341 display dimensions (landscape)
#define TFT_WIDTH  320
#define TFT_HEIGHT 240
// Layout zones (Y pixel positions)
#define TFT_HEADER_Y      0
#define TFT_HEADER_H     40
#define TFT_FREQ_Y       45
#define TFT_MODE_Y       90
#define TFT_STATUS_Y    130
#define TFT_SWR_Y       170
#define TFT_ICONS_Y     210
#define TFT_ICON_SIZE    16
void RefreshServices();// LCD display routine
void activateSDCard();
void playAlarmNOSD();
int searchingCycle = 0;
int refreshTimeout = 500;
int timeoutCycle = 0;
int returnToMenutimeout = 60; // * time refreshTime = duration in secomds ie. refresh timeout of (500ms) and returnToMenutimeout of (60 cycle) ==> 500ms * 60 = 30 seconds
int screenSaverCycle = 0;
int screenSaverTimeout = 240; // * time refreshScreenSaverTime = duration in secomds ie. refresh timeout of (500ms) and screenSaverTimeout of (240 cycle) ==> 500ms * 240 = 120 seconds
boolean screensaver = false;
TimedAction refreshTimer = TimedAction(refreshTimeout, RefreshServices);// mS update the LCD
//*********************************************
//
//      Load Configs from SD Card file
//
//*********************************************
void loadConfiguration(const char *filename, Config &config) {
        activateSDCard();
        // Open file for reading
        File file = SD.open(filename);
        // Allocate a temporary JsonDocument
        // Don't forget to change the capacity to match your requirements.
        // Use arduinojson.org/v6/assistant to compute the capacity.
        StaticJsonDocument<512> doc;
        // Deserialize the JSON document
        DeserializationError error = deserializeJson(doc, file);
        if (error) {
                configLoaded = false;
                CONFIG_STATUS = STATUS_ERROR;
                Serial.println(F("Failed to read file, using default configuration"));
        } else {
                configLoaded = true;
                CONFIG_STATUS = OK;
        }
        // Copy values from the JsonDocument to the Config
        config.CURRENT_POSITION = doc["CURRENT_POSITION"];
        config.CURRENT_FRQ = doc["CURRENT_FRQ"];
        config.CURRENT_MODE = doc["CURRENT_MODE"];
        config.CURRENT_BAND = doc["CURRENT_BAND"];
        config.CURRENT_DIRECTION = doc["CURRENT_DIRECTION"];
        config.PREVIOUS_DIRECTION = doc["PREVIOUS_DIRECTION"];
        config.BACKSLASH = doc["BACKSLASH"];
        config.ISCALIB = doc["ISCALIB"];
        config.POS10M = doc["POS10M"];
        config.POS12M = doc["POS12M"];
        config.POS15M = doc["POS15M"];
        config.POS17M = doc["POS17M"];
        config.POS20M = doc["POS20M"];
        config.POS30M = doc["POS30M"];
        config.POS40M = doc["POS40M"];
        config.POS60M = doc["POS60M"];
        config.POS80M = doc["POS80M"];
        config.POS160M = doc["POS160M"];
        config.AUTO_TUNE = doc["AUTO_TUNE"];
        config.AUTO_FOLLOW = doc["AUTO_FOLLOW"];

        // Close the file
        file.close();
}

//********************************************************************************************************************************************************
//
//   Save Configs (Do not use the function directly. Use the mutex and lock mecanism in startSaveConfig(); function instead to prevent file coruption)
//
//********************************************************************************************************************************************************
// Saves the configuration to a file
void saveConfiguration(const char *filename, Config &config) {
        activateSDCard();
        // Delete existing file, otherwise the configuration is appended to the file
        CONFIG_STATUS = PROCESSING;
        confLocked = true;
        if (!SD.remove(filename));

        // Open file for writing
        File file = SD.open(filename, FILE_WRITE);
        if (!file) {
                Serial.println(F("Failed to create file"));
                CONFIG_STATUS = STATUS_ERROR;
                return;
        }

        // Allocate a temporary JsonDocument
        // Don't forget to change the capacity to match your requirements.
        // Use arduinojson.org/assistant to compute the capacity.
        StaticJsonDocument<512> doc;

        // Set the values in the document
        doc["CURRENT_POSITION"] = config.CURRENT_POSITION;
        doc["CURRENT_FRQ"] = config.CURRENT_FRQ;
        doc["CURRENT_MODE"] = config.CURRENT_MODE;
        doc["CURRENT_BAND"] = config.CURRENT_BAND;
        doc["CURRENT_DIRECTION"] = config.CURRENT_DIRECTION;
        doc["PREVIOUS_DIRECTION"] = config.PREVIOUS_DIRECTION;
        doc["BACKSLASH"] = config.BACKSLASH;
        doc["ISCALIB"] = config.ISCALIB;
        doc["POS10M"] = config.POS10M;
        doc["POS12M"] = config.POS12M;
        doc["POS15M"] = config.POS15M;
        doc["POS17M"] = config.POS17M;
        doc["POS20M"] = config.POS20M;
        doc["POS30M"] = config.POS30M;
        doc["POS40M"] = config.POS40M;
        doc["POS60M"] = config.POS60M;
        doc["POS80M"] = config.POS80M;
        doc["POS160M"] = config.POS160M;
        doc["AUTO_TUNE"] = config.AUTO_TUNE;
        doc["AUTO_FOLLOW"] = config.AUTO_FOLLOW;

        // Serialize JSON to file
        if (serializeJson(doc, file) == 0) {
                Serial.println(F("Failed to write to file"));
                CONFIG_STATUS = STATUS_ERROR;
        } else {
                CONFIG_STATUS = OK;
        }
        // Close the file
        file.close();
        toSavedConf = false;
        confLocked = false;

}

//*********************************************
//
//         Load Memory from a SD Card file
//
//*********************************************
void loadMemory(int BAND_MEMORY) {
        activateSDCard();
        // Open file for reading
        MEM_STATUS = PROCESSING;
        File file;
        if (BAND_MEMORY == BAND_10M) {
                file = SD.open(memoryTeMFile);
        } else if (BAND_MEMORY == BAND_12M) {
                file = SD.open(memoryTwelMFile);
        } else if (BAND_MEMORY == BAND_15M) {
                file = SD.open(memoryFiMFile);
        } else if (BAND_MEMORY == BAND_17M) {
                file = SD.open(memorySeMFile);
        } else if (BAND_MEMORY == BAND_20M) {
                file = SD.open(memoryTwMFile);
        } else if (BAND_MEMORY == BAND_30M) {
                file = SD.open(memoryThMFile);
        } else if (BAND_MEMORY == BAND_40M) {
                file = SD.open(memoryFoMFile);
        } else if (BAND_MEMORY == BAND_60M) {
                file = SD.open(memorySiMFile);
        } else if (BAND_MEMORY == BAND_80M) {
                file = SD.open(memoryHeMFile);
        } else if (BAND_MEMORY == BAND_160M) {
                file = SD.open(memoryHuSiMFile);
        }
        // Allocate a temporary JsonDocument
        // Don't forget to change the capacity to match your requirements.
        // Use arduinojson.org/v6/assistant to compute the capacity.
        StaticJsonDocument<2512> doc;

        // Deserialize the JSON document
        DeserializationError error = deserializeJson(doc, file);
        if (error) {
                MEM_STATUS = STATUS_ERROR;
                Serial.println(F("Failed to read file, using default configuration"));
        } else {
                MEM_STATUS = OK;
        }
        // Copy values from the JsonDocument to the Memory
        if (BAND_MEMORY == BAND_10M) {
                memory10Size = doc["memorySize10m"];
                for (int x = 0; x < memory10Size; x++) {
                        if (x==0) {
                                memory10m[x].FREQUENCY = defaultMemories[BAND_10M];
                                memory10m[x].POSITION = doc["memory10m"][String(x)]["POSITION"];
                        } else {
                                memory10m[x].FREQUENCY = doc["memory10m"][String(x)]["FREQUENCY"];
                                memory10m[x].POSITION = doc["memory10m"][String(x)]["POSITION"];
                        }
                }
        } else if (BAND_MEMORY == BAND_12M) {
                memory12Size = doc["memorySize12m"];
                for (int x = 0; x < memory12Size; x++) {
                        if (x==0) {
                                memory12m[x].FREQUENCY = defaultMemories[BAND_12M];
                                memory12m[x].POSITION = doc["memory12m"][String(x)]["POSITION"];
                        } else {
                                memory12m[x].FREQUENCY = doc["memory12m"][String(x)]["FREQUENCY"];
                                memory12m[x].POSITION = doc["memory12m"][String(x)]["POSITION"];
                        }
                }
        } else if (BAND_MEMORY == BAND_15M) {
                memory15Size = doc["memorySize15m"];
                for (int x = 0; x < memory15Size; x++) {
                        if (x==0) {
                                memory15m[x].FREQUENCY = defaultMemories[BAND_15M];
                                memory15m[x].POSITION = doc["memory15m"][String(x)]["POSITION"];
                        } else {
                                memory15m[x].FREQUENCY = doc["memory15m"][String(x)]["FREQUENCY"];
                                memory15m[x].POSITION = doc["memory15m"][String(x)]["POSITION"];
                        }
                }
        } else if (BAND_MEMORY == BAND_17M) {
                memory17Size = doc["memorySize17m"];
                for (int x = 0; x < memory17Size; x++) {
                        if (x==0) {
                                memory17m[x].FREQUENCY = defaultMemories[BAND_17M];
                                memory17m[x].POSITION = doc["memory17m"][String(x)]["POSITION"];
                        } else {
                                memory17m[x].FREQUENCY = doc["memory17m"][String(x)]["FREQUENCY"];
                                memory17m[x].POSITION = doc["memory17m"][String(x)]["POSITION"];
                        }
                }
        } else if (BAND_MEMORY == BAND_20M) {
                memory20Size = doc["memorySize20m"];
                for (int x = 0; x < memory20Size; x++) {
                        if (x==0) {
                                memory20m[x].FREQUENCY = defaultMemories[BAND_20M];
                                memory20m[x].POSITION = doc["memory20m"][String(x)]["POSITION"];
                        } else {
                                memory20m[x].FREQUENCY = doc["memory20m"][String(x)]["FREQUENCY"];
                                memory20m[x].POSITION = doc["memory20m"][String(x)]["POSITION"];
                        }
                }
        } else if (BAND_MEMORY == BAND_30M) {
                memory30Size = doc["memorySize30m"];
                for (int x = 0; x < memory30Size; x++) {
                        if (x==0) {
                                memory30m[x].FREQUENCY = defaultMemories[BAND_30M];
                                memory30m[x].POSITION = doc["memory30m"][String(x)]["POSITION"];
                        } else {
                                memory30m[x].FREQUENCY = doc["memory30m"][String(x)]["FREQUENCY"];
                                memory30m[x].POSITION = doc["memory30m"][String(x)]["POSITION"];
                        }
                }
        } else if (BAND_MEMORY == BAND_40M) {
                memory40Size = doc["memorySize40m"];
                for (int x = 0; x < memory40Size; x++) {
                        if (x==0) {
                                memory40m[x].FREQUENCY = defaultMemories[BAND_40M];
                                memory40m[x].POSITION = doc["memory40m"][String(x)]["POSITION"];
                        } else {
                                memory40m[x].FREQUENCY = doc["memory40m"][String(x)]["FREQUENCY"];
                                memory40m[x].POSITION = doc["memory40m"][String(x)]["POSITION"];
                        }
                }
        } else if (BAND_MEMORY == BAND_60M) {
                memory60Size = doc["memorySize60m"];
                for (int x = 0; x < memory60Size; x++) {
                        if (x==0) {
                                memory60m[x].FREQUENCY = defaultMemories[BAND_60M];
                                memory60m[x].POSITION = doc["memory60m"][String(x)]["POSITION"];
                        } else {
                                memory60m[x].FREQUENCY = doc["memory60m"][String(x)]["FREQUENCY"];
                                memory60m[x].POSITION = doc["memory60m"][String(x)]["POSITION"];
                        }
                }
        } else if (BAND_MEMORY == BAND_80M) {
                memory80Size = doc["memorySize80m"];
                for (int x = 0; x < memory80Size; x++) {
                        if (x==0) {
                                memory80m[x].FREQUENCY = defaultMemories[BAND_80M];
                                memory80m[x].POSITION = doc["memory80m"][String(x)]["POSITION"];
                        } else {
                                memory80m[x].FREQUENCY = doc["memory80m"][String(x)]["FREQUENCY"];
                                memory80m[x].POSITION = doc["memory80m"][String(x)]["POSITION"];
                        }
                }
        } else if (BAND_MEMORY == BAND_160M) {
                memory160Size = doc["memorySize160m"];
                for (int x = 0; x < memory160Size; x++) {
                        if (x==0) {
                                memory160m[x].FREQUENCY = defaultMemories[BAND_160M];
                                memory160m[x].POSITION = doc["memory160m"][String(x)]["POSITION"];
                        } else {
                                memory160m[x].FREQUENCY = doc["memory160m"][String(x)]["FREQUENCY"];
                                memory160m[x].POSITION = doc["memory160m"][String(x)]["POSITION"];
                        }
                }
        }
        // Close the file
        file.close();
}
//*****************************************************************************************************************************************************
//
//   Save Memory (Do not use the function directly. Use the mutex and lock mecanism in startSaveMem(); function instead to prevent file corruption)
//
//*****************************************************************************************************************************************************
// Saves the Memory to a file
void saveMemory() {
        activateSDCard();
        memLocked = true;
        MEM_STATUS = PROCESSING;
        File file;
        // Delete existing file, otherwise the configuration is appended to the file
        if (CURRENT_BAND == BAND_10M) {
                if (!SD.remove(memoryTeMFile));
        } else if (CURRENT_BAND == BAND_12M) {
                if (!SD.remove(memoryTwelMFile));
        } else if (CURRENT_BAND == BAND_15M) {
                if (!SD.remove(memoryFiMFile));
        } else if (CURRENT_BAND == BAND_17M) {
                if (!SD.remove(memorySeMFile));
        } else if (CURRENT_BAND == BAND_20M) {
                if (!SD.remove(memoryTwMFile));
        } else if (CURRENT_BAND == BAND_30M) {
                if (!SD.remove(memoryThMFile));
        } else if (CURRENT_BAND == BAND_40M) {
                if (!SD.remove(memoryFoMFile));
        } else if (CURRENT_BAND == BAND_60M) {
                if (!SD.remove(memorySiMFile));
        } else if (CURRENT_BAND == BAND_80M) {
                if (!SD.remove(memoryHeMFile));
        } else if (CURRENT_BAND == BAND_160M) {
                if (!SD.remove(memoryHuSiMFile));
        }
        // Open file for writing
        if (CURRENT_BAND == BAND_10M) {
                file = SD.open(memoryTeMFile, FILE_WRITE);
        } else if (CURRENT_BAND == BAND_12M) {
                file = SD.open(memoryTwelMFile, FILE_WRITE);
        } else if (CURRENT_BAND == BAND_15M) {
                file = SD.open(memoryFiMFile, FILE_WRITE);
        } else if (CURRENT_BAND == BAND_17M) {
                file = SD.open(memorySeMFile, FILE_WRITE);
        } else if (CURRENT_BAND == BAND_20M) {
                file = SD.open(memoryTwMFile, FILE_WRITE);
        } else if (CURRENT_BAND == BAND_30M) {
                file = SD.open(memoryThMFile, FILE_WRITE);
        } else if (CURRENT_BAND == BAND_40M) {
                file = SD.open(memoryFoMFile, FILE_WRITE);
        } else if (CURRENT_BAND == BAND_60M) {
                file = SD.open(memorySiMFile, FILE_WRITE);
        } else if (CURRENT_BAND == BAND_80M) {
                file = SD.open(memoryHeMFile, FILE_WRITE);
        } else if (CURRENT_BAND == BAND_160M) {
                file = SD.open(memoryHuSiMFile, FILE_WRITE);
        }
        if (!file) {
                MEM_STATUS = STATUS_ERROR;
                playAlarmNOSD();
                Serial.println(F("Failed to create file"));
                memLocked = false;
                toSavedMem = false;
                return;
        }

        // Allocate a temporary JsonDocument
        // Don't forget to change the capacity to match your requirements.
        // Use arduinojson.org/assistant to compute the capacity.
        StaticJsonDocument<2512> doc;
        // Set the values in the document

        if (CURRENT_BAND == BAND_10M) {
                doc["memorySize10m"] = memory10Size;
                for (int x = 0; x < MEMORY_MAX_SIZE; x++) {
                        if (x==0) {
                                doc["memory10m"][String(x)]["FREQUENCY"] = defaultMemories[BAND_10M];
                                doc["memory10m"][String(x)]["POSITION"] = memory10m[x].POSITION;
                        } else {
                                doc["memory10m"][String(x)]["FREQUENCY"] = memory10m[x].FREQUENCY;
                                doc["memory10m"][String(x)]["POSITION"] = memory10m[x].POSITION;
                        }
                }
        } else if (CURRENT_BAND == BAND_12M) {
                doc["memorySize12m"] = memory12Size;
                for (int x = 0; x < MEMORY_MAX_SIZE; x++) {
                        if (x==0) {
                                doc["memory12m"][String(x)]["FREQUENCY"] = defaultMemories[BAND_12M];
                                doc["memory12m"][String(x)]["POSITION"] = memory12m[x].POSITION;
                        } else {
                                doc["memory12m"][String(x)]["FREQUENCY"] = memory12m[x].FREQUENCY;
                                doc["memory12m"][String(x)]["POSITION"] = memory12m[x].POSITION;
                        }
                }
        } else if (CURRENT_BAND == BAND_15M) {
                doc["memorySize15m"] = memory15Size;
                for (int x = 0; x < MEMORY_MAX_SIZE; x++) {
                        if (x==0) {
                                doc["memory15m"][String(x)]["FREQUENCY"] = defaultMemories[BAND_15M];
                                doc["memory15m"][String(x)]["POSITION"] = memory15m[x].POSITION;
                        } else {
                                doc["memory15m"][String(x)]["FREQUENCY"] = memory15m[x].FREQUENCY;
                                doc["memory15m"][String(x)]["POSITION"] = memory15m[x].POSITION;
                        }
                }
        } else if (CURRENT_BAND == BAND_17M) {
                doc["memorySize17m"] = memory17Size;
                for (int x = 0; x < MEMORY_MAX_SIZE; x++) {
                        if (x==0) {
                                doc["memory17m"][String(x)]["FREQUENCY"] = defaultMemories[BAND_17M];
                                doc["memory17m"][String(x)]["POSITION"] = memory17m[x].POSITION;
                        } else {
                                doc["memory17m"][String(x)]["FREQUENCY"] = memory17m[x].FREQUENCY;
                                doc["memory17m"][String(x)]["POSITION"] = memory17m[x].POSITION;
                        }
                }
        } else if (CURRENT_BAND == BAND_20M) {
                doc["memorySize20m"] = memory20Size;
                for (int x = 0; x < MEMORY_MAX_SIZE; x++) {
                        if (x==0) {
                                doc["memory20m"][String(x)]["FREQUENCY"]  = defaultMemories[BAND_20M];
                                doc["memory20m"][String(x)]["POSITION"] = memory20m[x].POSITION;
                        } else {
                                doc["memory20m"][String(x)]["FREQUENCY"] = memory20m[x].FREQUENCY;
                                doc["memory20m"][String(x)]["POSITION"] = memory20m[x].POSITION;
                        }
                }
        } else if (CURRENT_BAND == BAND_30M) {
                doc["memorySize30m"] = memory30Size;
                for (int x = 0; x < MEMORY_MAX_SIZE; x++) {
                        if (x==0) {
                                doc["memory30m"][String(x)]["FREQUENCY"] = defaultMemories[BAND_30M];
                                doc["memory30m"][String(x)]["POSITION"] = memory30m[x].POSITION;
                        } else {
                                doc["memory30m"][String(x)]["FREQUENCY"] = memory30m[x].FREQUENCY;
                                doc["memory30m"][String(x)]["POSITION"] = memory30m[x].POSITION;
                        }
                }
        } else if (CURRENT_BAND == BAND_40M) {
                doc["memorySize40m"] = memory40Size;
                for (int x = 0; x < MEMORY_MAX_SIZE; x++) {
                        if (x==0) {
                                doc["memory40m"][String(x)]["FREQUENCY"] = defaultMemories[BAND_40M];
                                doc["memory40m"][String(x)]["POSITION"] = memory40m[x].POSITION;
                        } else {
                                doc["memory40m"][String(x)]["FREQUENCY"] = memory40m[x].FREQUENCY;
                                doc["memory40m"][String(x)]["POSITION"] = memory40m[x].POSITION;
                        }
                }
        } else if (CURRENT_BAND == BAND_60M) {
                doc["memorySize60m"] = memory60Size;
                for (int x = 0; x < MEMORY_MAX_SIZE; x++) {
                        if (x==0) {
                                doc["memory60m"][String(x)]["FREQUENCY"] = defaultMemories[BAND_60M];
                                doc["memory60m"][String(x)]["POSITION"] = memory60m[x].POSITION;
                        } else {
                                doc["memory60m"][String(x)]["FREQUENCY"] = memory60m[x].FREQUENCY;
                                doc["memory60m"][String(x)]["POSITION"] = memory60m[x].POSITION;
                        }
                }
        } else if (CURRENT_BAND == BAND_80M) {
                doc["memorySize80m"] = memory80Size;
                for (int x = 0; x < MEMORY_MAX_SIZE; x++) {
                        if (x==0) {
                                doc["memory80m"][String(x)]["FREQUENCY"] = defaultMemories[BAND_80M];
                                doc["memory80m"][String(x)]["POSITION"] = memory80m[x].POSITION;
                        } else {
                                doc["memory80m"][String(x)]["FREQUENCY"] = memory80m[x].FREQUENCY;
                                doc["memory80m"][String(x)]["POSITION"] = memory80m[x].POSITION;
                        }
                }
        } else if (CURRENT_BAND == BAND_160M) {
                doc["memorySize160m"] = memory160Size;
                for (int x = 0; x < MEMORY_MAX_SIZE; x++) {
                        if (x==0) {
                                doc["memory160m"][String(x)]["FREQUENCY"] = defaultMemories[BAND_160M];
                                doc["memory160m"][String(x)]["POSITION"] = memory160m[x].POSITION;
                        } else {
                                doc["memory160m"][String(x)]["FREQUENCY"] = memory160m[x].FREQUENCY;
                                doc["memory160m"][String(x)]["POSITION"] = memory160m[x].POSITION;
                        }
                }
        }
        // Serialize JSON to file
        if (serializeJson(doc, file) == 0) {
                MEM_STATUS = STATUS_ERROR;
                playAlarmNOSD();
                Serial.println(F("Failed to write to file"));
                memLocked = false;
                toSavedMem = false;
        } else {
                MEM_STATUS = OK;
        }
        // Close the file
        file.close();
        toSavedMem = false;
        memLocked = false;
}
//**********************************
//
//        Icons Definition
//
//  Tool to generate bitmap icons
//    http://dotmatrixtool.com/
//          configs:
//16px by 16px, row major, big endian.
//
//**********************************
//  16x16 Disk Icon
static const uint8_t PROGMEM icon_disk[] =
{
        0xff, 0xf8, 0x88, 0x14, 0x88, 0xd2, 0x88, 0xd1, 0x88, 0x11, 0x8f, 0xf1, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0xbf, 0xfd, 0xa0, 0x05, 0xa0, 0x05, 0xa0, 0x05, 0xa0, 0x05, 0xff, 0xff
};

//  16x16 SD Card Icon
static const uint8_t PROGMEM icon_sd[] =
{
        0x3f, 0xe0, 0x20, 0x10, 0x20, 0x08, 0x20, 0x04, 0x20, 0x04, 0x20, 0x04, 0x20, 0x04, 0x20, 0x04, 0x20, 0x04, 0x20, 0x04, 0x20, 0x04, 0x2a, 0xa4, 0x2a, 0xa4, 0x2a, 0xa4, 0x2a, 0xa4, 0x3f, 0xfc
};

//  16x16 Calibrated Icon
static const uint8_t PROGMEM icon_cal[] =
{
        0x00, 0x00, 0x1f, 0x80, 0x1f, 0x80, 0x18, 0x00, 0x18, 0x01, 0x18, 0x03, 0x1f, 0x87, 0x1f, 0x8e, 0x80, 0x1c, 0xc0, 0x38, 0xe0, 0x70, 0x70, 0xe0, 0x39, 0xc0, 0x1f, 0x80, 0x0f, 0x00, 0x04, 0x00
};
//  16x16 tuned Icon
static const uint8_t PROGMEM icon_tuned[] =
{
        0x06, 0x60, 0x16, 0x68, 0x76, 0x6e, 0xc6, 0x63, 0x96, 0x69, 0xb6, 0x6d, 0xa7, 0xe5, 0xa3, 0xc5, 0xb1, 0x8d, 0x91, 0x89, 0xc1, 0x83, 0x71, 0x8e, 0x11, 0x88, 0x01, 0x80, 0x03, 0xc0, 0x01, 0x80
};
// 16 x 16 memory icon
static const uint8_t PROGMEM icon_mem[] =
{
        0x00, 0x00, 0x0f, 0xe0, 0x18, 0x30, 0x60, 0x0c, 0x40, 0x04, 0x60, 0x0c, 0x5c, 0x34, 0x47, 0xe4, 0x70, 0x1c, 0x4c, 0x64, 0x73, 0x9c, 0x4c, 0x64, 0x63, 0x8c, 0x18, 0x30, 0x07, 0xc0, 0x00, 0x00
};
// 16 x 16 www icon
static const uint8_t PROGMEM icon_www[] =
{
        0x07, 0xe0, 0x18, 0x18, 0x37, 0xec, 0x67, 0xe6, 0x41, 0x82, 0xc1, 0x83, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x41, 0x82, 0x67, 0xe6, 0x37, 0xec, 0x18, 0x18, 0x07, 0xe0, 0x00, 0x00
};
// 16 x 16 network icon
static const uint8_t PROGMEM icon_network[] =
{
        0x03, 0xc0, 0x02, 0x40, 0x02, 0x40, 0x03, 0xc0, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x7f, 0xfe, 0x61, 0x86, 0x61, 0x86, 0x61, 0x86, 0x61, 0x86, 0xf3, 0xcf, 0x92, 0x49, 0x92, 0x49, 0xf3, 0xcf
};
// 16 x 16 radio icon
static const uint8_t PROGMEM icon_radio[] =
{
        0x08, 0x10, 0x1c, 0x10, 0x2a, 0x10, 0x49, 0x10, 0x88, 0x90, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x09, 0x11, 0x08, 0x92, 0x08, 0x54, 0x08, 0x38, 0x08, 0x10
};
// 16 x 16 cloud icon
static const uint8_t PROGMEM icon_cloud[] =
{
        0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x02, 0x20, 0x1a, 0x10, 0x24, 0x1c, 0x20, 0x22, 0x60, 0x02, 0x90, 0x01, 0x80, 0x01, 0x80, 0x01, 0x40, 0x02, 0x20, 0x04, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00
};
// 32 x 16 auto tune icon
static const uint8_t PROGMEM icon_autotune[] =
{
        0x00, 0x01, 0x80, 0x0c, 0x00, 0x03, 0x00, 0x06, 0x0f, 0xc6, 0xbf, 0xeb, 0x1f, 0xe5, 0xa0, 0x2d, 0x18, 0x65, 0x10, 0x45, 0x18, 0x65, 0x08, 0x85, 0x1f, 0xe5, 0x05, 0x05, 0x1f, 0xe5, 0x82, 0x0d, 0x18, 0x66, 0x82, 0x0b, 0x18, 0x63, 0x02, 0x06, 0x18, 0x61, 0x82, 0x0c, 0x18, 0x60, 0x02, 0x00, 0x18, 0x60, 0x02, 0x00, 0x18, 0x60, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00
};
// 32 x 16 auto follow icon
static const uint8_t PROGMEM icon_autofollow[] =
{
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0, 0x01, 0xe0, 0x1f, 0xe0, 0x01, 0x10, 0x18, 0x60, 0x01, 0x08, 0x18, 0x63, 0xff, 0x04, 0x1f, 0xe2, 0x00, 0x02, 0x1f, 0xe2, 0x00, 0x01, 0x18, 0x62, 0x00, 0x01, 0x18, 0x62, 0x00, 0x02, 0x18, 0x63, 0xff, 0x04, 0x18, 0x60, 0x01, 0x08, 0x18, 0x60, 0x01, 0x10, 0x18, 0x60, 0x01, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

//=============================================================================
//
//   YAESU CAT PROTOCOL FUNCTIONS  (replaces Icom CI-V functions)
//   Target: Yaesu FTDX-10 / FT-991A  –  Serial1, 38400 baud, 8N1
//
//   Wiring (Arduino Mega 2560):
//     Pin 18 (TX1)  -->  Radio CAT RX
//     Pin 19 (RX1)  <--  Radio CAT TX
//     GND           ---  Radio GND
//
//   FTDX-10 menu:  CAT RATE=38400, CAT TOT=10ms, CAT RTS=Disable
//
//=============================================================================
#ifdef YAESU

// ---------------------------------------------------------------------------
//  Low-level helpers
// ---------------------------------------------------------------------------

/** Drain any stale bytes from the CAT receive buffer. */
void catFlush() {
    unsigned long t = millis();
    while (Serial1.available() && (millis() - t < 50))
        Serial1.read();
}

/**
 * Read CAT response bytes until ';' is received or timeout expires.
 * @param buf     Caller-supplied buffer.
 * @param bufLen  Size of buf (including null terminator).
 * @return Number of bytes stored (excluding null).
 */
int catReadResponse(char* buf, int bufLen) {
    int idx = 0;
    unsigned long start = millis();
    while (millis() - start < CAT_TIMEOUT_MS) {
        if (Serial1.available()) {
            char c = (char)Serial1.read();
            if (idx < bufLen - 1) buf[idx++] = c;
            if (c == ';') break;
        }
    }
    buf[idx] = '\0';
    return idx;
}

/**
 * Send a CAT command string and optionally wait for the response.
 * @param cmd        Null-terminated CAT command, e.g. "FA;"
 * @param respBuf    Buffer for the response; pass NULL to discard.
 * @param respLen    Size of respBuf.
 */
void catSend(const char* cmd, char* respBuf = nullptr, int respLen = 0) {
    catFlush();
    Serial1.print(cmd);
    delay(SERIAL_DELAY);
    if (respBuf && respLen > 0) {
        catReadResponse(respBuf, respLen);
    }
}

// ---------------------------------------------------------------------------
//  Initialisation
// ---------------------------------------------------------------------------

/**
 * Initialise Serial1 for Yaesu CAT and verify the radio is reachable.
 * Sets radio.radioIsPresent = 1 on success.
 */
void initRadio() {
    Serial1.begin(CAT_BAUD);
    delay(300);
    catFlush();

    // Send FA; and check for a valid 14-char response starting with "FA"
    char resp[20];
    catSend(CAT_CMD_GET_FREQ, resp, sizeof(resp));

    if (strlen(resp) >= 14 && resp[0] == 'F' && resp[1] == 'A') {
        radio.radioIsPresent = 1;
        Serial.println(F("Yaesu FTDX-10: found"));
    } else {
        radio.radioIsPresent = 0;
        Serial.println(F("Yaesu FTDX-10: NOT found – check wiring & baud"));
    }
}

// ---------------------------------------------------------------------------
//  Frequency
// ---------------------------------------------------------------------------

/**
 * Read VFO-A frequency via FA;
 * @return Frequency in Hz, or 0 on error.
 */
uint32_t catGetFrequency() {
    char resp[20];
    catSend(CAT_CMD_GET_FREQ, resp, sizeof(resp));
    // Response: "FAxxxxxxxx;" – FA + 11 digits + ;  = 14 chars
    if (strlen(resp) < 14 || resp[0] != 'F' || resp[1] != 'A') return 0;
    uint32_t freq = 0;
    for (int i = 2; i < 13; i++) {
        if (resp[i] < '0' || resp[i] > '9') return 0;
        freq = freq * 10UL + (resp[i] - '0');
    }
    return freq;
}

/**
 * Poll radio frequency, update radio.oldFrequency if changed.
 * Returns true when a new frequency has been read successfully.
 */
boolean getFrequency() {
    uint32_t f = catGetFrequency();
    if (f == 0) return false;
    if (f != radio.oldFrequency) {
        radio.oldFrequency = f;
        radio.freqSaved    = true;
    }
    return true;
}

// ---------------------------------------------------------------------------
//  TX / RX status
// ---------------------------------------------------------------------------

/**
 * Query TX status via IF;
 * The IF; response is 38 chars; byte [28] (0-based) is '1' when transmitting.
 * @return true if radio is currently transmitting.
 */
boolean catIsTX() {
    char resp[48];
    catSend(CAT_CMD_GET_INFO, resp, sizeof(resp));
    if (strlen(resp) < 29) return false;
    return (resp[28] == '1');
}

/**
 * Update radio.txmit by polling the radio.
 * Returns true when transmitting, false when in receive.
 */
boolean getTXStatus() {
    radio.txmit = catIsTX();
    txmiting    = radio.txmit;
    return radio.txmit;
}

// ---------------------------------------------------------------------------
//  PTT control
// ---------------------------------------------------------------------------

/**
 * Key (tx=true) or unkey (tx=false) the transmitter via CAT.
 * TX1; = transmit carrier (AM/CW), TX0; = return to RX.
 */
void setPTT(boolean tx) {
    catSend(tx ? CAT_CMD_PTT_ON : CAT_CMD_PTT_OFF);
    delay(50);
    radio.txmit = tx;
    txmiting    = tx;
}

// ---------------------------------------------------------------------------
//  Operating mode
// ---------------------------------------------------------------------------

/**
 * Read the current operating mode via MD0;
 * @return One of MODE_TYPE_* constants, or 0 on error.
 */
uint8_t catGetMode() {
    char resp[10];
    catSend(CAT_CMD_GET_MODE, resp, sizeof(resp));
    // Response: "MD0x;" where x = mode digit/letter
    if (strlen(resp) < 5 || resp[0] != 'M' || resp[1] != 'D') return 0;
    char mc = resp[3];
    switch (mc) {
        case YAESU_MODE_LSB:  return MODE_TYPE_LSB;
        case YAESU_MODE_USB:  return MODE_TYPE_USB;
        case YAESU_MODE_CW:   return MODE_TYPE_CW;
        case YAESU_MODE_CWR:  return MODE_TYPE_CWR;
        case YAESU_MODE_FM:   return MODE_TYPE_FM;
        case YAESU_MODE_AM:   return MODE_TYPE_AM;
        case YAESU_MODE_RTTY: return MODE_TYPE_RTTY;
        default:              return 0;
    }
}

/**
 * Poll and store the current radio mode into radio.trx_mode.
 */
void getMode() {
    radio.trx_mode = catGetMode();
}

/**
 * Set operating mode using the MD0x; command.
 * @param modeType One of MODE_TYPE_* constants.
 */
void setMode(uint8_t modeType) {
    char cmd[8];
    char mc;
    switch (modeType) {
        case MODE_TYPE_LSB:  mc = YAESU_MODE_LSB;  break;
        case MODE_TYPE_USB:  mc = YAESU_MODE_USB;  break;
        case MODE_TYPE_CW:   mc = YAESU_MODE_CW;   break;
        case MODE_TYPE_CWR:  mc = YAESU_MODE_CWR;  break;
        case MODE_TYPE_FM:   mc = YAESU_MODE_FM;   break;
        case MODE_TYPE_AM:   mc = YAESU_MODE_AM;   break;
        case MODE_TYPE_RTTY: mc = YAESU_MODE_RTTY; break;
        default:             return;
    }
    snprintf(cmd, sizeof(cmd), "MD0%c;", mc);
    catSend(cmd);
}

// ---------------------------------------------------------------------------
//  TX Power control
// ---------------------------------------------------------------------------

/**
 * Read the current TX power level (0–100) via PC;
 * @return Power percentage 0-100, or 0 on error.
 */
uint16_t catGetPower() {
    char resp[10];
    catSend(CAT_CMD_GET_PWR, resp, sizeof(resp));
    // Response: "PCxxx;" where xxx = 000..100
    if (strlen(resp) < 6 || resp[0] != 'P' || resp[1] != 'C') return 0;
    char numStr[4] = {resp[2], resp[3], resp[4], '\0'};
    return (uint16_t)atoi(numStr);
}

/**
 * Set TX power (0–100%) via PC; command.
 */
void catSetPower(uint16_t pct) {
    if (pct > 100) pct = 100;
    char cmd[8];
    snprintf(cmd, sizeof(cmd), "PC%03u;", pct);
    catSend(cmd);
    radio.trx_pwr = pct;
}

/**
 * Read and store current TX power into radio.trx_pwr.
 */
void getRFPower() {
    radio.trx_pwr = catGetPower();
}

/**
 * Lower TX power to safe tuning level and save the original.
 */
void setTuningSafePower() {
    cat_saved_power = catGetPower();
    catSetPower(TUNING_SAFE_POWER_PCT);
    Serial.print(F("Saved power: ")); Serial.print(cat_saved_power);
    Serial.println(F("% → set to 10%"));
}

/**
 * Restore TX power to the level saved before tuning.
 */
void restorePower() {
    catSetPower(cat_saved_power);
    Serial.print(F("Power restored to: ")); Serial.println(cat_saved_power);
}

// ---------------------------------------------------------------------------
//  SWR / S-Meter  (analog sensor – CAT SWR not available on FTDX-10 in CAT)
// ---------------------------------------------------------------------------
// The FTDX-10 does not expose SWR via CAT commands;
// use the external SWR sensor on swrSensorPin (A10) as in the original code.

/**
 * Read SWR from external analog sensor.
 * @return Raw ADC value 0-1023; scale as needed for your bridge.
 */
int getSWR() {
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += analogRead(swrSensorPin);
        delay(2);
    }
    return sum / 5;
}

// ---------------------------------------------------------------------------
//  High-level radio polling  (called from loop() via TimedAction)
// ---------------------------------------------------------------------------

/**
 * Poll the radio state (frequency, mode, TX status).
 * Call this from the TimedAction refresh callback.
 */
void pollRadio() {
    if (!radio.radioIsPresent) return;
    getFrequency();
    getMode();
    getTXStatus();
}

#endif // YAESU

//=============================================================================
//
//   ILI9341 DISPLAY FUNCTIONS  (replaces ST7735 display functions)
//
//   The ILI9341 is 320x240 pixels (landscape).
//   Layout zones are defined by TFT_HEADER_Y, TFT_FREQ_Y etc. above.
//
//   NOTE: The original ST7735 code used 128x160. Most display calls
//   (drawBitmap, print, fillRect, setTextColor, etc.) are API-compatible.
//   The main differences are:
//     1. init call: display.begin()  (ILI9341) vs display.initR()  (ST7735)
//     2. Background is filled with ILI9341_BLACK after begin().
//     3. Larger canvas allows bigger text sizes and wider fields.
//
//=============================================================================

/**
 * Initialise the ILI9341 display.
 * Call this from setup() INSTEAD of display.initR() / display.initB().
 */
void initDisplay() {
    display.begin();
    display.setRotation(1);                    // landscape, connector at right
    display.fillScreen(ILI9341_BLACK);
    display.setTextWrap(false);
}

/**
 * Draw the fixed header bar with callsign and version.
 */
void drawHeader() {
    display.fillRect(0, TFT_HEADER_Y, TFT_WIDTH, TFT_HEADER_H, ILI9341_NAVY);
    display.setTextColor(ILI9341_WHITE);
    display.setTextSize(2);
    display.setCursor(6, TFT_HEADER_Y + 8);
    display.print(CALLSIGN);
    display.print(F("  Mag Loop Tuner  "));
    display.print(VERSION);
}

/**
 * Display the current VFO frequency on the TFT.
 * @param freqHz  Frequency in Hz (e.g. 14175000).
 */
void displayFrequency(uint32_t freqHz) {
    display.fillRect(0, TFT_FREQ_Y, TFT_WIDTH, 38, ILI9341_BLACK);
    display.setTextColor(ILI9341_YELLOW);
    display.setTextSize(3);
    display.setCursor(8, TFT_FREQ_Y);

    uint32_t MHz  = freqHz / 1000000UL;
    uint32_t kHz  = (freqHz % 1000000UL) / 1000UL;
    uint32_t Hz   = freqHz % 1000UL;

    char buf[20];
    snprintf(buf, sizeof(buf), "%2lu.%03lu.%03lu Hz", MHz, kHz, Hz);
    display.print(buf);
}

/**
 * Display operating mode and radio model on the TFT.
 * @param modeIndex  One of MODE_TYPE_* constants.
 */
void displayMode(uint8_t modeIndex) {
    display.fillRect(0, TFT_MODE_Y, TFT_WIDTH, 35, ILI9341_BLACK);
    display.setTextColor(ILI9341_CYAN);
    display.setTextSize(2);
    display.setCursor(8, TFT_MODE_Y);
    if (modeIndex < 9)
        display.print(mode[modeIndex]);
    else
        display.print(F("---"));
    display.print(F("   "));
    display.print(RADIO_MODEL);
}

/**
 * Display a status / tuning message on line 1 and an optional line 2.
 * Mirrors the original two-line message display on ST7735.
 * @param msg1  First line text (up to ~26 chars at size 2).
 * @param msg2  Second line text (pass "" to clear).
 */
void displayStatus(const String& msg1, const String& msg2) {
    display.fillRect(0, TFT_STATUS_Y, TFT_WIDTH, 36, ILI9341_BLACK);
    display.setTextColor(ILI9341_GREEN);
    display.setTextSize(2);

    // Split msg1 / msg2 if combined with '|' as in the original code
    int splitPos = msg1.indexOf('|');
    if (splitPos >= 0) {
        display.setCursor(8, TFT_STATUS_Y);
        display.print(msg1.substring(0, splitPos));
        display.setCursor(8, TFT_STATUS_Y + 18);
        display.print(msg1.substring(splitPos + 1));
    } else {
        display.setCursor(8, TFT_STATUS_Y);
        display.print(msg1);
        if (msg2.length()) {
            display.setCursor(8, TFT_STATUS_Y + 18);
            display.print(msg2);
        }
    }
}

/**
 * Display the SWR bar graph and numeric value.
 * @param swrRaw  Raw ADC reading from getSWR() (0-1023).
 */
void displaySWR(int swrRaw) {
    display.fillRect(0, TFT_SWR_Y, TFT_WIDTH, 34, ILI9341_BLACK);
    display.setTextColor(ILI9341_WHITE);
    display.setTextSize(1);
    display.setCursor(8, TFT_SWR_Y);
    display.print(F("SWR:"));

    // Bar width proportional to SWR ADC reading
    int barWidth = map(swrRaw, 0, 1023, 0, TFT_WIDTH - 60);
    uint16_t barColor = (swrRaw < 200) ? ILI9341_GREEN :
                        (swrRaw < 500) ? ILI9341_YELLOW : ILI9341_RED;
    display.fillRect(40, TFT_SWR_Y + 2, barWidth, 12, barColor);
    display.fillRect(40 + barWidth, TFT_SWR_Y + 2, (TFT_WIDTH - 60) - barWidth, 12, ILI9341_DARKGREY);

    // Numeric value
    display.setCursor(TFT_WIDTH - 52, TFT_SWR_Y);
    display.setTextSize(2);
    float swrV = (float)swrRaw / 100.0f;
    char sv[8];
    dtostrf(swrV, 4, 1, sv);
    display.print(sv);
}

/**
 * Display the status icons row at the bottom of the screen.
 * Draws the same bitmap icons as the original ST7735 code.
 * Icons are defined as 16x16 PROGMEM bitmaps earlier in the file.
 */
void displayIcons() {
    int y = TFT_ICONS_Y;
    int x = 4;

    // SD card icon
    display.drawBitmap(x, y, icon_sd, 16, 16,
        sdCardPresent ? ILI9341_WHITE : ILI9341_DARKGREY);
    x += 24;

    // Config status icon (disk)
    display.drawBitmap(x, y, icon_disk, 16, 16,
        (CONFIG_STATUS == OK) ? ILI9341_WHITE : ILI9341_RED);
    x += 24;

    // Memory icon
    display.drawBitmap(x, y, icon_mem, 16, 16,
        (MEM_STATUS == OK) ? ILI9341_WHITE : ILI9341_DARKGREY);
    x += 24;

    // Radio icon (green = present, red = not found)
    display.drawBitmap(x, y, icon_radio, 16, 16,
        (radio.radioIsPresent) ? ILI9341_GREEN : ILI9341_RED);
    x += 24;

    // TX indicator
    if (radio.txmit) {
        display.fillRect(x, y, 32, 16, ILI9341_RED);
        display.setTextColor(ILI9341_WHITE);
        display.setTextSize(1);
        display.setCursor(x + 4, y + 4);
        display.print(F("TX"));
    } else {
        display.fillRect(x, y, 32, 16, ILI9341_DARKGREY);
        display.setTextColor(ILI9341_WHITE);
        display.setTextSize(1);
        display.setCursor(x + 4, y + 4);
        display.print(F("RX"));
    }
    x += 40;

    // Auto-tune icon
    if (config.AUTO_TUNE) {
        display.drawBitmap(x, y, icon_autotune, 32, 16, ILI9341_GREEN);
    }
    x += 40;

    // Auto-follow icon
    if (config.AUTO_FOLLOW) {
        display.drawBitmap(x, y, icon_autofollow, 32, 16, ILI9341_CYAN);
    }
}

/**
 * Full screen refresh.  Call this from RefreshServices() (TimedAction callback).
 * Replaces the ST7735 RefreshServices() body.
 */
void refreshDisplay() {
    drawHeader();
    displayFrequency(radio.oldFrequency);
    displayMode(radio.trx_mode);
    displayStatus(MENU_STRING[CURRENT_MODE], "");
    displaySWR(getSWR());
    displayIcons();
}

/**
 * Screensaver: fill screen and show scrolling message.
 * Call when screenSaverCycle >= screenSaverTimeout.
 */
void showScreenSaver() {
    static int ssX = 0;
    display.fillScreen(ILI9341_BLACK);
    display.setTextColor(ILI9341_GREEN);
    display.setTextSize(2);
    display.setCursor(ssX, TFT_HEIGHT / 2 - 8);
    display.print(SCREENSAVER_MESSAGE);
    ssX -= 4;
    if (ssX < -(int)(strlen(SCREENSAVER_MESSAGE) * 12))
        ssX = TFT_WIDTH;
}

//=============================================================================
//  NOTE FOR OTHER SKETCH TABS
//=============================================================================
//
//  In setup():
//    Replace:  display.initR(INITR_BLACKTAB);   (or initB)
//    With:     initDisplay();
//
//    Replace:  Serial1.begin(baudRates[x]);      (Icom auto-baud loop)
//    With:     initRadio();                       (Yaesu single-baud init)
//
//  In the TimedAction callback (RefreshServices):
//    Replace:  [all ST7735 drawing code]
//    With:     pollRadio(); refreshDisplay();
//
//  Replace all Icom CI-V function calls:
//    icomSendCommand(...)      →  catSend(...)
//    icomReadFrequency()       →  getFrequency()
//    icomReadMode()            →  getMode()
//    icomReadRFPower()         →  getRFPower()
//    icomSetRFPower(...)       →  catSetPower(pct)
//    icomSafePower()           →  setTuningSafePower()
//    icomRestorePower()        →  restorePower()
//    icomReadTXStatus()        →  getTXStatus()
//    icomSetPTT(true/false)    →  setPTT(true/false)
//    icomReadMode() / setMode  →  getMode() / setMode(MODE_TYPE_*)
//
//=============================================================================
