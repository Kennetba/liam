#ifndef _API_H_
#define _API_H_

#include "Definition.h"

enum Liam_Command{
CMD_AUTO  = 0,
CMD_MOW,
CMD_HOME,
CMD_STOP,
CMD_DEBUG
};
void mqtt_setup();
void mqtt_send();
void UpdateJSONObject(int MQTT_VALUES, char *value);

#endif //_API_H_