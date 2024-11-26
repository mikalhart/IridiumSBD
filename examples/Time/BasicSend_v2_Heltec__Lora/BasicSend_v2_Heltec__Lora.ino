#include <IridiumSBD.h>

/*
   BasicSend

   This sketch sends a "Hello, world!" message from the satellite modem.
   If you have activated your account and have credits, this message
   should arrive at the endpoints you have configured (email address or
   HTTP POST).

   Assumptions

   The sketch assumes an Arduino Mega or other Arduino-like device with
   multiple HardwareSerial ports.  It assumes the satellite modem is
   connected to Serial3.  Change this as needed.  SoftwareSerial on an Uno
   works fine as well.

   you can't use U0TXD and U0RXD becuase they are used for the programming of the board, quite why Heltec brought these pins
   out to the edge connector I don't know, I suppose to allow an external programmer, who knows.
   The pins that are free are as follows:
   GPIO-17
   GPIO-2
   GPIO-22
   GPIO-23
   GPIO-0
   GPIO-12
   GPIO-13
   GPIO-25

   All other spare pins are inputs only, best avoided.

   I've modified the code here accordingly to use (say) 12 and 13

   Wiring:
   Heltec Pin 12 (rxpin) --- Iridium txdata pin
   Heltec Pin 13 (txpin) --- Iridium rxdata pin

*/

#define IridiumSerial Serial2
#define DIAGNOSTICS false// Change this to see diagnostics
#define RXD2 2
#define TXD2 0

// Declare the IridiumSBD object
IridiumSBD modem(IridiumSerial);


void setup()
{
  Serial.begin(19200);
  Serial.println("Starting setup...");

  int signalQuality = -1;
        int err;

  // Start the serial port connected to the satellite modem
  Serial.println("Starting satellite serial connection (swSer)");
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);

  // Begin satellite modem operation
  Serial.println("Starting modem...");
  err = modem.begin();
  Serial.print("modem: ");
  Serial.println(err);

  if (err != ISBD_SUCCESS)
  {
    Serial.print("Begin failed: error ");
    Serial.println(err);
    if (err == ISBD_NO_MODEM_DETECTED)
      Serial.println("No modem detected: check wiring.");
    return;
  }
}

void loop() {
}
