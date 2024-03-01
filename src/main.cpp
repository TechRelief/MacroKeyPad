/*
This code will scan the macro-pad designed by Tech-Relief with 10 keys and output the programmed keystrokes via bluetooth.
See MacroPad.h for the keyCode0 and KeyCode1 tables where the key codes that are send via bluetooth are defined.
*/
#include <Arduino.h>
#include <BLEAdvertising.h>
#include "MacroPad.h"

BLEMultiAdvertising advert(1); // Max number of advertisement data used by Bluetooth to advertise itself as a device that can be paired

void setup()
{
  initMacroPad();
}

void loop(void)
{
  scanMacroPad();
}
