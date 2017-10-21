#include <IridiumSBD.h>
#include <time.h>
/*
 * Time
 * 
 * This sketch demonstrates how to retrieve the Iridium system time
 * from the modem using the new getSystemTime method. This uses
 * the Iridium command AT-MSSTM to acquire the time.  The method will
 * fail if the Iridium network has not yet been acquired.
 * 
 * Assumptions
 * 
 * The sketch assumes an Arduino Mega or other Arduino-like device with
 * multiple HardwareSerial ports. It assumes the satellite modem is
 * connected to Serial3. Change this as needed. SoftwareSerial on an Uno
 * works fine as well.
 */

#define IridiumSerial Serial3
#define DIAGNOSTICS false // Change this to see diagnostics

// Declare the IridiumSBD object
IridiumSBD modem(IridiumSerial);

void setup()
{
  int err;
  
  // Start the console serial port
  Serial.begin(115200);
  while (!Serial);

  // Start the serial port connected to the satellite modem
  IridiumSerial.begin(19200);

  // If we're powering the device by USB, tell the library to 
  // relax timing constraints waiting for the supercap to recharge.
  modem.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);

  // Begin satellite modem operation
  Serial.println("Starting modem...");
  err = modem.begin();
  if (err != ISBD_SUCCESS)
  {
    Serial.print("Begin failed: error ");
    Serial.println(err);
    if (err == ISBD_NO_MODEM_DETECTED)
      Serial.println("No modem detected: check wiring.");
    return;
  }
}

void loop()
{
   struct tm t;
   int err = modem.getSystemTime(t);
   if (err == ISBD_SUCCESS)
   {
      char buf[32];
      sprintf(buf, "%d-%02d-%02d %02d:%02d:%02d",
         t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
      Serial.print("Iridium time/date is ");
      Serial.println(buf);
   }

   else if (err == ISBD_NO_NETWORK)
   {
      Serial.println("No network detected.  Waiting 10 seconds.");
   }

   else
   {
      Serial.print("Unexpected error ");
      Serial.println(err);
      return;
   }

   // Delay 10 seconds
   delay(10 * 1000UL);
}

#if DIAGNOSTICS
void ISBDConsoleCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}

void ISBDDiagsCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}
#endif
