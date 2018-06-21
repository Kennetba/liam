#ifndef _API_H_
#define _API_H_

#include "Definition.h"
#include "Battery.h"
#include "Controller.h"
class API
{
public:
  API(BATTERY * battery, CONTROLLER *controller,int *state);
  char* API_Parse_Command(String topic, String *data);
  
  enum Liam_Command_STATUS_CODE
  {
    OK = 1,
    API_CODE_ERROR = 2,
    ARGUMENT_FAILED = 3
  }; 
  enum Liam_Command
  {
    /* Notify 0-->99 */
    HEARTBEAT = 4,
    NOTIFY = 10,
    SUBSCRIBE = 11,
    DEBUG = 90,
    ONLINE = 99,
    /* GetCommands 100-->199 */
    GetCommands = 100,
    GetMowerStatus = 101,
    GetState = 102,
    GetBattery = 103,
    GetSensor = 104,
    GetMotor = 105, // ;105:MOTOR[0=left,1=Right,2=cutter]#
    GetSlowWheelWhenDocking = 106,
    GetWheelOverloadLevel = 107,

    /* Set commands 200-->299 */
    SetState = 202,                /* state */
    SetBattery = 203,              /* battype,min,max,gohome */
    SetMotor = 205,                //;205:MOTOR[0=left,1=Right,2=cutter]:SPEED# speed -100 --> 100
    SetSlowWheelWhenDocking = 206, /* value */
    SetWheelOverloadLevel = 207,   /* value */
    SetFirstByteToFalse = 998      /* <;998# > */
  };
  private:
  BATTERY *battery;
  CONTROLLER *controller;
  int *state;
};

#endif //_API_H_