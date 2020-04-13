#include <Arduino.h>
#include "RFM69.h"

#define PinNSS 10

#define PinDIO0 2
//instance RFM69
RFM69 rfm69;

void checkcmd()
{
  String cmdstr;
  unsigned char reg, regval;
  char cmd;
  char *ptr;
  if (Serial.available() > 0)
  {
    cmdstr = Serial.readStringUntil('\n');
    //we assume "register, value" = 5 byte
    cmd = cmdstr.charAt(0);
    reg = strtoul(cmdstr.substring(1, 3).c_str(), &ptr, 16);

    if (cmd == 'w')
    {
      rfm69.setModeStdby();
      regval = strtoul(cmdstr.substring(4, 6).c_str(), &ptr, 16);
      rfm69.writeSPI(reg, regval);
      Serial.print("set ");
      Serial.print(reg, HEX);
      Serial.print(" to ");
      rfm69.setModeRx();
    }
    if (cmd == 'r')
    {
      regval = rfm69.readSPI(reg);
      Serial.print(reg, HEX);
      Serial.print(" is: ");
    }

    Serial.println(regval, HEX);
  }
}

void setup()
{

  Serial.begin(115200);
  Serial.println("here is the RFM_F007TH");

  if (!rfm69.init433(PinNSS, PinDIO0))
  {
    Serial.println("error initializing RFM69");
  }
  else
  {
    Serial.println("RFM69 ready");
  };

  rfm69.setModeRx();
}

void loop()
{
  // put your main code here, to run repeatedly:

  byte cnt = rfm69.rxMsg();
 // if (rfm69._RxBuffer[0] == 0x45)
  if (cnt > 0)
  {
    /*
    for (byte i = 0; i < cnt; i++)
    {
      Serial.print(rfm69._RxBuffer[i], HEX);
    }
    Serial.println(); */
    
    byte stnId = (rfm69._RxBuffer[1] & B01110000) >> 4;

    Serial.print(stnId +1);
    Serial.print(":");
    uint16_t chTemp = ((rfm69._RxBuffer[1] & B00000111) << 8) +rfm69._RxBuffer[2];
    Serial.print ((chTemp -720) * 0.0556);
    Serial.print(":");
    byte hum = rfm69._RxBuffer[3];
    Serial.print(hum);
    Serial.print(":");
    Serial.println(rfm69.getLastRSSI());
    
  }

  checkcmd();
}
