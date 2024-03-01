#include "MacroPad.h"

static uint8_t fnMode = 0; //Used to record the current function mode; 0 = none.

void initMacroPad()
{
  pinMode(D0, OUTPUT);  //Address for Multiplexer to address Key Switches
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  digitalWrite(D0, LOW);
  digitalWrite(D1, LOW);
  digitalWrite(D2, LOW);
  digitalWrite(D3, LOW);
  pinMode(D7, INPUT); //No need for pull up since the MOSFET already has a 10 K pullup.
  pinMode(D8, OUTPUT);
  digitalWrite(D8, HIGH); //Strobe when high to low will set output on D7, seems to need the strobe to cycle to get reliable output?!
  // start the Bluetooth task
  xTaskCreate(bluetoothTask, "bluetooth", 20000, NULL, 5, NULL);  //Second to last parameter is called uxPriority and was set to 5 on some boards this may need to be tweeked.
}

/// @brief scanKeyPad scans the specified key on the key pad to see if it is pressed
/// @param keyNum the key number to scan
/// @return true if the key is currently pressed down
bool scanKeyPad(uint8_t keyNum)
{
  digitalWrite(D0, ((keyNum & 0x01) != 0) ? HIGH : LOW);
  digitalWrite(D1, ((keyNum & 0x02) != 0) ? HIGH : LOW);
  digitalWrite(D2, ((keyNum & 0x04) != 0) ? HIGH : LOW);
  digitalWrite(D3, ((keyNum & 0x08) != 0) ? HIGH : LOW);
  digitalWrite(D8, LOW);
  delay(10);
  bool isDown = (digitalRead(D7) == LOW);
  digitalWrite(D8, HIGH);
  return isDown;
}

/// @brief Gets the function mode 0, 1 or 2.
/// @return If only key 9 is down it is mode 1; if both 9 and 4 are down it is mode 2 otherwise it is mode 0;
uint8_t getFnMode()
{
  if (scanKeyPad(9))
  {
    if (scanKeyPad(4))
      return 2;
    else
      return 1;
  }
  else
  {
    if (fnMode == 2)
    {
      if (scanKeyPad(4))  //if 4 is still down stay in fnMode 2 to avoid key 4 triggering a responce
        return 2; 
      else
        return 1;
    }
    else
      return 0;
  }
}

/// @brief If the key code and / or the text is non-zero output it via bluetooth HID codes
/// @param mod The modifiers like SHift, Ctrl, Alt or Win
/// @param key The HID keycode
/// @param text The ASCII text to output
void processKeyAndText(uint8_t mod, uint8_t key, char *text)
{
  //first process the key code if any...
  if (key != 0) //Only output the key code if it is non-zero
  {
    hidReport.modifier = mod;
    hidReport.keycode[0] = key;
    sendInputReport(hidReport); //Send the key report via bluetooth
    sendInputReport(NO_KEY_PRESSED); // Key released Report
  }
  if (text != 0)
    typeText(text);
}

void scanMacroPad()
{
  uint8_t newFnMode = getFnMode(); //Get the current function mode
  //Scan keys to see if any are down and then report them via Bluetooth when they are released
  for (uint8_t i=0; i<9; i++) //Key 9 is used as shift function key and never has any output
  {
    if ((fnMode != 0 || newFnMode != 0) && i == 4)
    {
      fnMode = newFnMode;
      continue; // Under mode 1 or 2 the number 4 key is used as a function key and should not trigger anything and if key 9 was released first it could trigger an unwanted key 4 response
    }
    // Scan each of the macro key pad keys
    if (scanKeyPad(i))
    {
      keyState[i] = true; //If the key is down just record it, we won't respond until it is released; we don't need a repeat function for these keys!
    }
    else
    {
      if (keyState[i]) //Was the key down before? We only report a key when it is released...
      {
        uint8_t mod; //Used to fetch the modifier to use
        uint8_t key; //Used to fetch the keycode to use
        char *text; //Used to fetch any text to output
        for (uint8_t x=0; x<4; x++) //The system allows for up to 4 keycodes and text per macro key to be output
        {
          hidReport = NO_KEY_PRESSED;  //Clear the report variable
          switch (fnMode)
          {
            case 0:
              mod = keyCode0[i][x].modifier;
              key = keyCode0[i][x].keycode;
              text = keyCode0[i][x].text;
              break;
            case 1:
              mod = keyCode1[i][x].modifier;
              key = keyCode1[i][x].keycode;
              text = keyCode1[i][x].text;
              break;
            case 2:
              mod = keyCode2[i][x].modifier;
              key = keyCode2[i][x].keycode;
              text = keyCode2[i][x].text;
              break;
          default:
              mod = 0;
              key = 0;
              text = 0;
            break;
          }
          processKeyAndText(mod, key, text);
        }
        sendInputReport(NO_KEY_PRESSED); // Key released Report
        keyState[i] = false; //Finally reset the keyState to show that the key press was processed
      }
    }
  }
}