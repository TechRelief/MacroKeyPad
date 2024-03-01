#include <Arduino.h>
#include <BleHidKb.h>
#include <BLEDevice.h>

/// @brief Used to record when a key is pressed down
static bool keyState[10] = {false, false, false, false, false, false, false, false, false, false};

/// @brief hid_keycodes structure is used to store key stroke information and any optional text to output after the keycode has been output; use 0,0,0,0 if the keystroke and text should be ignored and not output
typedef struct TU_ATTR_PACKED
{
  uint8_t modifier;   //Keyboard modifier (KEYBOARD_MODIFIER_* masks). Bit 1 = ctrl, bit 2 = shift, bit 3 = alt, bit 4 = win
  uint8_t keycode;    //Key code to output for the currently pressed key if not zero.
  char text[64]; //If defined text will be output after any keycodes.
} hid_keycodes;

// The HID report codes to output for each Macro-Pad key. The keys are in this order:
/*
+------------------+
| 5 | 6 | 7 | 8 | 9|
+------------------+
| 0 | 1 | 2 | 3 | 4|
+------------------+

For reference, modifier values:
Mod  Ctrl Shft Alt  Win | Mod  Ctrl Shft Alt  Win
0x0:                    | 0x8:                 X
0x1:  X                 | 0x9:  X              X
0x2:       X            | 0xA:        X        X
0x3:  X    X            | 0xB:  X     X        X
0x4:            X       | 0xC:            X    X
0x5:  X         X       | 0xD:  X         X    X
0x6:       X    X       | 0xE:        X   X    X
0x7:  X    X    X       | 0xF:  X     X   X    X
*/
/// @brief keyCodes0 is used when the fnMode=0
static hid_keycodes keyCode0[10][4] =
{
  {
    {0, HID_KEY_END, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 0 - End
  },
  {
    {0, HID_KEY_HOME, 0}, {2, HID_KEY_END, 0}, {0, 0, 0}, {0, 0, 0} //key 1 - Select Line
  },
  {
    {6, HID_KEY_ARROW_DOWN, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //key 2 - Copy Line Down
  },
  {
    {0, HID_KEY_HOME, 0}, {2, HID_KEY_END, 0}, {0, HID_KEY_DELETE, 0}, {0, 0, 0} //key 3 - Delete Line
  },
  {
    {0, HID_KEY_PRINT_SCREEN, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 4 - PrtScrn (used for Screen Snip)
  },
  {
    {0, HID_KEY_HOME, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 5 - Home
  },
  {
    {0, HID_KEY_HOME, 0}, {0, HID_KEY_ARROW_DOWN, 0}, {0, 0, 0}, {0, 0, 0} //Key 6 - Start of next line
  },
  {
    {6, HID_KEY_ARROW_UP, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 7 - Copy line to above
  },
  {
    {0, HID_KEY_END, 0}, {0, HID_KEY_ENTER, 0}, {0, 0, 0}, {0, 0, 0} //Key 8 - Insert Line
  },
  {
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 9 - FN0 (no output, used to switch to second mode)
  }
};

/// @brief keyCodes1 is used when fnMode = 1
static hid_keycodes keyCode1[10][4] =
{
  {
    {1, HID_KEY_END, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //key 0 - Bottom of text
  },
  {
    {6, HID_KEY_A, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 1 - /* */ Comment Toggle
  },
  {
    {1, HID_KEY_H, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //key 2 - Text Replace
  },
  {
    {1, HID_KEY_K, 0}, {1, HID_KEY_X, 0}, {0, 0, 0}, {0, 0, 0} //Key 3 - Trim Trailing Spaces
  },
  {
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //N/A Unused
  },
  {
    {1, HID_KEY_HOME, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 5 - Top of text
  },
  {
    {1, HID_KEY_SLASH, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 6 - Toggle //
  },
  {
    {1, HID_KEY_F, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 7 - Find Text
  },
  {
    {0, HID_KEY_F2}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 8 - Rename Symbol
  },
  {
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //N/A Unused
  }
};

/// @brief keyCodes2 is used when fnMode = 2
static hid_keycodes keyCode2[10][4] =
{
  //Note: Replace "Text-n" and/or key codes with what ever you want the keys to output
  {
    {0, 0, "Text-0"}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //key 0
  },
  {
    {0, 0, "Text-1"}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 1
  },
  {
    {0, 0, "Text-2"}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //key 2
  },
  {
    {0, 0, "Text-3"}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 3
  },
  {
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} // Key 4 - N/A - Not Used (FN3)
  },
  {
    {0, 0, "Text-5"}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 5
  },
  {
    {0, 0, "Text-6"}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 6
  },
  {
    {0, 0, "Text-7"}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 7
  },
  {
    {0, 0, "Text-8"}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //Key 8
  },
  {
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} //N/A Not Used (FN1)
  }
};

/// @brief hidReport is used to output a report via Bluetooth that a HID key is pressed
static hid_keyboard_report_t hidReport = NO_KEY_PRESSED;

/// @brief scans the MacroPad for depressed and released keys and acts accordingly
void scanMacroPad();

/// @brief initializes the MacroPad for use
void initMacroPad();
