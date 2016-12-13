#include <ArduinoJson.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <EEPROM.h>


#include "Config.h"
#include "WakeLight.h"

WakeLight::WakeLight(
		time_t alarmTime, 
		uint8_t alarmID, 
		int duration, 
		int timeout, 
		int value, 
		bool daily){
	init(alarmTime, alarmID, duration, timeout, value, daily);
}
WakeLight::WakeLight(){
	init(0, -1, 1200, 1200, 0, false);
}
WakeLight::~WakeLight(){}

void WakeLight::init(time_t alarmTime, 
		uint8_t alarmID, 
		int duration, 
		int timeout, 
		int value, 
		bool daily){
	this->alarmTime = alarmTime;
	this->alarmID = alarmID;
	this->timeout = timeout;
	this->duration = duration;
	this->daily = daily;
	this->value = value;
	this->changeFlag = false;
}

void WakeLight::toCharBuffer(char* buffer, int size){
	DynamicJsonBuffer jsonBuffer;
	JsonObject &root = jsonBuffer.createObject();
	root["time"] = getAlarmTime();
	root["duration"] = getDuration();
	root["timeout"] = getTimeout();
	root["value"] = (getValue()+1)>>2;
	root["daily"] = isDaily();
	root.prettyPrintTo(buffer, size);
}

bool 	WakeLight::isDaily()		{return daily;}
int 	WakeLight::getDuration()	{return duration;}
int 	WakeLight::getTimeout()		{return timeout;}
int 	WakeLight::getValue()		{return value;}
uint8_t WakeLight::getAlarmID()		{return alarmID;}
time_t 	WakeLight::getAlarmTime()	{return alarmTime;}

void WakeLight::setDaily(bool daily)		{ this->daily = daily;}
void WakeLight::setDuration(int dur)		{ duration = dur;}
void WakeLight::setTimeout(int tout)		{ timeout = tout;}
void WakeLight::setAlarmID(uint8_t aID)		{ alarmID = aID;}
void WakeLight::setAlarmTime(time_t atime)	{ alarmTime = atime;}


bool WakeLight::incValue(){
	if (value < 1023){
		value++;
		analogWrite(LEDPORT, value);
		return true;
	} else
		return false;
}

bool WakeLight::decValue(){
	if (value > 0){
		value--;
		analogWrite(LEDPORT, value);
		return true;
	} else
		return false;
}

int WakeLight::setValue(int val){
	int v = val;
	if (val > PWMRANGE)
		v = PWMRANGE;
	if (val < 0)
		v = 0;
	analogWrite(LEDPORT, v);
	value = v;
	return v;
}

void WakeLight::updateAlarm(){
	updateAlarm(alarmTime);
}
void WakeLight::updateAlarm(time_t t){
  time_t n = now();
  if (DEBUG) Serial.printf("Actual alarm updated to: %u = %u:%02u\nCurrent time: %u -> %u:%02u:%02u\n", t, hour(t), minute(t), n, hour(n), minute(n), second(n));
  time_t internalAlarm = t - getDuration();   
  Alarm.write(alarmID, internalAlarm);
  if (DEBUG) Serial.printf("Internal alarm updated to: %u\n", Alarm.read(getAlarmID()));
}

void WakeLight::decodeSettings(char*  payload){
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);
  if (!root.success()) {
    if (DEBUG) Serial.println("parseObject() failed");
    return;
  }
  if (root.containsKey("date") && root["date"] > 0)
  {
    Alarm.disable(alarmID); // if not disabled, alarm will trigger immediately after setting the new time
    setTime(root["date"]);
    Alarm.enable(alarmID);

    if (DEBUG) Serial.printf("Current date updated to: %u\n", now());
  }
  if (root.containsKey("time")){
    setAlarmTime(root["time"]);
    updateAlarm(root["time"]);    
	changeFlag = true;
  }
  if (root.containsKey("duration")){
    setDuration(root["duration"]);
    updateAlarm(getAlarmTime());
	changeFlag = true;
    if (DEBUG) Serial.printf("Set duration to %u seconds\n", getDuration());
  }
  if (root.containsKey("timeout")){
    setTimeout(root["timeout"]);
  	changeFlag = true;
    if (DEBUG) Serial.printf("Set timeout to %u seconds\n", getTimeout());
  }
  if (root.containsKey("value")){
    int val = root["value"];
    setValue((val<<2)-1);
  }
  if (root.containsKey("daily")){
    setDaily(root["daily"]);
  	changeFlag = true;
  }
}

bool WakeLight::paramsChanged(){
	return changeFlag;
}

int writeToEEPROM(byte start, time_t tme){
	for (int i=0; i<sizeof(tme); i++)
		EEPROM.write(start+i, (byte) (tme >> (i<<3)));	
	return start + sizeof(tme);
}
int writeToEEPROM(byte start, int val){
	for (int i=0; i<sizeof(val); i++)
		EEPROM.write(start+i, (byte) (val >> (i<<3)));
	return start + sizeof(val);
}
int readFromEEPROM(byte start, time_t* valAdr, byte length){
	time_t val = 0;
	for (int i=0; i<length; i++)
		val = val | (EEPROM.read(start+i) << (i<<3));
	*valAdr = val;
	return start + length;
}
int readFromEEPROM(byte start, int* valAdr, byte length){
	int val = 0;
	for (int i=0; i<length; i++)
		val = val | (EEPROM.read(start+i) << (i<<3));
	*valAdr = val;
	return start + length;
}

void WakeLight::saveParameters(){
	EEPROM.begin( sizeof(daily) + sizeof(alarmTime) + sizeof(timeout) + sizeof(duration) );

	EEPROM.write(0,1); // mark EEPROM content as valid

	int start = 1;
	start = writeToEEPROM(start, alarmTime);
	start = writeToEEPROM(start, duration);
	start = writeToEEPROM(start, timeout);
	EEPROM.write(start, daily);

	EEPROM.end();
	if (DEBUG) Serial.printf("EEPROM saved: %u|%u|%u|%u\n", daily, alarmTime, duration, timeout);
	changeFlag = false;
}

void WakeLight::restoreParameters(){
	EEPROM.begin( sizeof(daily) + sizeof(alarmTime) + sizeof(timeout) + sizeof(duration) );

	byte validContent = EEPROM.read(0);
	if (validContent == 1) {
		int start = 1;
		start = readFromEEPROM(start, &alarmTime, sizeof(alarmTime));
		start = readFromEEPROM(start, &duration, sizeof(duration));
		start = readFromEEPROM(start, &timeout, sizeof(timeout));
		daily = (bool) EEPROM.read(start);

		EEPROM.end();
		Serial.printf("EEPROM restored: %u|%u|%u|%u\n\n", daily, alarmTime, duration, timeout);
	} else 
		Serial.println("EEPROM content not valid, using defaults\n");
}