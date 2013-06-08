/*
IridiumSBD - An Arduino library for Iridium SBD ("Short Burst Data") Communications
Suggested and generously supported by Rock Seven Location Technology
(http://rock7mobile.com), makers of the brilliant RockBLOCK satellite modem.
Copyright (C) 2013 Mikal Hart
All rights reserved.

The latest version of this library is available at http://arduiniana.org.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <WString.h> // for FlashString
#include <Stream.h> // for Stream
#include "Arduino.h"

#define ISBD_LIBRARY_REVISION           1

#define ISBD_DIAGS                      1

#define ISBD_DEFAULT_AT_TIMEOUT         30
#define ISBD_DEFAULT_CSQ_INTERVAL       10
#define ISBD_DEFAULT_CSQ_INTERVAL_USB   20
#define ISBD_DEFAULT_SBDIX_INTERVAL     30
#define ISBD_DEFAULT_SBDIX_INTERVAL_USB 30
#define ISBD_DEFAULT_SENDRECEIVE_TIME   300
#define ISBD_STARTUP_MAX_TIME           240
#define ISBD_DEFAULT_CSQ_MINIMUM        2

#define ISBD_SUCCESS             0
#define ISBD_ALREADY_AWAKE       1
#define ISBD_SERIAL_FAILURE      2
#define ISBD_PROTOCOL_ERROR      3
#define ISBD_CANCELLED           4
#define ISBD_NO_MODEM_DETECTED   5
#define ISBD_SBDIX_FATAL_ERROR   6
#define ISBD_SENDRECEIVE_TIMEOUT 7
#define ISBD_RX_OVERFLOW         8
#define ISBD_REENTRANT           9
#define ISBD_IS_ASLEEP           10
#define ISBD_NO_SLEEP_PIN        11

extern bool ISBDCallback() __attribute__ ((weak));
typedef const __FlashStringHelper *FlashString;

class IridiumSBD
{
public:
   int begin();
   int sendSBDText(const char *message);
   int sendSBDBinary(const uint8_t *txData, size_t txDataSize);
   int sendReceiveSBDText(const char *message, uint8_t *rxBuffer, size_t &rxBufferSize);
   int sendReceiveSBDBinary(const uint8_t *txData, size_t txDataSize, uint8_t *rxBuffer, size_t &rxBufferSize);
   int getSignalQuality(int &quality);

   int getWaitingMessageCount();
   int sleep();
   bool isAsleep();

   void setPowerProfile(int profile);          // 0 = direct connect (default), 1 = USB
   void adjustATTimeout(int seconds);          // default value = 20 seconds
   void adjustSendReceiveTimeout(int seconds); // default value = 300 seconds
   void setMinimumSignalQuality(int quality);  // a number between 1 and 5, default ISBD_DEFAULT_CSQ_MINIMUM
   void useMSSTMWorkaround(bool useWorkAround); // true to use workaround from Iridium Alert 5/7 

   void attachConsole(Stream &stream);
#if ISBD_DIAGS
   void attachDiags(Stream &stream);
#endif

   IridiumSBD(Stream &str, int sleepPinNo = -1) : 
      stream(str),
      pConsoleStream(NULL),
#if ISBD_DIAGS
      pDiagsStream(NULL),
#endif
      csqInterval(ISBD_DEFAULT_CSQ_INTERVAL),
      sbdixInterval(ISBD_DEFAULT_SBDIX_INTERVAL),
      atTimeout(ISBD_DEFAULT_AT_TIMEOUT),
      sendReceiveTimeout(ISBD_DEFAULT_SENDRECEIVE_TIME),
      remainingMessages(-1),
      asleep(true),
      reentrant(false),
      sleepPin(sleepPinNo),
      minimumCSQ(ISBD_DEFAULT_CSQ_MINIMUM),
      useWorkaround(true)
   {
      pinMode(sleepPin, OUTPUT);
   }

private:
   Stream &stream;
   Stream *pConsoleStream;
#if ISBD_DIAGS
   Stream *pDiagsStream;
#endif

   // Timings
   int csqInterval;
   int sbdixInterval;
   int atTimeout;
   int sendReceiveTimeout;

   // State variables  
   int remainingMessages;
   bool asleep;
   bool reentrant;
   int  sleepPin;
   int  minimumCSQ;
   bool useWorkaround;

   // Internal utilities
   bool smartWait(int seconds);
   bool waitForATResponse(char *response=NULL, int responseSize=0, const char *prompt=NULL, const char *terminator="OK\r\n");

   int  internalBegin();
   int  internalSendReceiveSBD(const char *txTxtMessage, const uint8_t *txData, size_t txDataSize, uint8_t *rxBuffer, size_t *prxBufferSize);
   int  internalGetSignalQuality(int &quality);
   int  internalMSSTMWorkaround(bool &okToProceed);
   int  internalSleep();

   int  doSBDIX(uint16_t &moCode, uint16_t &moMSN, uint16_t &mtCode, uint16_t &mtMSN, uint16_t &mtLen, uint16_t &mtRemaining);
   int  doSBDRB(uint8_t *rxBuffer, size_t *prxBufferSize); // in/out
   void power(bool on);

   void send(FlashString str, bool beginLine = true, bool endLine = true);
   void send(const char *str);
   void send(uint16_t n);

   bool cancelled();

   void dbg(FlashString str);
   void dbg(const char *str);
   void dbg(uint16_t n);
   void dbg(char c);

   void console(FlashString str);
   void console(const char *str);
   void console(uint16_t n);
   void console(char c);
};
