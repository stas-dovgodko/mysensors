/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik EKblad
 *
 * DESCRIPTION
 * Example sketch showing how to measue light level using a LM393 photo-resistor
 * http://www.mysensors.org/build/light
 */

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Enable inclusion mode
#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
#define MY_INCLUSION_BUTTON_FEATURE

// Inverses behavior of inclusion button (if using external pullup)
//#define MY_INCLUSION_BUTTON_EXTERNAL_PULLUP

// Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_DURATION 60
// Digital pin used for inclusion mode button
#define MY_INCLUSION_MODE_BUTTON_PIN  3

#define MY_SIGNING_ATSHA204
#define MY_RF24_ENABLE_ENCRYPTION

#include <SPI.h>
#include <MySensors.h>

#define CHILD_ID 0
#define CHILD_ID1 1

unsigned long SLEEP_TIME = 5000; // Sleep time between reads (in milliseconds)

MyMessage msg(CHILD_ID, V_VOLTAGE);

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Voltage Sensor", "1.2");

  // Register all sensors to gateway (they will be created as child devices)
  present(CHILD_ID, S_MULTIMETER);
  present(CHILD_ID1, S_BINARY);
}

void receive(const MyMessage &message)
{
    // We only expect one type of message from controller. But we better check anyway.
    //if (message.type==V_STATUS) {
        
        Serial.print("Incoming change for sensor:");
        Serial.print(message.sensor);
        Serial.print(", New status: ");
        Serial.println(message.getBool());
    //}
}

void loop()
{
  /*int v = 220;
  Serial.print("V: ");
  Serial.print(v); // Convert ping time to distance in cm and print result (0 = outside set distance range)
  send(msg.set(v));
  sleep(SLEEP_TIME);*/
}
