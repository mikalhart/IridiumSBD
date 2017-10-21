#include <IridiumSBD.h>

/*
 * Ring
 * 
 * This sketch demonstrates how to use the Iridium RING line to detect
 * when inbound messages are available and retrieve them.
 * 
 * Assumptions
 * 
 * The sketch assumes an Arduino Mega or other Arduino-like device with
 * multiple HardwareSerial ports.  It assumes the satellite modem is
 * connected to Serial3.  Change this as needed.  SoftwareSerial on an Uno
 * works fine as well.
 * 
 * This sketch assumes the RING pin is connected to Arduino pin 5
 * 
 */
 
#define IridiumSerial Serial3
#define RING_PIN 5
#define DIAGNOSTICS false // Change this to see diagnostics

IridiumSBD modem(IridiumSerial, -1, RING_PIN);

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

  // Check signal quality for fun.
  int err = modem.getSignalQuality(signalQuality);
  if (err != 0)
  {
    Serial.print("SignalQuality failed: error ");
    Serial.println(err);
    return;
  }

  Serial.print("Signal quality is ");
  Serial.println(signalQuality);
  Serial.println("Begin waiting for RING...");
}


void loop()
{
  static int err = ISBD_SUCCESS;
  bool ring = modem.hasRingAsserted();
  if (ring || modem.getWaitingMessageCount() > 0 || err != ISBD_SUCCESS)
  {
    if (ring)
      Serial.println("RING asserted!  Let's try to read the incoming message.");
    else if (modem.getWaitingMessageCount() > 0)
      Serial.println("Waiting messages available.  Let's try to read them.");
    else
      Serial.println("Let's try again.");

    uint8_t buffer[200];
    size_t bufferSize = sizeof(buffer);
    err = modem.sendReceiveSBDText(NULL, buffer, bufferSize);
    if (err != ISBD_SUCCESS)
    {
      Serial.print("sendReceiveSBDBinary failed: error ");
      Serial.println(err);
      return;
    }

    Serial.println("Message received!");
    Serial.print("Inbound message size is ");
    Serial.println(bufferSize);
    for (int i=0; i<(int)bufferSize; ++i)
    {
      Serial.print(buffer[i], HEX);
      if (isprint(buffer[i]))
      {
        Serial.print("(");
        Serial.write(buffer[i]);
        Serial.print(")");
      }
      Serial.print(" ");
    }
    Serial.println();
    Serial.print("Messages remaining to be retrieved: ");
    Serial.println(modem.getWaitingMessageCount());
  }
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

