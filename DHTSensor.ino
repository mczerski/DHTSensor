/**
   The MySensors Arduino library handles the wireless radio link and protocol
   between your home built sensors/actuators and HA controller of choice.
   The sensors forms a self healing radio network with optional repeaters. Each
   repeater and gateway builds a routing tables in EEPROM which keeps track of the
   network topology allowing messages to be routed to nodes.

   Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
   Copyright (C) 2013-2015 Sensnology AB
   Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors

   Documentation: http://www.mysensors.org
   Support Forum: http://forum.mysensors.org

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

 *******************************

   REVISION HISTORY
   Version 1.0: Henrik EKblad
   Version 1.1 - 2016-07-20: Converted to MySensors v2.0 and added various improvements - Torben Woltjen (mozzbozz)

   DESCRIPTION
   This sketch provides an example of how to implement a humidity/temperature[
   sensor using a DHT11/DHT-22.

   For more information, please visit:
   http://www.mysensors.org/build/humidity

*/

// Enable debug prints
//#define MY_DEBUG
//#define MY_MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
#define MY_RF24_CE_PIN 9
#define MY_RF24_CS_PIN 8
#define MY_RF24_CHANNEL 100
//#define MY_RADIO_RFM69
//#define MY_RS485
#define MY_TRANSPORT_WAIT_READY_MS 1

#define MY_NODE_ID 2

#include "MyMySensors/MyMySensors.h"
#include <SPI.h>
#include <DHT.h>

using namespace mymysensors;

// Set this to the pin you connected the DHT's data pin to
#define DHT_DATA_PIN 6
// Set this to the pin you connected the DHT's vcc pin to
#define DHT_POWER_PIN 17
// Set this to the pin you connected the wakeup button to
#define BUTTON_PIN 2
// Set this to the pin you connected led to
#define MY_LED 19
// Set this to the pin you connected dc/dc converter enable to
#define POWER_BOOST_PIN 18

// Sleep time between sensor updates (in milliseconds)
// Must be >1000ms for DHT22 and >2000ms for DHT11
static const unsigned long SLEEP_TIME = 600000;

DHT dht;
MyValue<float> humidity(0, V_HUM, S_HUM, 3.0);
MyValue<float> temperature(1, V_TEMP, S_TEMP, 0.5);
PowerManager& powerManager = PowerManager::initInstance(POWER_BOOST_PIN, true);

void presentation()
{
  // Send the sketch version information to the gateway
  sendSketchInfo("TemperatureAndHumidity", "1.3");

  humidity.presentValue();
  temperature.presentValue();
}

void setup()
{
  powerManager.setBatteryPin(A0);
  pinMode(MY_LED, OUTPUT);
  digitalWrite(MY_LED, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(DHT_POWER_PIN, OUTPUT);
  digitalWrite(DHT_POWER_PIN, LOW);

  dht.setup(DHT_DATA_PIN, DHT::DHT22); // set data pin of DHT sensor
}

void loop()
{
  powerManager.turnBoosterOn();
  digitalWrite(DHT_POWER_PIN, HIGH);
  //wait for everything to setup (minimum DHT sampling rate + 1s for dc/dc converter)
  sleep(dht.getMinimumSamplingPeriod() + 2000);
  dht.readSensor(true);

  //turn off led (in case it was turned on by the wakeup button)
  digitalWrite(MY_LED, HIGH);

  float temp = dht.getTemperature();
  float hum = dht.getHumidity();
  //this is necessary because dht library leaves the pull-up resistor enabled
  //and the dht sensor pulls the data pin low, so it consumes a lot of unnecesary power
  pinMode(DHT_DATA_PIN, INPUT);
  digitalWrite(DHT_POWER_PIN, LOW);
  powerManager.turnBoosterOff();
  wait(200);

  //if the transport layer is not operational state (in case the GW was down)
  //this will give time for the transport to recover
  checkTransport();

  //update values (send them to GW if necessary)
  bool success = temperature.updateValue(temp);
  success &= humidity.updateValue(hum);

  //this will calculate sleep timeout taking into account if the communication with GW was succesful
  unsigned long sleepTimeout = getSleepTimeout(success, SLEEP_TIME);

  int wakeUpCause = sleep(digitalPinToInterrupt(BUTTON_PIN), FALLING, sleepTimeout);
  if (wakeUpCause == digitalPinToInterrupt(BUTTON_PIN)) {
    //digitalWrite(MY_LED, LOW);
    //wakeup button forces sending data to GW (even if the readings wont change)
    temperature.forceResend();
    humidity.forceResend();
  }
}

