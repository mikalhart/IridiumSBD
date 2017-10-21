#include <IridiumSBD.h>

/*
 * SendReceive
 * 
 * This sketch demonstrates a basic bidirectional transmission.  Before
 * loading this sketch, send a message to your Iridium modem from your
 * control panel or via email.  This sketch will send a binary buffer
 * of 11 bytes, then attempt to read the next incoming messages (if any).
 * It stops when there are no more messages to read.
 * 
 * Assumptions
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

uint8_t buffer[200] = 
{ 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89 };

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
}

static bool messageSent = false;

void loop()
{
  int err;
  
  // Read/Write the first time or if there are any remaining messages
  if (!messageSent || modem.getWaitingMessageCount() > 0)
  {
    size_t bufferSize = sizeof(buffer);

    // First time through send+receive; subsequent loops receive only
    if (!messageSent)
      err = modem.sendReceiveSBDBinary(buffer, 11, buffer, bufferSize);
    else
      err = modem.sendReceiveSBDText(NULL, buffer, bufferSize);
      
    if (err != ISBD_SUCCESS)
    {
      Serial.print("sendReceiveSBD* failed: error ");
      Serial.println(err);
    }
    else // success!
    {
      messageSent = true;
      Serial.print("Inbound buffer size is ");
      Serial.println(bufferSize);
      for (int i=0; i<bufferSize; ++i)
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
