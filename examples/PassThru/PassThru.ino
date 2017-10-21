#include <IridiumSBD.h>

/*
 * PassThru
 * 
 * This sketch allows you to send data directly from the Serial console
 * to the modem.  Characters typed in the console are relayed to the 
 * modem and vice versa.
 * 
 * The sketch assumes an Arduino Mega or other Arduino-like device with
 * multiple HardwareSerial ports.  It assumes the satellite modem is
 * connected to Serial3.  Change this as needed.  SoftwareSerial on an Uno
 * works fine as well.
 * 
 */
 
#define IridiumSerial Serial3
#define DIAGNOSTICS false // Change this to see diagnostics

IridiumSBD modem(IridiumSerial);

void setup()
{
  int signalQuality = -1;

  // Start the serial ports
  Serial.begin(115200);
  while (!Serial);
  IridiumSerial.begin(19200);

  // Setup the Iridium modem
  modem.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);
  if (modem.begin() != ISBD_SUCCESS)
  {
    Serial.println("Couldn't begin modem operations.");
    exit(0);
  }

  // Check the signal quality (optional)
  int err = modem.getSignalQuality(signalQuality);
  if (err != 0)
  {
    Serial.print("SignalQuality failed: error ");
    Serial.println(err);
    exit(1);
  }

  Serial.print("Signal quality is ");
  Serial.println(signalQuality);

  Serial.println("Enter commands terminated by Carriage Return ('\\r'):");
}

void loop()
{
  if (Serial.available())
    IridiumSerial.write(Serial.read());
  if (IridiumSerial.available())
    Serial.write(IridiumSerial.read());
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
