#include <IridiumSBD.h>

/*
   BasicSend

   This sketch sends a "Hello, world!" message from the satellite modem.
   If you have activated your account and have credits, this message
   should arrive at the endpoints you have configured (email address or
   HTTP POST).

   The TTGO T Beam unit does not have many if any spare GPIO ports. Most of the 
   ports are already in use for Esp32, Lora and GPS. So this only leaves you 

   Pins:
   GPIO-13
   GPIO-25

   All other spare pins are inputs only, best avoided.

   I've modified the code here accordingly to use (say) 12 and 13

   Wiring:
   TTGO Pin 13 (rxpin) --- Iridium txdata pin
   TTGO Pin 25 (txpin) --- Iridium rxdata pin

*/

#define IridiumSerial Serial2
#define DIAGNOSTICS false// Change this to see diagnostics
#define RXD2 13
#define TXD2 25

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
