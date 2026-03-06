#include "communication.h"

#include <Arduino.h>
#include <string.h>

#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_MEGA2560)
#include <SoftwareSerial.h>
SoftwareSerial classSoftSerial(7,8);
#define CLASS_SERIAL classSoftSerial
#else
#define CLASS_SERIAL Serial
#endif

char rxBuf[64];
uint8_t rxIdx = 0;

void initCommunication()
{
    CLASS_SERIAL.begin(COMM_BAUDRATE);
}


bool readCommand(int &class_, float &angle, int &action)
{
    while(CLASS_SERIAL.available() > 0)
    {
        char c = CLASS_SERIAL.read();

        if(c == '\r') continue;

        if(c == '\n')
        {
            if(rxIdx == 0) return false;

            rxBuf[rxIdx] = '\0';

            char *p1 = strtok(rxBuf,",");
            char *p2 = strtok(NULL,",");
            char *p3 = strtok(NULL,",");

            if(p1 == NULL) return false;

            class_ = atoi(p1);

            if(p2 != NULL)
                angle = atof(p2);
            else
                angle = 0;

            if(p3 != NULL)
                action = atoi(p3);
            else
                action = 0;

            rxIdx = 0;
            return true;
        }

        if(rxIdx < sizeof(rxBuf)-1)
            rxBuf[rxIdx++] = c;
        else
            rxIdx = 0;
    }

    return false;
}