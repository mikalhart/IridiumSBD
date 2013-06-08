#include <IridiumSBD.h>
#include <SoftwareSerial.h>

SoftwareSerial nss(18, 19);
IridiumSBD isbd(nss, 10);

void setup()
{
  int signalQuality = -1;

  Serial.begin(115200);
  nss.begin(19200);

  isbd.attachConsole(Serial);
  isbd.attachDiags(Serial);

  isbd.begin();

  isbd.setPowerProfile(1);

  int err = isbd.getSignalQuality(signalQuality);
  if (err != 0)
  {
    Serial.print("SignalQuality failed: error ");
    Serial.println(err);
    return;
  }

  Serial.print("Signal quality is ");
  Serial.println(signalQuality);

  uint8_t buffer[200] = 
  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
  size_t bufferSize = sizeof(buffer);
  err = isbd.sendReceiveSBDBinary(buffer, 20, buffer, bufferSize);
  if (err != 0)
  {
    Serial.print("sendReceiveSBDText failed: error ");
    Serial.println(err);
    return;
  }

  Serial.print("Inbound buffer size is ");
  Serial.println(bufferSize);
  for (int i=0; i<bufferSize; ++i)
  {
    Serial.write(buffer[i]);
    Serial.print("(");
    Serial.print(buffer[i], HEX);
    Serial.print(") ");
  }
  Serial.print("Messages left: ");
  Serial.println(isbd.getWaitingMessageCount());
}

void loop()
{
}

