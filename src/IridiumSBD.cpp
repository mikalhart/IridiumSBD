/*
IridiumSBD - An Arduino library for Iridium SBD ("Short Burst Data") Communications
Suggested and generously supported by Rock Seven Location Technology
(http://rock7mobile.com), makers of the brilliant RockBLOCK satellite modem.
Copyright (C) 2013-2017 Mikal Hart
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

#include <time.h>
#include "IridiumSBD.h"

// Power on the RockBLOCK or return from sleep
int IridiumSBD::begin()
{
   if (this->reentrant)
      return ISBD_REENTRANT;

   this->reentrant = true;
   int ret = internalBegin();
   this->reentrant = false;

   // Absent a successful startup, keep the device turned off
   if (ret != ISBD_SUCCESS)
      power(false);

   return ret;
}

// Transmit a binary message
int IridiumSBD::sendSBDBinary(const uint8_t *txData, size_t txDataSize)
{
   if (this->reentrant)
      return ISBD_REENTRANT;

   this->reentrant = true;
   int ret = internalSendReceiveSBD(NULL, txData, txDataSize, NULL, NULL);
   this->reentrant = false;
   return ret;
}

// Transmit and receive a binary message
int IridiumSBD::sendReceiveSBDBinary(const uint8_t *txData, size_t txDataSize, uint8_t *rxBuffer, size_t &rxBufferSize)
{
   if (this->reentrant)
      return ISBD_REENTRANT;

   this->reentrant = true;
   int ret = internalSendReceiveSBD(NULL, txData, txDataSize, rxBuffer, &rxBufferSize);
   this->reentrant = false;
   return ret;
}

// Transmit a text message
int IridiumSBD::sendSBDText(const char *message)
{
   if (this->reentrant)
      return ISBD_REENTRANT;

   this->reentrant = true;
   int ret = internalSendReceiveSBD(message, NULL, 0, NULL, NULL);
   this->reentrant = false;
   return ret;
}

// Transmit a text message and receive reply
int IridiumSBD::sendReceiveSBDText(const char *message, uint8_t *rxBuffer, size_t &rxBufferSize)
{
   if (this->reentrant)
      return ISBD_REENTRANT;

   this->reentrant = true;
   int ret = internalSendReceiveSBD(message, NULL, 0, rxBuffer, &rxBufferSize);
   this->reentrant = false;
   return ret;
}

// High-level wrapper for AT+CSQ
int IridiumSBD::getSignalQuality(int &quality)
{
   if (this->reentrant)
      return ISBD_REENTRANT;

   this->reentrant = true;
   int ret = internalGetSignalQuality(quality);
   this->reentrant = false;
   return ret;
}

// Gracefully put device to lower power mode (if sleep pin provided)
int IridiumSBD::sleep()
{
   if (this->reentrant)
      return ISBD_REENTRANT;

   if (this->sleepPin == -1)
      return ISBD_NO_SLEEP_PIN;

   this->reentrant = true;
   int ret = internalSleep();
   this->reentrant = false;

   if (ret == ISBD_SUCCESS)
      power(false); // power off
   return ret;
}

// Return sleep state
bool IridiumSBD::isAsleep()
{
   return this->asleep;
}

// Return number of pending messages
int IridiumSBD::getWaitingMessageCount()
{
   return this->remainingMessages;
}

// Define capacitor recharge times
void IridiumSBD::setPowerProfile(POWERPROFILE profile) // 0 = direct connect (default), 1 = USB
{
   switch(profile)
   {
   case DEFAULT_POWER_PROFILE:
      this->sbdixInterval = ISBD_DEFAULT_SBDIX_INTERVAL;
      break;

   case USB_POWER_PROFILE:
      this->sbdixInterval = ISBD_USB_SBDIX_INTERVAL;
      break;
   }
}

// Tweak AT timeout 
void IridiumSBD::adjustATTimeout(int seconds)
{
   this->atTimeout = seconds;
}

// Tweak Send/Receive SBDIX process timeout
void IridiumSBD::adjustSendReceiveTimeout(int seconds)
{
   this->sendReceiveTimeout = seconds;
}

void IridiumSBD::useMSSTMWorkaround(bool useWorkAround) // true to use workaround from Iridium Alert 5/7 
{
   this->msstmWorkaroundRequested = useWorkAround;
}

void IridiumSBD::enableRingAlerts(bool enable) // true to enable SBDRING alerts and RING signal pin
{
   this->ringAlertsEnabled = enable;
   if (enable)
      this->ringAsserted = false;
}

bool IridiumSBD::hasRingAsserted()
{
   if (!ringAlertsEnabled)
      return false;

   if (!reentrant)
   {
      // It's possible that the SBDRING message comes while we're not doing anything
      filterSBDRING();
   }

   bool ret = ringAsserted;
   ringAsserted = false;
   return ret;
}

int IridiumSBD::getSystemTime(struct tm &tm)
{
   char msstmResponseBuf[24];

   send(F("AT-MSSTM\r"));
   if (!waitForATResponse(msstmResponseBuf, sizeof(msstmResponseBuf), "-MSSTM: "))
      return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;

   if (!isxdigit(msstmResponseBuf[0]))
      return ISBD_NO_NETWORK;

   // Latest epoch began at May 11, 2014, at 14:23:55 UTC.
   struct tm epoch_start;
   epoch_start.tm_year = 2014 - 1900;
   epoch_start.tm_mon = 5 - 1;
   epoch_start.tm_mday = 11;
   epoch_start.tm_hour = 14;
   epoch_start.tm_min = 23;
   epoch_start.tm_sec = 55;

   unsigned long ticks_since_epoch = strtoul(msstmResponseBuf, NULL, 16);

   /* Strategy: we'll convert to seconds by finding the largest number of integral
      seconds less than the equivalent ticks_since_epoch. Subtract that away and 
      we'll be left with a small number that won't overflow when we scale by 90/1000.

      Many thanks to Scott Weldon for this suggestion.
   */
   unsigned long secs_since_epoch = (ticks_since_epoch / 1000) * 90;
   unsigned long small_ticks = ticks_since_epoch - (secs_since_epoch / 90) * 1000;
   secs_since_epoch += small_ticks * 90 / 1000;

   time_t epoch_time = mktime(&epoch_start);
   time_t now = epoch_time + secs_since_epoch;
   memcpy(&tm, localtime(&now), sizeof tm);
   return ISBD_SUCCESS;
}

int IridiumSBD::getFirmwareVersion(char *version, size_t bufferSize)
{
   if (bufferSize < 8)
      return ISBD_RX_OVERFLOW;

   send(F("AT+CGMR\r"));
   if (!waitForATResponse(version, bufferSize, "Call Processor Version: "))
      return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;

   return ISBD_SUCCESS;
}

/*
Private interface
*/

int IridiumSBD::internalBegin()
{
   diagprint(F("Calling internalBegin\r\n"));

   if (!this->asleep)
      return ISBD_ALREADY_AWAKE;

   power(true); // power on

   bool modemAlive = false;

   unsigned long startupTime = 500; //ms
   for (unsigned long start = millis(); millis() - start < startupTime;)
      if (cancelled())
         return ISBD_CANCELLED;

   // Turn on modem and wait for a response from "AT" command to begin
   for (unsigned long start = millis(); !modemAlive && millis() - start < 1000UL * ISBD_STARTUP_MAX_TIME;)
   {
      send(F("AT\r"));
      modemAlive = waitForATResponse();
      if (cancelled())
         return ISBD_CANCELLED;
   }

   if (!modemAlive)
   {
      diagprint(F("No modem detected.\r\n"));
      return ISBD_NO_MODEM_DETECTED;
   }

   // The usual initialization sequence
   FlashString strings[3] = { F("ATE1\r"), F("AT&D0\r"), F("AT&K0\r") };
   for (int i=0; i<3; ++i)
   {
      send(strings[i]); 
      if (!waitForATResponse())
         return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;
   }

   // Enable or disable RING alerts as requested by user
   // By default they are on if a RING pin was supplied on constructor
   diagprint(F("Ring alerts are")); diagprint(ringAlertsEnabled ? F("") : F(" NOT")); diagprint(F(" enabled.\r\n"));
   send(ringAlertsEnabled ? F("AT+SBDMTA=1\r") : F("AT+SBDMTA=0\r"));
   if (!waitForATResponse())
      return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;

   // Decide whether the internal MSSTM workaround should be enforced on TX/RX
   // By default it is unless the firmware rev is >= TA13001
   char version[8];
   int ret = getFirmwareVersion(version, sizeof(version));
   if (ret != ISBD_SUCCESS)
   {
      diagprint(F("Unknown FW version\r\n"));
      msstmWorkaroundRequested = true;
   }
   else
   {
      diagprint(F("Firmware version is ")); diagprint(version); diagprint(F("\r\n"));
      if (version[0] == 'T' && version[1] == 'A')
      {
         unsigned long ver = strtoul(version + 2, NULL, 10);
         msstmWorkaroundRequested = ver < ISBD_MSSTM_WORKAROUND_FW_VER;
      }
   }
   diagprint(F("MSSTM workaround is")); diagprint(msstmWorkaroundRequested ? F("") : F(" NOT")); diagprint(F(" enforced.\r\n"));

   // Done!
   diagprint(F("InternalBegin: success!\r\n"));
   return ISBD_SUCCESS;
}

int IridiumSBD::internalSendReceiveSBD(const char *txTxtMessage, const uint8_t *txData, size_t txDataSize, uint8_t *rxBuffer, size_t *prxBufferSize)
{
   diagprint(F("internalSendReceive\r\n")); 

   if (this->asleep)
      return ISBD_IS_ASLEEP;

   // Binary transmission?
   if (txData && txDataSize)
   {
      if (txDataSize > ISBD_MAX_MESSAGE_LENGTH)
         return ISBD_MSG_TOO_LONG;

      send(F("AT+SBDWB="), true, false);
      send(txDataSize);
      send(F("\r"), false);
      if (!waitForATResponse(NULL, 0, NULL, "READY\r\n"))
         return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;

      uint16_t checksum = 0;
      for (size_t i=0; i<txDataSize; ++i)
      {
         stream.write(txData[i]);
         checksum += (uint16_t)txData[i];
      }

      consoleprint(F("["));
      consoleprint((uint16_t)txDataSize);
      consoleprint(F(" bytes]"));

      diagprint(F("Checksum:"));
      diagprint(checksum);
      diagprint(F("\r\n"));

      stream.write(checksum >> 8);
      stream.write(checksum & 0xFF);

      if (!waitForATResponse(NULL, 0, NULL, "0\r\n\r\nOK\r\n"))
         return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;
   }

   else // Text transmission
   {
#if true // use long string implementation
      if (txTxtMessage == NULL) // It's ok to have a NULL txtTxtMessage if the transaction is RX only
      {
         send(F("AT+SBDWT=\r"));
         if (!waitForATResponse())
            return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;
      }
      else
      {
         // remove any embedded \r
         char *p = strchr(txTxtMessage, '\r');
         if (p) *p = 0;
         if (strlen(txTxtMessage) > ISBD_MAX_MESSAGE_LENGTH)
            return ISBD_MSG_TOO_LONG;
         send(F("AT+SBDWT\r"));
         if (!waitForATResponse(NULL, 0, NULL, "READY\r\n"))
            return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;
         send(txTxtMessage);
         send("\r");
         if (!waitForATResponse(NULL, 0, NULL, "0\r\n\r\nOK\r\n"))
            return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;
      }
#else
      send(F("AT+SBDWT="), true, false);
      if (txTxtMessage != NULL) // It's ok to have a NULL txtTxtMessage if the transaction is RX only
         send(txTxtMessage);
      send(F("\r"), false);
      if (!waitForATResponse())
         return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;
#endif
   }

   // Long SBDIX loop begins here
   for (unsigned long start = millis(); millis() - start < 1000UL * this->sendReceiveTimeout;)
   {
      bool okToProceed = true;
      if (this->msstmWorkaroundRequested)
      {
         okToProceed = false;
         int ret = internalMSSTMWorkaround(okToProceed);
         if (ret != ISBD_SUCCESS)
            return ret;
      }

      if (okToProceed)
      {
         uint16_t moCode = 0, moMSN = 0, mtCode = 0, mtMSN = 0, mtLen = 0, mtRemaining = 0;
         int ret = doSBDIX(moCode, moMSN, mtCode, mtMSN, mtLen, mtRemaining);
         if (ret != ISBD_SUCCESS)
            return ret;

         diagprint(F("SBDIX MO code: "));
         diagprint(moCode);
         diagprint(F("\r\n"));

         if (moCode >= 0 && moCode <= 4) // this range indicates successful return!
         {
            diagprint(F("SBDIX success!\r\n"));

            this->remainingMessages = mtRemaining;
            if (mtCode == 1 && rxBuffer) // retrieved 1 message
            {
               diagprint(F("Incoming message!\r\n"));
               return doSBDRB(rxBuffer, prxBufferSize);
            }

            else
            {
               // No data returned
               if (prxBufferSize) 
                  *prxBufferSize = 0;
            }
            return ISBD_SUCCESS;
         }

         else if (moCode == 12 || moCode == 14 || moCode == 16) // fatal failure: no retry
         {
            diagprint(F("SBDIX fatal!\r\n"));
            return ISBD_SBDIX_FATAL_ERROR;
         }

         else // retry
         {
            diagprint(F("Waiting for SBDIX retry...\r\n"));
            if (!noBlockWait(sbdixInterval))
               return ISBD_CANCELLED;
         }
      }

      else // MSSTM check fail
      {
         diagprint(F("Waiting for MSSTM retry...\r\n"));
         if (!noBlockWait(ISBD_MSSTM_RETRY_INTERVAL))
            return ISBD_CANCELLED;
      }
   } // big wait loop

   diagprint(F("SBDIX timeout!\r\n"));
   return ISBD_SENDRECEIVE_TIMEOUT;
}

int IridiumSBD::internalGetSignalQuality(int &quality)
{
   if (this->asleep)
      return ISBD_IS_ASLEEP;

   char csqResponseBuf[2];

   send(F("AT+CSQ\r"));
   if (!waitForATResponse(csqResponseBuf, sizeof(csqResponseBuf), "+CSQ:"))
      return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;

   if (isdigit(csqResponseBuf[0]))
   {
      quality = atoi(csqResponseBuf);
      return ISBD_SUCCESS;
   }

   return ISBD_PROTOCOL_ERROR;
}

int IridiumSBD::internalMSSTMWorkaround(bool &okToProceed)
{
   /*
   According to Iridium 9602 Product Bulletin of 7 May 2013, to overcome a system erratum:

   "Before attempting any of the following commands: +SBDDET, +SBDREG, +SBDI, +SBDIX, +SBDIXA the field application 
   should issue the AT command AT-MSSTM to the transceiver and evaluate the response to determine if it is valid or not:

   Valid Response: "-MSSTM: XXXXXXXX" where XXXXXXXX is an eight-digit hexadecimal number.

   Invalid Response: "-MSSTM: no network service"

   If the response is invalid, the field application should wait and recheck system time until a valid response is 
   obtained before proceeding. 

   This will ensure that the Iridium SBD transceiver has received a valid system time before attempting SBD communication. 
   The Iridium SBD transceiver will receive the valid system time from the Iridium network when it has a good link to the 
   satellite. Ensuring that the received signal strength reported in response to AT command +CSQ and +CIER is above 2-3 bars 
   before attempting SBD communication will protect against lockout.
   */
   char msstmResponseBuf[24];

   send(F("AT-MSSTM\r"));
   if (!waitForATResponse(msstmResponseBuf, sizeof(msstmResponseBuf), "-MSSTM: "))
      return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;

   // Response buf now contains either an 8-digit number or the string "no network service"
   okToProceed = isxdigit(msstmResponseBuf[0]);
   return ISBD_SUCCESS;
}

int IridiumSBD::internalSleep()
{
   if (this->asleep)
      return ISBD_IS_ASLEEP;

#if false // recent research suggest this is not what you should do when just sleeping
   // Best Practices Guide suggests this before shutdown
   send(F("AT*F\r"));

   if (!waitForATResponse())
      return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;
#endif

   return ISBD_SUCCESS;
}

bool IridiumSBD::noBlockWait(int seconds)
{
   for (unsigned long start=millis(); millis() - start < 1000UL * seconds;)
      if (cancelled())
         return false;

   return true;
}

// Wait for response from previous AT command.  This process terminates when "terminator" string is seen or upon timeout.
// If "prompt" string is provided (example "+CSQ:"), then all characters following prompt up to the next CRLF are
// stored in response buffer for later parsing by caller.
bool IridiumSBD::waitForATResponse(char *response, int responseSize, const char *prompt, const char *terminator)
{
   diagprint(F("Waiting for response "));
   diagprint(terminator);
   diagprint(F("\r\n"));

   if (response)
      memset(response, 0, responseSize);

   int matchPromptPos = 0; // Matches chars in prompt
   int matchTerminatorPos = 0; // Matches chars in terminator
   enum {LOOKING_FOR_PROMPT, GATHERING_RESPONSE, LOOKING_FOR_TERMINATOR};
   int promptState = prompt ? LOOKING_FOR_PROMPT : LOOKING_FOR_TERMINATOR;
   consoleprint(F("<< "));
   for (unsigned long start=millis(); millis() - start < 1000UL * atTimeout;)
   {
      if (cancelled())
         return false;

      while (filteredavailable() > 0)
      {
         char c = filteredread();
         if (prompt)
         {
            switch (promptState)
            {
            case LOOKING_FOR_PROMPT:
               if (c == prompt[matchPromptPos])
               {
                  ++matchPromptPos;
                  if (prompt[matchPromptPos] == '\0')
                     promptState = GATHERING_RESPONSE;
               }

               else
               {
                  matchPromptPos = c == prompt[0] ? 1 : 0;
               }

               break;
            case GATHERING_RESPONSE: // gathering response from end of prompt to first \r
               if (response)
               {
                  if (c == '\r' || responseSize < 2)
                  {
                     promptState = LOOKING_FOR_TERMINATOR;
                  }
                  else
                  {
                     *response++ = c;
                     responseSize--;
                  }
               }
               break;
            }
         }

         if (c == terminator[matchTerminatorPos])
         {
            ++matchTerminatorPos;
            if (terminator[matchTerminatorPos] == '\0')
               return true;
         }
         else
         {
            matchTerminatorPos = c == terminator[0] ? 1 : 0;
         }
      } // while (stream.available() > 0)
   } // timer loop
   return false;
}

bool IridiumSBD::cancelled()
{
   if (ringPin != -1 && digitalRead(ringPin) == LOW) // Active low per guide
      ringAsserted = true;

   if (ISBDCallback != NULL)
      return !ISBDCallback();

   return false;
}

int IridiumSBD::doSBDIX(uint16_t &moCode, uint16_t &moMSN, uint16_t &mtCode, uint16_t &mtMSN, uint16_t &mtLen, uint16_t &mtRemaining)
{
   // Returns xx,xxxxx,xx,xxxxx,xx,xxx
   char sbdixResponseBuf[32];
   send(F("AT+SBDIX\r"));
   if (!waitForATResponse(sbdixResponseBuf, sizeof(sbdixResponseBuf), "+SBDIX: "))
      return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;

   uint16_t *values[6] = { &moCode, &moMSN, &mtCode, &mtMSN, &mtLen, &mtRemaining };
   for (int i=0; i<6; ++i)
   {
      char *p = strtok(i == 0 ? sbdixResponseBuf : NULL, ", ");
      if (p == NULL)
         return ISBD_PROTOCOL_ERROR;
      *values[i] = atol(p);
   }
   return ISBD_SUCCESS;
}

int IridiumSBD::doSBDRB(uint8_t *rxBuffer, size_t *prxBufferSize)
{
   bool rxOverflow = false;

   send(F("AT+SBDRB\r"));
   if (!waitForATResponse(NULL, 0, NULL, "AT+SBDRB\r")) // waits for its own echo
      return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;

   // Time to read the binary data: size[2], body[size], checksum[2]
   unsigned long start = millis();
   while (millis() - start < 1000UL * atTimeout)
   {
      if (cancelled())
         return ISBD_CANCELLED;
      if (stream.available() >= 2)
         break;
   }

   if (stream.available() < 2)
      return ISBD_SENDRECEIVE_TIMEOUT;

   uint16_t size = 256 * stream.read() + stream.read();
   consoleprint(F("[Binary size:"));
   consoleprint(size);
   consoleprint(F("]"));

   for (uint16_t bytesRead = 0; bytesRead < size;)
   {
      if (cancelled())
         return ISBD_CANCELLED;

      if (stream.available())
      {
         uint8_t c = stream.read();
         bytesRead++;
         if (rxBuffer && prxBufferSize)
         {
            if (*prxBufferSize > 0)
            {
               *rxBuffer++ = c;
               (*prxBufferSize)--;
            }
            else
            {
               rxOverflow = true;
            }
         }
      }

      if (millis() - start >= 1000UL * atTimeout)
         return ISBD_SENDRECEIVE_TIMEOUT;
   }

   while (millis() - start < 1000UL * atTimeout)
   {
      if (cancelled())
         return ISBD_CANCELLED;
      if (stream.available() >= 2)
         break;
   }

   if (stream.available() < 2)
      return ISBD_SENDRECEIVE_TIMEOUT;

   uint16_t checksum = 256 * stream.read() + stream.read();
   consoleprint(F("[csum:"));
   consoleprint(checksum);
   consoleprint(F("]"));

   // Return actual size of returned buffer
   if (prxBufferSize) 
      *prxBufferSize = (size_t)size;

   // Wait for final OK
   if (!waitForATResponse())
      return cancelled() ? ISBD_CANCELLED : ISBD_PROTOCOL_ERROR;

   return rxOverflow ? ISBD_RX_OVERFLOW : ISBD_SUCCESS;
}

void IridiumSBD::power(bool on)
{
   this->asleep = !on;

   if (this->sleepPin == -1)
      return;

   pinMode(this->sleepPin, OUTPUT);

   if (on)
   {
      diagprint(F("Powering on modem...\r\n"));
      digitalWrite(this->sleepPin, HIGH); // HIGH = awake
      lastPowerOnTime = millis();
   }

   else
   {
      // Best Practices Guide suggests waiting at least 2 seconds
      // before powering off again
      unsigned long elapsed = millis() - lastPowerOnTime;
      if (elapsed < 2000UL)
         delay(2000UL - elapsed);

      diagprint(F("Powering off modem...\r\n"));
      digitalWrite(this->sleepPin, LOW); // LOW = asleep
   }
}

void IridiumSBD::send(FlashString str, bool beginLine, bool endLine)
{
   if (beginLine)
      consoleprint(F(">> "));
   consoleprint(str);
   if (endLine)
      consoleprint(F("\r\n"));
   stream.print(str);
}

void IridiumSBD::send(const char *str)
{
   consoleprint(F(">> "));
   consoleprint(str);
   consoleprint(F("\r\n"));
   stream.print(str);
}

void IridiumSBD::send(uint16_t n)
{
   consoleprint(n);
   stream.print(n);
}

void IridiumSBD::diagprint(FlashString str)
{
   PGM_P p = reinterpret_cast<PGM_P>(str);
   while (1)
   {
      char c = pgm_read_byte(p++);
      if (c == 0) break;
      ISBDDiagsCallback(this, c);
   }
}

void IridiumSBD::diagprint(const char *str)
{
   while (*str)
      ISBDDiagsCallback(this, *str++);
}

void IridiumSBD::diagprint(uint16_t n)
{
   char str[10];
   sprintf(str, "%u", n);
   diagprint(str);
}

void IridiumSBD::consoleprint(FlashString str)
{
   PGM_P p = reinterpret_cast<PGM_P>(str);
   while (1) 
   {
      char c = pgm_read_byte(p++);
      if (c == 0) break;
      ISBDConsoleCallback(this, c);
   }
}

void IridiumSBD::consoleprint(const char *str)
{
   while (*str)
      ISBDConsoleCallback(this, *str++);
}

void IridiumSBD::consoleprint(uint16_t n)
{
   char str[10];
   sprintf(str, "%u", n);
   consoleprint(str);
}

void IridiumSBD::consoleprint(char c)
{
   ISBDConsoleCallback(this, c);
}

void IridiumSBD::SBDRINGSeen()
{
   ringAsserted = true;
   diagprint(F("SBDRING alert seen!\r\n"));
}

// Read characters until we find one that doesn't match SBDRING
// If nextChar is -1 it means we are still entertaining a possible
// match with SBDRING\r\n.  Once we find a mismatch, stuff it into
// nextChar.
void IridiumSBD::filterSBDRING()
{
   while (stream.available() > 0 && nextChar == -1)
   {
      char c = stream.read();
      consoleprint(c);
      if (*head != 0 && c == *head)
      {
         ++head;
         if (*head == 0)
         {
            SBDRINGSeen();
            head = tail = SBDRING;
         }
         else
         {
            // Delay no more than 10 milliseconds waiting for next char in SBDRING
            for (unsigned long start = millis(); stream.available() == 0 && millis() - start < FILTERTIMEOUT; );

            // If there isn't one, assume this ISN'T an unsolicited SBDRING
            if (stream.available() == 0) // pop the character back into nextChar
            {
               --head;
               nextChar = c;
            }
         }
      }
      else
      {
         nextChar = c;
      }
   }
}

const char IridiumSBD::SBDRING[] = "SBDRING\r\n";

int IridiumSBD::filteredavailable()
{
   filterSBDRING();
   return head - tail + (nextChar != -1 ? 1 : 0);
}

int IridiumSBD::filteredread()
{
   filterSBDRING();

   // Use up the queue first
   if (head > tail)
   {
      char c = *tail++;
      if (head == tail)
         head = tail = SBDRING;
      return c;
   }

   // Then the "extra" char
   else if (nextChar != -1)
   {
      char c = (char)nextChar;
      nextChar = -1;
      return c;
   }


   return -1;
}
