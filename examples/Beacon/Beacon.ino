#include <IridiumSBD.h>
#include <TinyGPS++.h> // NMEA parsing: http://arduiniana.org

/*
 * Beacon
 * 
 * This sketch shows how you might use a GPS with the satellite modem
 * to create a beacon device that periodically transmits a location
 * message to the configured endpoints.
 * 
 * Assumptions
 * 
 * The sketch assumes an Arduino Mega or other Arduino-like device with
 * multiple HardwareSerial ports.  It assumes the satellite modem is
 * connected to Serial3.  Change this as needed.  SoftwareSerial on an Uno
 * works fine as well.  This assumes a 9600 baud GPS is connected to Serial2
 */

#define IridiumSerial Serial3
#define GPSSerial Serial2
#define GPSBaud 9600
#define DIAGNOSTICS false // Change this to see diagnostics

// Time between transmissions (seconds)
#define BEACON_INTERVAL 3600

IridiumSBD modem(IridiumSerial);
TinyGPSPlus tinygps;
static const int ledPin = 13;

void setup()
{
  pinMode(ledPin, OUTPUT);

  // Start the serial ports
  Serial.begin(115200);
  while (!Serial);
  IridiumSerial.begin(19200);
  GPSSerial.begin(GPSBaud);

  // Assume battery power
  modem.setPowerProfile(IridiumSBD::DEFAULT_POWER_PROFILE);

  // Setup the Iridium modem
  if (modem.begin() != ISBD_SUCCESS)
  {
    Serial.println("Couldn't begin modem operations.");
    exit(0);
  }
}

void loop()
{
  unsigned long loopStartTime = millis();

  // Begin listening to the GPS
  Serial.println("Beginning to listen for GPS traffic...");

  // Look for GPS signal for up to 7 minutes
  while ((!tinygps.location.isValid() || !tinygps.date.isValid()) && 
    millis() - loopStartTime < 7UL * 60UL * 1000UL)
  {
    if (GPSSerial.available())
      tinygps.encode(GPSSerial.read());
  }

  // Did we get a GPS fix?
  if (!tinygps.location.isValid())
  {
    Serial.println(F("Could not get GPS fix."));
    Serial.print(F("GPS characters seen = "));
    Serial.println(tinygps.charsProcessed());
    Serial.print(F("Checksum errors = "));
    Serial.println(tinygps.failedChecksum());
    return;
  }

  Serial.println(F("A GPS fix was found!"));

  // Step 3: Start talking to the RockBLOCK and power it up
  Serial.println("Beginning to talk to the RockBLOCK...");
  char outBuffer[60]; // Always try to keep message short
  sprintf(outBuffer, "%d%02d%02d%02d%02d%02d,%s%u.%09lu,%s%u.%09lu,%lu,%ld", 
    tinygps.date.year(), 
    tinygps.date.month(), 
    tinygps.date.day(), 
    tinygps.time.hour(), 
    tinygps.time.minute(), 
    tinygps.time.second(),
    tinygps.location.rawLat().negative ? "-" : "",
    tinygps.location.rawLat().deg,
    tinygps.location.rawLat().billionths,
    tinygps.location.rawLng().negative ? "-" : "",
    tinygps.location.rawLng().deg,
    tinygps.location.rawLng().billionths,
    tinygps.speed.value() / 100,
    tinygps.course.value() / 100);

    Serial.print("Transmitting message '");
    Serial.print(outBuffer);
    Serial.println("'");

  int err = modem.sendSBDText(outBuffer);
  if (err != ISBD_SUCCESS)
  {
    Serial.print("Transmission failed with error code ");
    Serial.println(err);
  }

  // Sleep
  int elapsedSeconds = (int)((millis() - loopStartTime) / 1000);
  if (elapsedSeconds < BEACON_INTERVAL)
  {
    int delaySeconds = BEACON_INTERVAL - elapsedSeconds;
    Serial.print(F("Waiting for "));
    Serial.println(delaySeconds);
    Serial.println(F(" seconds"));
    delay(1000UL * delaySeconds);
  }

  // Wake
  Serial.println("Wake up!");
}

void blinkLED()
{
  digitalWrite(ledPin, (millis() / 1000) % 2 == 1 ? HIGH : LOW);
}

bool ISBDCallback()
{
  blinkLED();
  return true;
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
