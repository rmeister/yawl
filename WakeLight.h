#ifndef WAKELIGHT_H
#define WAKELIGHT_H

#include "Arduino.h"
#include <WebSocketsServer.h>


class WakeLight{
	public:
		WakeLight(time_t alarmTime, 
			uint8_t alarmID, 
			int timeout, 
			int duration, 
			int value, 
			bool daily);
		WakeLight();
		~WakeLight();

		void decodeSettings(char* payload);
		void updateAlarm();
		void saveParameters();
		void restoreParameters();
		bool paramsChanged();
		void toCharBuffer(char* buffer, int size);

		bool isDaily();
		int getDuration();
		int getTimeout();
		int getValue();
		uint8_t getAlarmID();
		time_t getAlarmTime();

		void setDaily(bool daily);
		void setDuration(int dur);
		void setTimeout(int tout);
		void setAlarmID(uint8_t aID);
		void setAlarmTime(time_t atime);

		bool incValue();
		bool decValue();
		int setValue(int val);

	private:
		time_t alarmTime;
		uint8_t alarmID;
		int timeout;	// seconds
		int duration;	// seconds
		bool daily;
		int value;		// current brightness

		char buffer[128];
		bool changeFlag;

		void init(
			time_t alarmTime, 
			uint8_t alarmID, 
			int timeout, 
			int duration, 
			int value, 
			bool daily);
		void updateAlarm(time_t t);
};


// workaroud wrapper callback functions 
void alarm();
void timeout();
void wsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght);

#endif