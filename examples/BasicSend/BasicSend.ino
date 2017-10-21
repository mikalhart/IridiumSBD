#include <IridiumSBD.h>

/*
 * BasicSend
 * 
 * This sketch sends a "Hello, world!" message from the satellite modem.
 * If you have activated your account and have credits, this message
 * should arrive at the endpoints you have configured (email address or
 * HTTP POST).
 * 
 * Assumptions
 * 
 * The sketch assumes an Arduino Mega or other Arduino-like device with
 * multiple HardwareSerial ports.  It assumes the satellite modem is
 * connected to Serial3.  Change this as needed.  SoftwareSerial on an Uno
 * works fine as well.
 */

#define IridiumSerial Serial3
#define DIAGNOSTICS false // Change this to see diagnostics

// Declare the IridiumSBD object
IridiumSBD modem(IridiumSerial);

void setup()
{
  int signalQuality = -1;
  int err;
  
  // Start the console serial port
  Serial.begin(115200);
  while (!Serial);

  // Start the serial port connected to the satellite modem
  IridiumSerial.begin(19200);

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

  // Example: Print the firmware revision
  char version[12];
  err = modem.getFirmwareVersion(version, sizeof(version));
  if (err != ISBD_SUCCESS)
  {
     Serial.print("FirmwareVersion failed: error ");
     Serial.println(err);
     return;
  }
  Serial.print("Firmware Version is ");
  Serial.print(version);
  Serial.println(".");

  // Example: Test the signal quality.
  // This returns a number between 0 and 5.
  // 2 or better is preferred.
  err = modem.getSignalQuality(signalQuality);
  if (err != ISBD_SUCCESS)
  {
    Serial.print("SignalQuality failed: error ");
    Serial.println(err);
    return;
  }

  Serial.print("On a scale of 0 to 5, signal quality is currently ");
  Serial.print(signalQuality);
  Serial.println(".");

  // Send the message
  Serial.print("Trying to send the message.  This might take several minutes.\r\n");
  err = modem.sendSBDText("Hello, world!");
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
}

void loop()
{
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
