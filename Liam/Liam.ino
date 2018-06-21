/*
 Liam - DIY Robot Lawn Mower

 ======================
  Licensed under GPLv3
 ======================
*/

/*
   Welcome to the Liam5_1 program
   This program will control your mower and relies on a two coil
   configuration (0 and 1) with an optional (2).

   The mower is assumed to be rear wheel driven and have three
   boundary wire reciever coils in the following configuration

    wheel
    ----------------
   |      (0) |
   |(2)           |  ----> Mowing direction
   |      (1) |
    ----------------
  wheel

  Most of the default values for your mower can be set in the
  Definition.h file. Use this to configure your mower for best
  performance.

  (c) Jonas Forssell & team
  Free to use for all.

  Changes in this version
  - Removed OzOLED Support for Arduino101 Compatibility
  - -----------------------------------------------------------
  - Ultrasound sensor bumper support            (Planned)
  - More robust shutdown of mower if wheel overload   (Planned)
  - Revised Error messages                (Planned)
  - Support for OLED Display                (Planned)
  - Signal sensitivity factor in Definition.h       (Planned)
  - Slower mowing if cutter motor is using much current (Planned)
  ---------------------------------------------------------------
*/

#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <I2Cdev.h>
#include "RTClib.h"
#include "HMC5883L.h"
#include "MPU9150.h"
#include "Battery.h"
#include "Wheelmotor.h"
#include "CutterMotor.h"
#include "BWFSensor.h"
#include "Controller.h"
#include "myLcd.h"
#include "Clock.h"
#include "Error.h"
#include "MotionSensor.h"
#include "Sens5883L.h"
#include "Sens9150.h"
#include "Definition.h"
#include "API.h"

#ifdef DEBUG_ENABLED
  #include "SetupDebug.h"
#endif
#if USE_MQTT
#include <ELClient.h>
#include <ELClientCmd.h>
#include <ELClientMqtt.h>
ELClient esp(&Serial,&Serial);
ELClientCmd cmd(&esp);
ELClientMqtt mqtt(&esp);
long publishTime = -1;
bool connected;
long last_mqtt;
#endif

// Global variables
int state;
int previousState = -1;
int command;

char commonBuffer[20];
long time_at_turning = millis();

bool sensorOutside[2];


// Set up all the defaults (check the Definition.h file for all default values)
DEFINITION Defaults;

// Please select which type of cutter motor you have
CUTTERMOTOR CutterMotor(CUTTER_MOTOR_TYPE, CUTTER_PWM_PIN, CUTTER_CURRENT_PIN);

// Wheelmotors
WHEELMOTOR rightMotor(WHEEL_MOTOR_A_PWM_PIN, WHEEL_MOTOR_A_DIRECTION_PIN, WHEEL_MOTOR_A_CURRENT_PIN, WHEELMOTOR_SMOOTHNESS);
WHEELMOTOR leftMotor(WHEEL_MOTOR_B_PWM_PIN, WHEEL_MOTOR_B_DIRECTION_PIN, WHEEL_MOTOR_B_CURRENT_PIN, WHEELMOTOR_SMOOTHNESS);

// Battery
BATTERY Battery(BATTERY_TYPE, BAT_PIN, DOCK_PIN);

// BWF Sensors
BWFSENSOR Sensor(BWF_SELECT_B_PIN, BWF_SELECT_A_PIN);

// Compass
#if defined __MS5883L__
MS5883L Compass;
#elif defined __MS9150__
MS9150 Compass;
#elif defined __ADXL345__
MS9150 Compass;
#else
MOTIONSENSOR Compass;
#endif

// Controller (pass adresses to the motors and sensors for the controller to operate on)
CONTROLLER Mower(&leftMotor, &rightMotor, &CutterMotor, &Sensor, &Compass);

#ifdef DEBUG_ENABLED
SETUPDEBUG SetupAndDebug(&Mower, &leftMotor, &rightMotor, &CutterMotor, &Sensor, &Compass, &Battery);
#endif

// Display
#if defined __LCD__
myLCD Display(&Battery, &leftMotor, &rightMotor, &CutterMotor, &Sensor, &Compass, &state);
#else
MYDISPLAY Display(&Battery, &leftMotor, &rightMotor, &CutterMotor, &Sensor, &Compass, &state);
#endif

// RTC clock
#if defined __RTC_CLOCK__
CLOCK Clock(GO_OUT_TIME, GO_HOME_TIME);
#endif

// Error handler
ERROR Error(&Display, LED_PIN, &Mower);

//  API

API api(&Battery,&Mower,&state);
// This function calls the sensor object every time there is a new signal pulse on pin2
void updateBWF() {
  Sensor.readSensor();
}

void setMowerState()
{
  switch (state)
  {
  case Cutter_states::MOWING:
    Mower.startCutter();
    Mower.runForward(FULLSPEED);
    break;
  case Cutter_states::IDLE:
    Mower.stopCutter();
    Mower.stop();
    break;
  case Cutter_states::LOOKING_FOR_BWF:
    Mower.stopCutter();
    break;
  default:
    break;
  }
}
// ****************** SETUP ******************************************
void setup() {

  // Fast communication on the serial port for all terminal messages
  Serial.begin(115200);

  // Configure all the pins for input or output
  Defaults.definePinsInputOutput();

  // Turn off the cutter motor as fast as possible
  CutterMotor.initialize();

  // Set default levels (defined in Definition.h) for your mower
  Defaults.setDefaultLevels(&Battery, &leftMotor, &rightMotor, &CutterMotor);

  // Start up the display
  Display.initialize();

  // Reset the battery voltage reading
  Battery.resetVoltage();
  Compass.initialize();

  // Run the updateBWF function every time there is a pulse on digital pin2
  attachInterrupt(0, updateBWF, RISING);
  Sensor.select(ORIENTATION::LEFT);

  // Print version information for five seconds before starting
  Display.clear();
  Display.print(F("--- LIAM ---\n"));
  Display.print(F(VERSION_STRING "\n"));
  Display.print(__DATE__ " " __TIME__ "\n");

  #ifdef DEBUG_ENABLED
  Serial.println(F("----------------"));
  Serial.println(F("Send D to enter setup and debug mode"));
  #endif
  delay(5000);
  #ifdef DEBUG_ENABLED
  state = SetupAndDebug.tryEnterSetupDebugMode(IDLE);
  #endif
  Display.clear();

  if (state != SETUP_DEBUG) {
    if (Battery.isBeingCharged()) {
      state = CHARGING;
      Mower.stopCutter();
    }
    else {
      state = MOWING;
    }
  }
#if USE_MQTT
    Serial.println(F("Laddar MQTT stöd."));
    mqtt_setup();
#endif
} // setup.

// TODO: This should probably be in Controller
void randomTurn(bool goBack) {
  if(goBack) {
    Mower.runBackward(FULLSPEED);
    delay(2000);
  }

  int angle = random(90, 160);
  if (random(0, 100) % 2 == 0) {
    Mower.turnRight(angle);
  } else {
    Mower.turnLeft(angle);
  }
  time_at_turning = millis();
  Compass.setNewTargetHeading();
  Mower.runForward(FULLSPEED);
}

// ***************** SAFETY CHECKS ***********************************
void checkIfFlipped() {
#if defined __MS9150__ || defined __MS5883L__ || __ADXL345__
  if (Mower.hasFlipped()) {
    Serial.print("Mower has flipped ");
    Mower.stopCutter();
    Mower.stop();
    Error.flag(ERROR_TILT);
  }
#endif
}

void checkIfLifted() {
#if defined __Lift_Sensor__
  if (Mower.isLifted()) {
    Serial.println("Mower is lifted");
    Mower.stopCutter();
    Mower.stop();
    Mower.runBackward(FULLSPEED);
    delay(2000);
    if(Mower.isLifted())
      Error.flag(ERROR_LIFT);
    Mower.randomTurn(false);
  }
#endif
}


// ***************** MOWING ******************************************
void doMowing() {
  if (Battery.mustCharge()) {
    state = LOOKING_FOR_BWF;
    return;
  }

  // Make regular turns to avoid getting stuck on things
  if((millis() - time_at_turning) > TURN_INTERVAL)
    randomTurn(true);

  // Check if any sensor is outside
  for(int i = 0; i < 2; i++) {
    // If sensor is inside, don't do anything
    if(!sensorOutside[i])
      continue;
    // ... otherwise ...

    char buf[40];
    #if USE_MQTT
    {
      sprintf_P(buf, PSTR("{\"message\":\"%s sensor outside\"}"),ORIENTATION_STRING[i]);
      UpdateJSONObject(MQTT_MESSAGE, buf);
    }
    #else
    {
      sprintf_P(buf, PSTR("%s sensor outside"), ORIENTATION_STRING[i]);
      Serial.println(buf);
    }
    #endif
    Sensor.select(i);
    Mower.stop();

    int err = Mower.GoBackwardUntilInside(&Sensor);
      if(err)
        Error.flag(err);

    // Try to turn away from BWF
    if(i == ORIENTATION::LEFT)
      err = Mower.turnToReleaseRight(30);
    else
      err = Mower.turnToReleaseLeft(30);

    if(err) {
      // If turning failed, reverse and try once more
      Mower.runBackward(FULLSPEED);
      delay(1000);
      Mower.stop();

      if(i == ORIENTATION::LEFT)
        err = Mower.turnToReleaseRight(30);
      else
        err = Mower.turnToReleaseLeft(30);

      if(err)
        Error.flag(err);
    }

    time_at_turning = millis();
    Compass.setNewTargetHeading();
  }

  // Avoid obstacles
  Mower.turnIfObstacle();

  // When mowing, the cutter should be on and we should be going forward
  Mower.startCutter();
  Mower.runForwardOverTime(SLOWSPEED, FULLSPEED, ACCELERATION_DURATION);

  // Adjust the speed of the mower to the grass thickness
  Mower.compensateSpeedToCutterLoad();

  // Adjust the speed of the mower to the compass heading
  Compass.updateHeading();
  Mower.compensateSpeedToCompassHeading();
}

// ***************** LAUNCHING ***************************************
void doLaunching() {
  // Back out of charger, turn and start mowing
  Mower.runBackward(FULLSPEED);
  delay(5000);
  Mower.stop();
  randomTurn(false);

  Battery.resetVoltage();

  state = MOWING;
}

// ***************** DOCKING *****************************************
void doDocking() {
  static int collisionCount = 0;
  static long lastCollision = 0;
  static long lastAllOutsideCheck = 0;
  static long lastOutside = 0;
  char buf[40];

  Mower.stopCutter();

  if(sensorOutside[0])
    lastOutside = millis();

  if(Battery.isBeingCharged()) {
    Mower.stop();
    state = CHARGING;
    return;
  }

  // If the mower hits something along the BWF
  if(Mower.wheelsAreOverloaded()) {
    if(millis() - lastCollision > 10000)
      collisionCount = 0;
    collisionCount++;
    lastCollision = millis();
    #if USE_MQTT
    sprintf_P(buf, PSTR("%S%i") , "Collision while docking: ",collisionCount);
    UpdateJSONObject(MQTT_MESSAGE,buf);

    #else
    Serial.print(F("Collision while docking: "));
    Serial.println(collisionCount);
#endif
    // Let it run for a bit and check if we hit the charger
    delay(1000);
    if(Battery.isBeingCharged()) {
      Mower.stop();
      state = CHARGING;
      return;
    }

    // Go back a bit and try again
    Mower.runBackward(FULLSPEED);
    delay(1300);

    // After third try. Try to go around obstacle
    if(collisionCount == 3) {
      Mower.turnRight(70);
      Mower.stop();
      collisionCount = 0;
      lastOutside = millis();
      Mower.runForward(FULLSPEED);
      return;
    }
  }

  // Check regularly if right sensor is outside
  if(millis() - lastAllOutsideCheck > 500) {
    if(sensorOutside[ORIENTATION::RIGHT]) {
      Mower.stop();
      Mower.runBackward(FULLSPEED);
      delay(700);
      Mower.stop();
      Mower.turnRight(20);
      Mower.stop();
      Mower.runForward(FULLSPEED);
    }
    lastAllOutsideCheck = millis();
  }

  // If left sensor has been inside fence for a long time
  if(millis() - lastOutside > 10000) {
    Mower.stop();
    Mower.turnLeft(30);
    state = LOOKING_FOR_BWF;
    return;
  }

  // Track the BWF by compensating the wheel motor speeds
  Sensor.select(ORIENTATION::LEFT);
  Mower.adjustMotorSpeeds();
} // DOCKING

void doLookForBWF() {
  Mower.stopCutter();

  // If sensor is outside, then the BWF has been found
  if(sensorOutside[ORIENTATION::LEFT]) {
    state = DOCKING;
    return;
  }

  // Make regular turns to avoid getting stuck on things
  if((millis() - time_at_turning) > TURN_INTERVAL)
    randomTurn(true);

  Mower.runForwardOverTime(SLOWSPEED, FULLSPEED, ACCELERATION_DURATION);
  Mower.turnIfObstacle();
}

// ***************** CHARGING ****************************************
void doCharging() {
  static long lastContact = 0;
  Mower.stop();
  Mower.stopCutter();

  if(Battery.isBeingCharged()) {
    lastContact = millis();
  }
  // If not charging for a long time, try to re-dock
  if(millis() - lastContact > 20000) {
    Mower.runBackward(SLOWSPEED);
    delay(500);
    Mower.runForward(SLOWSPEED);
    delay(2000);
    Mower.stop();
    // TODO: This resets the timer whether contact was made or not
    // The number of contact attempts should be counted, and some different action
    // should be taken when a certain number of unsuccessfull attempts has been made
    lastContact = millis();
  }

  if(Battery.isFullyCharged()
#if defined __RTC__CLOCK__
    && Clock.timeToCut()
#endif
    ) {
    // Don't launch if no BWF signal is present
    if(Sensor.isInside() || Sensor.isOutside()) {
      state = LAUNCHING;
      return;
    }
  }
}



// ***************** MAIN LOOP ***************************************
void loop() {
  #if(USE_MQTT)
  esp.Process();
  #else
  #ifdef DEBUG_ENABLED
    if((state = SetupAndDebug.tryEnterSetupDebugMode(state)) == SETUP_DEBUG)
      return;
  #endif
  #endif

  static long lastDisplayUpdate = 0;

  long looptime = millis();
  
  if(state != previousState)
  {
    char buf[40] ={'\0'};
    sprintf_P(buf, PSTR("{\"state\":%i,\"name\":\"%s\"}"),state,Cutter_states_STRING[state]);
    UpdateJSONObject(MQTT_STATE ,buf);
    previousState = state;
    setMowerState();
  }

  Battery.updateVoltage();

  // Check state of all sensors
  for(int i = 0; i < 2; i++) {
    Sensor.select(i);
    sensorOutside[i] = Sensor.isOutOfBounds();
  }

  // Safety checks
  checkIfFlipped();
  checkIfLifted();

  switch(state) {
    case IDLE:
      delay(100);
      break;
    case MOWING:
      doMowing();
      break;
    case LAUNCHING:
      doLaunching();
      break;
    case DOCKING:
      doDocking();
      break;
    case LOOKING_FOR_BWF:
      doLookForBWF();
      break;
    case CHARGING:
      doCharging();
      break;
  }

  if (millis() - lastDisplayUpdate > 15000)
  {
    Display.update();

    char buf[40];

  if (USE_MQTT)
      {
        sprintf_P(buf, PSTR("{\"battery\": %i}"), Battery.getVoltage());
        UpdateJSONObject(MQTT_BATTERY, buf);
        sprintf_P(buf, PSTR("{\"looptime\": %i}"), millis() - looptime);
        UpdateJSONObject(MQTT_LOOPTIME, buf);
      }
      else
      {
        sprintf_P(buf, PSTR("Battery %imV"), Battery.getVoltage());
        Serial.println(buf);
        sprintf(buf, PSTR("looptime %ims"), millis() - looptime);
        Serial.println(buf);
      }
    lastDisplayUpdate = millis();
  }
}


#pragma region MQTT

void mqttData(void *response)
{
 
  ELClientResponse *res = (ELClientResponse *)response;

  String topic = res->popString();
  String data = res->popString();

  char temp[64];
  // response needs to be copied due to scope issues i guess //
  strcpy(temp,api.API_Parse_Command(topic ,&data));
  mqtt.publish("/liam/1/cmd_resp",temp);
};
void mqttConnected(void *response)
{
  mqtt.subscribe("/liam/1/cmd/#");
  Serial.println(F("MQTT CONNECTED"));
  connected = true;
}
void mqttDisconnected(void *response)
{
  connected = false;
  Serial.println(F("MQTT DISCONNECTED"));
}

void mqtt_setup()
{
  bool ok;
  do
  {
    ok = esp.Sync();
    if(!ok) Serial.println(".");
  } while(!ok);
  Serial.println(F("Synced"));
  mqtt.connectedCb.attach(mqttConnected);
  mqtt.disconnectedCb.attach(mqttDisconnected);
  mqtt.dataCb.attach(mqttData);
  mqtt.setup();
}

void UpdateJSONObject(int MQTT_VALUES, char *value)
{
if(!connected)
  {
    Serial.println(F("Not connected"));
    return;
  }

  switch (MQTT_VALUES)
  {
  case MQTT_BATTERY:
    mqtt.publish("/liam/1/event/battery", value, 0, 1);
    break;
  case MQTT_STATE:
    mqtt.publish("/liam/1/event/state", value, 0, 1);
    break;
  case MQTT_MESSAGE:
    mqtt.publish("/liam/1/event/lastmessage", value, 0, 1);
    break;
  case MQTT_LOOPTIME:
    mqtt.publish("/liam/1/event/looptime", value, 0, 1);
    break;
  default:
    break;
  }
};


#pragma endregion MQTT