/*
API INIT
*/

#include "API.h"
#include <ELClient.h>
#include <ELClientCmd.h>
#include <ELClientMqtt.h>
#include <Arduino.h>
#include "Definition.h"


extern int command;
extern int state;




ELClient esp(&Serial,&Serial);
ELClientCmd cmd(&esp);
ELClientMqtt mqtt(&esp);
void mqttData(void *response)
{
  ELClientResponse *res = (ELClientResponse *)response;
  String topic = res->popString();
  String data = res->popString();
  Serial.print("MQTT In (");
  Serial.print(topic);
  Serial.print(") ");
  Serial.println(data);
  if(topic == F("/mower/1/command"))
  {
    if(data == "M")
      command = CMD_MOW;
    else if(data == "H")
      command = CMD_HOME;
    else
      command = CMD_AUTO;
  }
}
bool connected;
void mqttConnected(void *response)
{
  mqtt.subscribe("/liam/1/cmd");
  Serial.println("MQTT CONNECTED");
  connected = true;
}
void mqttDisconnected(void *response)
{
  connected = false;
  Serial.println("MQTT DISCONNECTED");
}

void mqtt_setup()
{
  
  bool ok;
  do
  {
    ok = esp.Sync();
    if(!ok) Serial.println(".");
  } while(!ok);
  Serial.println("Synced");
  mqtt.connectedCb.attach(mqttConnected);
  mqtt.disconnectedCb.attach(mqttDisconnected);
  mqtt.dataCb.attach(mqttData);
  mqtt.setup();
  
}

long last_mqtt;


void mqtt_send()
{
  esp.Process();
  if(!connected)
  {
    Serial.println("Not connected");
    return;
  }
  if((millis()-last_mqtt) < 10000)
  {
    Serial.println("Not time to send");
    return;
  }

  // last_mqtt = millis();
}

void UpdateJSONObject(int MQTT_VALUES,char *value)
{
    // Add values in the object
switch (MQTT_VALUES)
{
case MQTT_BATTERY:
  mqtt.publish("/liam/1/Event/Battery", value);
break;
case MQTT_STATE:
  mqtt.publish("/liam/1/Event/State", value);
break;
case MQTT_MESSAGE:
  mqtt.publish("/liam/1/Event/LM", value);
break;
case MQTT_LOOPTIME:
  mqtt.publish("/liam/1/Event/looptime", value);
break;
default:
break;
}
};
