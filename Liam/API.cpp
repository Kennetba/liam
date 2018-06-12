/*
API INIT
*/

#include "API.h"
#include <ELClient.h>
#include <ELClientCmd.h>
#include <ELClientMqtt.h>
#include <Arduino.h>
#include "Definition.h"
 #include <ArduinoJson.h>

StaticJsonBuffer<140> jsonBuffer;
JsonObject &JSONroot = jsonBuffer.createObject();
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

  char buf[64];

  itoa(state, buf, 10);
  mqtt.publish("/liam/1/state", buf);
  Serial.print("State:");
  Serial.println(buf);
  itoa(command, buf, 10);
  mqtt.publish("/liam/1/lastcmd", buf);
  Serial.print("Command:");
  JSONroot.printTo(buf);
  mqtt.publish("/liam/1/test", buf);
  last_mqtt = millis();
}

void UpdateJSONObject(int MQTT_VALUES,char *value)
{
    // Add values in the object
 Serial.println(value);
switch (MQTT_VALUES)
{
case MQTT_BATTERY:
JSONroot["Battery"] = value ;
break;
case MQTT_STATE:
JSONroot["State"] = value ;
break;
case MQTT_MESSAGE:
JSONroot["LastMessage"] = value ;
break;
default:
break;

}
JSONroot["LastCommand"] = command;
};
