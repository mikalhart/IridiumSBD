#include <IridiumSBD.h>

/*
 * Sleep
 * 
 * This sketch demonstrates how to use the RockBLOCK Sleep pin.  The
 * general strategy is to call modem.begin() to start, then modem.sleep()
 * to stop.
 * 
 * Assumptions
 * 
 * The sketch assumes an Arduino Mega or other Arduino-like device with
 * multiple HardwareSerial ports.  It assumes the satellite modem is
 * connected to Serial3.  Change this as needed.  SoftwareSerial on an Uno
 * works fine as well.
 * 
 * This sketch also assumes that pin 4 is connected to Sleep
 */

#define IridiumSerial Serial3
#define SLEEP_PIN 4
#define DIAGNOSTICS false // Change this to see diagnostics

// Declare the IridiumSBD object (note SLEEP pin)
IridiumSBD modem(IridiumSerial, SLEEP_PIN);

void setup()
{ 
  // Start the console serial port
  Serial.begin(115200);
  while (!Serial);

  // Start the serial port connected to the satellite modem
  IridiumSerial.begin(19200);
}

void loop()
{
  // Begin satellite modem operation
  Serial.println("Starting modem...");
  int err = modem.begin();
  if (err != ISBD_SUCCESS)
  {
    Serial.print("Begin failed: error ");
    Serial.println(err);
    if (err == ISBD_NO_MODEM_DETECTED)
      Serial.println("No modem detected: check wiring.");
    return;
  }

  // Send the message
  Serial.print("Trying to send a message.  This might take several minutes.\r\n");
  err = modem.sendSBDText("Hello, world! (Sleep test)");
  if (err != ISBD_SUCCESS)
  {
    Serial.print("sendSBDText failed: error ");
    Serial.println(err);
    if (err == ISBD_SENDRECEIVE_TIMEOUT)
      Serial.println("Try again with a better view of the sky.");
  }

  else
  {
    Serial.println("Hey, it worked!");
  }

  // Put modem to sleep
  Serial.println("Putting modem to sleep.");
  err = modem.sleep();
  if (err != ISBD_SUCCESS)
  {
    Serial.print("sleep failed: error ");
    Serial.println(err);
  }

  // Demonstrate that device is asleep
  Serial.println("Trying to send while asleep.");
  err = modem.sendSBDText("This shouldn't work.");
  if (err == ISBD_IS_ASLEEP)
  {
    Serial.println("Couldn't send: device asleep.");
  }
  else
  {
    Serial.print("Send failed for some other reason: error ");
    Serial.println(err);
  }

  Serial.println("Sleeping for a minute.");
  delay(60 * 1000UL);
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
