/*
* The MySensors Arduino library handles the wireless radio link and protocol
* between your home built sensors/actuators and HA controller of choice.
* The sensors forms a self healing radio network with optional repeaters. Each
* repeater and gateway builds a routing tables in EEPROM which keeps track of the
* network topology allowing messages to be routed to nodes.
*
* Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
* Copyright (C) 2013-2016 Sensnology AB
* Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
*
* Documentation: http://www.mysensors.org
* Support Forum: http://forum.mysensors.org
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*/


// Topic structure: MY_MQTT_PUBLISH_TOPIC_PREFIX/NODE-ID/SENSOR-ID/CMD-TYPE/ACK-FLAG/SUB-TYPE

#include "MyGatewayTransport.h"

#if defined MY_CONTROLLER_IP_ADDRESS
IPAddress _brokerIp(MY_CONTROLLER_IP_ADDRESS);
#endif


#define EthernetClient WiFiClient

#if defined(MY_IP_ADDRESS)
IPAddress _gatewayIp(MY_IP_GATEWAY_ADDRESS);
IPAddress _subnetIp(MY_IP_SUBNET_ADDRESS);
#endif


#if defined(MY_IP_ADDRESS)
IPAddress _MQTT_clientIp(MY_IP_ADDRESS);
#endif

extern PubSubClient MQTTClient;
extern WiFiClient wclient;
extern char mqtt_path[255];
extern char mqtt_user[255];
extern char mqtt_pass[255];
extern char mqtt_server[255];
extern char mqtt_port[6];


static bool _MQTT_connecting = true;
static bool _MQTT_available = false;
static MyMessage _MQTT_msg;

bool gatewayTransportSend(MyMessage &message)
{
	if (!MQTTClient.connected()) {
		return false;
	}
	setIndication(INDICATION_GW_TX);
	
	char buf[255];
  sprintf(buf, "%s%s",mqtt_path, MY_MQTT_PUBLISH_TOPIC_PREFIX);
	
	char *topic = protocolFormatMQTTTopic(buf, message);
	debug(PSTR("Sending message on topic: %s\n"), topic);
	return MQTTClient.publish(topic, message.getString(_convBuffer));
}

void incomingMQTT(char* topic, uint8_t* payload, unsigned int length)
{
	debug(PSTR("Message arrived on topic: %s\n"), topic);
	_MQTT_available = protocolMQTTParse(_MQTT_msg, topic, payload, length);
}

bool reconnectMQTT(void)
{
  if (!MQTTClient.connected()) {
  
    String host = String(mqtt_server);
  int port = String(mqtt_port).toInt();

  MY_SERIALDEVICE.print("Connecting to MQTT server (" + host + ":" + port + ") ...");
  bool cstatus = MQTTClient.setServer(mqtt_server, port).connect("clientId", mqtt_user, mqtt_pass);
  

  if (cstatus) {

    unsigned long time;
    time = millis();

    while (!MQTTClient.connected() && (millis() - time) < 5000) {
      delay(500);
      MY_SERIALDEVICE.print(".");

    }


    MY_SERIALDEVICE.println("connected");

    char buf[255];
    sprintf(buf, "%s%s/+/+/+/+/+",mqtt_path, MY_MQTT_SUBSCRIBE_TOPIC_PREFIX);
  
    MQTTClient.subscribe(buf);
    
    MY_SERIALDEVICE.println(buf);
    MY_SERIALDEVICE.println("subscribe");

    return MQTTClient.connected();
  } else {
    MY_SERIALDEVICE.println("error mqtt");

    return false;
  }
	}
return true;
}

bool gatewayTransportConnect(void)
{
MQTTClient.setCallback(incomingMQTT);
return true;
}

bool gatewayTransportInit(void)
{
	_MQTT_connecting = true;
	
	MQTTClient.setCallback(incomingMQTT);
	
	
	_MQTT_connecting = false;
	return true;
}

bool gatewayTransportAvailable(void)
{
	if (_MQTT_connecting) {
		return false;
	}
	//keep lease on dhcp address
	//Ethernet.maintain();
	if (!MQTTClient.connected()) {
		//reinitialise client
		if (gatewayTransportConnect()) {
			reconnectMQTT();
		}
		return false;
	} else {
	  

	}
	MQTTClient.loop();
	return _MQTT_available;
}

MyMessage & gatewayTransportReceive(void)
{
	// Return the last parsed message
	_MQTT_available = false;
	return _MQTT_msg;
}
