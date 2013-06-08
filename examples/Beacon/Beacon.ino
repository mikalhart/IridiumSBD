#include <IridiumSBD.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h> // NMEA parsing: http://arduiniana.org
#include <PString.h> // String buffer formatting: http://arduiniana.org

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/common.h>
#include <avr/wdt.h>
#include <avr/sleep.h>

// Time between transmissions
#define BEACON_INTERVAL 3600

SoftwareSerial ssIridium(18, 19);
SoftwareSerial ssGPS(3, 4);
IridiumSBD isbd(ssIridium, 10);
TinyGPS tinygps;
static const int ledPin = 13;
int iterationCounter = 0;

void setup()
{
  pinMode(ledPin, OUTPUT);

  // Start the serial ports
  Serial.begin(115200);

  // Setup the RockBLOCK
  isbd.attachConsole(Serial);
  isbd.attachDiags(Serial);
  isbd.setPowerProfile(1);
  isbd.useMSSTMWorkaround(false);
}

void loop()
{
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long dateFix, locationFix;
  float latitude, longitude;
  long altitude;
  bool fixFound = false;
  bool charsSeen = false;
  unsigned long loopStartTime = millis();

  // Step 0: Start the serial ports
  ssIridium.begin(19200);
  ssGPS.begin(4800);

  // Step 1: Reset TinyGPS and begin listening to the GPS
  Serial.println("Beginning to listen for GPS traffic...");
  tinygps = TinyGPS();
  ssGPS.listen();

  // Step 2: Look for GPS signal for up to 7 minutes
  for (unsigned long now = millis(); !fixFound && millis() - now < 7UL * 60UL * 1000UL;)
  {
    if (ssGPS.available())
    {
      charsSeen = true;
      if (tinygps.encode(ssGPS.read()))
      {
        tinygps.f_get_position(&latitude, &longitude, &locationFix);
        tinygps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &dateFix);
        altitude = tinygps.altitude();
        fixFound = locationFix != TinyGPS::GPS_INVALID_FIX_TIME && 
                   dateFix != TinyGPS::GPS_INVALID_FIX_TIME && 
                   altitude != TinyGPS::GPS_INVALID_ALTITUDE &&
                   year != 2000;
      }
    }
    ISBDCallback(); // We can call it during our GPS loop too.

    // if we haven't seen any GPS in 5 seconds, then the wiring is wrong.
    if (!charsSeen && millis() - now > 5000)
    break;
  }

  Serial.println(charsSeen ? fixFound ? F("A GPS fix was found!") : F("No GPS fix was found.") : F("Wiring error: No GPS data seen."));

  // Step 3: Start talking to the RockBLOCK and power it up
  Serial.println("Beginning to talk to the RockBLOCK...");
  ssIridium.listen();
  ++iterationCounter;
  if (isbd.begin() == ISBD_SUCCESS)
  {
    char outBuffer[60]; // Always try to keep message short

    if (fixFound)
    {
      sprintf(outBuffer, "%d%02d%02d%02d%02d%02d,", year, month, day, hour, minute, second);
      int len = strlen(outBuffer);
      PString str(outBuffer + len, sizeof(outBuffer) - len);
      str.print(latitude, 6);
      str.print(",");
      str.print(longitude, 6);
      str.print(",");
      str.print(altitude / 100);
      str.print(",");
      str.print(tinygps.f_speed_knots(), 1);
      str.print(",");
      str.print(tinygps.course() / 100);
    }

    else
    {
      sprintf(outBuffer, "%d: No GPS fix found!", iterationCounter);
    }

    Serial.print("Transmitting message '");
    Serial.print(outBuffer);
    Serial.println("'");
    isbd.sendSBDText(outBuffer);

    Serial.println("Putting RockBLOCK in sleep mode...");
    isbd.sleep();
  }

  // Sleep
  Serial.println("Going to sleep mode for about an hour...");
  ssIridium.end();
  ssGPS.end();
  delay(1000); // Wait for serial ports to clear
  digitalWrite(ledPin, LOW);
  pinMode(ledPin, INPUT);
  int elapsedSeconds = (int)((millis() - loopStartTime) / 1000);
  if (elapsedSeconds < BEACON_INTERVAL)
    bigSleep(BEACON_INTERVAL - elapsedSeconds); // wait about an hour

  // Wake
  Serial.println("Wake up!");
  pinMode(ledPin, OUTPUT);
}

bool ISBDCallback()
{
  digitalWrite(ledPin, (millis() / 1000) % 2 == 1 ? HIGH : LOW);
  return true;
}

// Sleep stuff
SIGNAL(WDT_vect) {
  wdt_disable();
  wdt_reset();
  WDTCSR &= ~_BV(WDIE);
}

void babySleep(uint8_t wdt_period) 
{
  wdt_enable(wdt_period);
  wdt_reset();
  WDTCSR |= _BV(WDIE);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_mode();
  wdt_disable();
  WDTCSR &= ~_BV(WDIE);
}

void smallSleep(int milliseconds) {
  while (milliseconds >= 8000) { babySleep(WDTO_8S); milliseconds -= 8000; }
  if (milliseconds >= 4000)    { babySleep(WDTO_4S); milliseconds -= 4000; }
  if (milliseconds >= 2000)    { babySleep(WDTO_2S); milliseconds -= 2000; }
  if (milliseconds >= 1000)    { babySleep(WDTO_1S); milliseconds -= 1000; }
  if (milliseconds >= 500)     { babySleep(WDTO_500MS); milliseconds -= 500; }
  if (milliseconds >= 250)     { babySleep(WDTO_250MS); milliseconds -= 250; }
  if (milliseconds >= 125)     { babySleep(WDTO_120MS); milliseconds -= 120; }
  if (milliseconds >= 64)      { babySleep(WDTO_60MS); milliseconds -= 60; }
  if (milliseconds >= 32)      { babySleep(WDTO_30MS); milliseconds -= 30; }
  if (milliseconds >= 16)      { babySleep(WDTO_15MS); milliseconds -= 15; }
}

void bigSleep(int seconds)
{
   while (seconds > 8) { smallSleep(8000); seconds -= 8;	}
   smallSleep(1000 * seconds);
}

