#ifndef _API_H_
#define _API_H_

#include "ArduinoJson.h"
#include "Definition.h"

enum Liam_Command{
CMD_AUTO  = 0,
CMD_MOW,
CMD_HOME,
CMD_STOP,
CMD_DEBUG
};

// Memory pool for JSON object tree.
  //
  // Inside the brackets, 200 is the size of the pool in bytes.
  // Don't forget to change this value to match your JSON document.
  // Use arduinojson.org/assistant to compute the capacity.
  
  // Create the root of the object tree.
  //
  // It's a reference to the JsonObject, the actual bytes are inside the
  // JsonBuffer with all the other nodes of the object tree.
  // Memory is freed when jsonBuffer goes out of scope.
  


void mqtt_setup();
void mqtt_send();
void UpdateJSONObject(int MQTT_VALUES, char *value);

#endif //_API_H_