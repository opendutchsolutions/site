/*
  Reading the BQ40Z50 battery manager
  By: Nathan Seidle
  SparkFun Electronics
  Date: February 5th, 2024
  License: MIT. Please see LICENSE.md for more information.

  This example shows how to check the current battery level of the BQ40Z50 battery manager.

  These examples are written for the ESP32 but can be modified easily to be 
  used with any microcontroller that supports I2C.
*/

#include "SparkFun_BQ40Z50_Battery_Manager_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_BQ40Z50

#include <Wire.h>

BQ40Z50 myBattery;

int pin_SDA = 15;
int pin_SCL = 4;
uint16_t readMAC(uint16_t cmd)
{
  Wire.beginTransmission(0x0B);
  Wire.write(0x00);
  Wire.write(cmd & 0xFF);
  Wire.write(cmd >> 8);
  if (Wire.endTransmission() != 0)
  {
    Serial.println("MAC write failed");
    return 0xFFFF;
  }

  delay(5);   // gauge needs processing time

  Wire.beginTransmission(0x0B);
  Wire.write(0x00);
  Wire.endTransmission(false);

  Wire.requestFrom(0x0B, 2);
  if (Wire.available() < 2) return 0xFFFF;

  uint16_t val = Wire.read();
  val |= (Wire.read() << 8);
  return val;
}

bool writeWord(uint8_t command, uint16_t value)
{
  Wire.beginTransmission(0x0B);
  Wire.write(command);
  Wire.write(value & 0xFF);
  Wire.write(value >> 8);
  return (Wire.endTransmission() == 0);
}

void setup()
{
  Serial.begin(115200);
  delay(250);
  Serial.println();
  Serial.println("BQ40Z50 Battery Manager");

  Wire.begin(pin_SDA, pin_SCL);

  if (myBattery.begin() == false)
  {
    Serial.println("BQ40Z50 not detected. Freezing...");
    while (true);
  }
  Serial.println("BQ40Z50 detected!");
}

unsigned long lastCmd = 0;

void loop()
{
  uint16_t op = readMAC(0x0054); // low word
  Serial.printf("OperationStatus: 0x%04X\n", op);
  uint8_t soc = myBattery.getAbsoluteStateOfCharge();
  Serial.printf("State of charge: %d\n", soc);
  Serial.printf("XCHG=%d PRES=%d CHG_FET=%d\n",
              (op >> 13) & 1,   
              (op >> 0) & 1,    
              (op >> 2) & 1);

  Serial.printf("Voltage: %d mV\n", myBattery.getVoltageMv());
  Serial.printf("Current: %d mA\n", myBattery.getCurrentMa());
  Serial.printf("Temp: %.2f C\n", myBattery.getTemperatureC());
  Serial.printf("C1=%u C2=%u C3=%u C4=%u mV\n",
  myBattery.getCellVoltage1Mv(),
  myBattery.getCellVoltage2Mv(),
  myBattery.getCellVoltage3Mv(),
  myBattery.getCellVoltage4Mv());

  op = readMAC(0x0054);
  Serial.printf("OperationStatus: 0x%04X\n", op);

  delay(1000);
}
