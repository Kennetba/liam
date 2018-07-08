/*
API INIT
*/

#include <Arduino.h>
#include "Definition.h"
#include "API.h"
#include "Battery.h"
#include <ArduinoJson.h>
#include "Controller.h"

API::API(BATTERY *battery,CONTROLLER *controller,int *state)
{
  this->battery = battery;
  this->controller = controller;
  this->state = state;
}

 char* API::Parse_Command(String topic,String *data)
{
  // holds command
   StaticJsonBuffer<100> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(*data);
    // Test if parsing succeeds.
    char _resp[64]; // Mqtt response.
    if (!root.success())
    {
      sprintf_P(_resp, PSTR("{\"Error\":\"%s\",\"Command\":\"%s\"}"),"JSON parse failed",data);
      return _resp;
    }
  int cmdargs[10] ={-1};
  // // set topic to lowecase..
  for (int i = 0; topic[i]; i++)
  {
    topic[i] = tolower(topic[i]);
  }
  if (topic == F("/liam/1/cmd/set"))
  {
    cmdargs[0] = (int)root["0"];
    cmdargs[1] = (int)root["1"]; // first value, set commands always have at least one.
    switch (cmdargs[0])
    {
      case API::Liam_Command::SetState:
     sprintf_P(_resp, PSTR("{\"Set_state\":\"%s\"}"),Cutter_states_STRING[cmdargs[1]]);
      *state = cmdargs[1];
        break;
      default:
      sprintf_P(_resp, PSTR("{\"cmd\":%i,\"result\":\"ERROR\"}"),cmdargs[0]);
        break;
    }
  }
  else if (topic == F("/liam/1/cmd/get"))
  {
    cmdargs[0] = (int)root["0"];
    cmdargs[1] = (int)root["1"]; // first value, might have a number
    switch (cmdargs[0])
    {
    case API::Liam_Command::GetState:
      sprintf_P(_resp, PSTR("{\"state\":%i,\"name\":\"%s\"}"), *this->state,Cutter_states_STRING[*this->state]);
      break;
    case API::Liam_Command::GetBattery:
      sprintf_P(_resp, PSTR("{\"Battery\":\"%i\"}"),(int)battery->getVoltage());
      break;
    default:
      sprintf_P(_resp, PSTR("{\"cmd\":%i,\"result\":%i}"), cmdargs[0], API::Liam_Command_STATUS_CODE::API_CODE_ERROR);
      break;
    }
  }
  else
  {
    sprintf_P(_resp, PSTR("{\"Error\":%i,\"message\":\"%s\"}"),cmdargs[0],"Could not read topic");
  }
  return _resp;
}
