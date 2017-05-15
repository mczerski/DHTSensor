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
 * Version 1.0: Henrik EKblad
 * Version 1.1 - 2016-07-20: Converted to MySensors v2.0 and added various improvements - Torben Woltjen (mozzbozz)
 * 
 * DESCRIPTION
 * This sketch provides an example of how to implement a humidity/temperature
 * sensor using a DHT11/DHT-22.
 *  
 * For more information, please visit:
 * http://www.mysensors.org/build/humidity
 * 
 */

// Enable debug prints
//#define MY_DEBUG

// Enable and select radio type attached 
#define MY_RADIO_NRF24
#define MY_RF24_CE_PIN 9
#define MY_RF24_CS_PIN 8
#define MY_RF24_CHANNEL 100
//#define MY_RADIO_RFM69
//#define MY_RS485

#define MY_NODE_ID 2

#include <MyMySensors.h>
#include <SPI.h>
#include <DHT.h>

// Set this to the pin you connected the DHT's data pin to
#define DHT_DATA_PIN 6
#define DHT_POWER_PIN 17
#define BUTTON_PIN 2
#define MY_LED 19
#define POWER_BOOST_PIN 18

// Set this offset if the sensor has a permanent small offset to the real temperatures
#define SENSOR_TEMP_OFFSET 0

// Sleep time between sensor updates (in milliseconds)
// Must be >1000ms for DHT22 and >2000ms for DHT11
static const unsigned long SLEEP_TIME = 600000;

MyValue<float> humidity(0, V_HUM, S_HUM);
MyValue<float> temperature(1, V_TEMP, S_TEMP);

DHT dht;

bool powerOn()
{
  pinMode(POWER_BOOST_PIN, OUTPUT);
  digitalWrite(POWER_BOOST_PIN, HIGH);
  return true;
}

bool b = powerOn();

void presentation()  
{
  // Send the sketch version information to the gateway
  sendSketchInfo("TemperatureAndHumidity", "1.2");

  humidity.presentValue();
  temperature.presentValue();
}

void setup()
{
  myMySensorsSetup();
  pinMode(MY_LED, OUTPUT);
  digitalWrite(MY_LED, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(POWER_BOOST_PIN, OUTPUT);
  digitalWrite(POWER_BOOST_PIN, HIGH);

  pinMode(DHT_POWER_PIN, OUTPUT);
  digitalWrite(DHT_POWER_PIN, LOW);

  dht.setup(DHT_DATA_PIN, DHT::DHT22); // set data pin of DHT sensor
}

void loop()
{
  checkTransport();
  digitalWrite(POWER_BOOST_PIN, HIGH);
  digitalWrite(DHT_POWER_PIN, HIGH);
  sleep(dht.getMinimumSamplingPeriod() + 1020);
  dht.readSensor(true);

  digitalWrite(MY_LED, HIGH);

  float temp = dht.getTemperature();
  float hum = dht.getHumidity();
  pinMode(DHT_DATA_PIN, INPUT);
  digitalWrite(DHT_POWER_PIN, LOW);

  temp += SENSOR_TEMP_OFFSET;
  bool success = temperature.updateValue(temp);
  success &= humidity.updateValue(hum);

  digitalWrite(POWER_BOOST_PIN, LOW);

  unsigned long sleepTimeout = getSleepTimeout(success, SLEEP_TIME);

  int wakeUpCause = sleep(digitalPinToInterrupt(BUTTON_PIN), FALLING, sleepTimeout);
  if (wakeUpCause == digitalPinToInterrupt(BUTTON_PIN)) {
    digitalWrite(MY_LED, LOW);
    temperature.forceResend();
    humidity.forceResend();
  }
}

