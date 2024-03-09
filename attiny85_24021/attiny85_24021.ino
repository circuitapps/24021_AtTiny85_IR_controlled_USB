/*
 * 24021: IR remote controlled USB port
 *
 * Description: This code demonstrates how a commercially available remote controller can be repurposed to 
 * control any USB powered device using an AtTiny85 microcontroller using this IR receiver code.
 * Though we are specifically focusing on controlling a USB port for enabling and disabling power in this code,
 * you can easily modify it to control additional USB devices or different devices such as motors, lamps, MP3 players, 
 * and many more!
 *
 * Operation: Though you can use any IR remote controller, we suggest you get a cheap X11TI1
 * (see purchase link: www.amazon.com/dp/B0CG37L55Z), which will be fully compatible with the code
 * here. 
 * 
 * Following are the two button assignments we have used in this code:
 * #define USB_POWER_OFF BUTTON_HASH
 * #define USB_POWER_ON BUTTON_RIGHT
 *
 * Therefore, when you press the hash button on IR remote X11TI1, you should see USB port power turn off.
 * (Be sure to review the circuit diagram in this repository to create the same demo set up we have used for in this project)
 * When you press the RIGHT arrow on the remote controller, the USB power should turn on.
 * 
 * Additional information: If you decide to use an alternative IR remote controller, you will need to first create a
 * table of which button generates which 8-bit code and use that table to allocate your control
 * functions to the buttons of your choice. You can simply connect AtTiny85 (device pin 7) that is
 * programmed to send serial data to a host at 115200 baud to decode any IR transmitter button 
 * presses using this code! WE suggest you use TeraTerm as the serial terminal on your computer
 * to perform this.
 *
 * 
 *  This file is part of Arduino-IRremote https://github.com/Arduino-IRremote/Arduino-IRremote.
 *  Following is the license that applies to all or any part of this software. 
 * 
 * circuitapps, March 2024
 *
 ************************************************************************************
 * MIT License
 *
 * Copyright (c) 2020-2023 Armin Joachimsmeyer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************************
 */

/*
 * Specify which protocol(s) should be used for decoding.
 * If no protocol is defined, all protocols (except Bang&Olufsen) are active.
 * This must be done before the #include <IRremote.hpp>
 */
//#define DECODE_DENON        // Includes Sharp
//#define DECODE_JVC
//#define DECODE_KASEIKYO
//#define DECODE_PANASONIC    // alias for DECODE_KASEIKYO
//#define DECODE_LG
#define DECODE_NEC          // Includes Apple and Onkyo
//#define DECODE_SAMSUNG
//#define DECODE_SONY
//#define DECODE_RC5
//#define DECODE_RC6

//#define DECODE_BOSEWAVE
//#define DECODE_LEGO_PF
//#define DECODE_MAGIQUEST
//#define DECODE_WHYNTER
//#define DECODE_FAST

//#define DECODE_DISTANCE_WIDTH // Universal decoder for pulse distance width protocols
//#define DECODE_HASH         // special decoder for all protocols

//#define DECODE_BEO          // This protocol must always be enabled manually, i.e. it is NOT enabled if no protocol is defined. It prevents decoding of SONY!

//#define DEBUG               // Activate this for lots of lovely debug output from the decoders.

//#define RAW_BUFFER_LENGTH  180  // Default is 112 if DECODE_MAGIQUEST is enabled, otherwise 100.

#include <Arduino.h>

/*
 * This include defines the actual pin number for pins like IR_RECEIVE_PIN, IR_SEND_PIN for many different boards and architectures
 */
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp> // include the library

#define USING_REMOTE_X11TI1  // generic IR remote with 17 keys. Purchase link: www.amazon.com/dp/B0CG37L55Z

#ifdef USING_REMOTE_X11TI1
  #define BUTTON_0 0x19
  #define BUTTON_1 0x45
  #define BUTTON_2 0x46
  #define BUTTON_3 0x47
  #define BUTTON_4 0x44
  #define BUTTON_5 0x40
  #define BUTTON_6 0x43
  #define BUTTON_7 0x07
  #define BUTTON_8 0x15
  #define BUTTON_9 0x09
  #define BUTTON_STAR 0x16
  #define BUTTON_HASH 0x0D
  #define BUTTON_UP 0x18
  #define BUTTON_LEFT 0x08
  #define BUTTON_RIGHT 0x5A
  #define BUTTON_DOWN 0x52
  #define BUTTON_OK 0x1C

  // Demo function to IR command mapping for controller REMOTE_X11TI1
  #define USB_POWER_OFF BUTTON_HASH
  #define USB_POWER_ON BUTTON_RIGHT

#endif

#define USB_CONTROL_PIN PB1

void setup() {
    Serial.begin(115200);  // Only needed if you want to print stuff to serial terminal (e.g., by using TeraTerm) for test & debug
    // Serial TX pin is 2 (PB2 or lead 7 on device) for AtTiny85 device.
    // Connect to computer using serial cable, connect AtTiny85 TX pin (lead 7 on chip) to serial cable RX pin and run TeraTerm application to monitor commands
    // This serial method is also needed if you are trying to find out what an unknown IR transmitter is sending when different buttons on it are pressed!
    
    // Just to know which program is running on my Arduino
    //Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));

    // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);  // Using PB0 with AtTiny85

    Serial.print(F("Ready to receive IR signals of protocols: "));
    printActiveIRProtocols(&Serial);
    Serial.println(F("at pin " STR(IR_RECEIVE_PIN)));

    pinMode(USB_CONTROL_PIN, OUTPUT);

    digitalWrite(USB_CONTROL_PIN, LOW); // USB port power off at start up.
}

void loop() {
    /*
     * Check if received data is available and if yes, try to decode it.
     * Decoded result is in the IrReceiver.decodedIRData structure.
     *
     * E.g. command is in IrReceiver.decodedIRData.command
     * address is in command is in IrReceiver.decodedIRData.address
     * and up to 32 bit raw data in IrReceiver.decodedIRData.decodedRawData
     */
    if (IrReceiver.decode())
    {
        /*
         * Print a short summary of received data
         */
        IrReceiver.printIRResultShort(&Serial);
        IrReceiver.printIRSendUsage(&Serial);
        if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
            Serial.println(F("Received noise or an unknown (or not yet enabled) protocol"));
            // We have an unknown protocol here, print more info
            IrReceiver.printIRResultRawFormatted(&Serial, true);
        }
        Serial.println();

        /*
         * !!!Important!!! Enable receiving of the next value,
         * since receiving has stopped after the end of the current received data packet.
         */
        IrReceiver.resume(); // Enable receiving of the next value

        /*
         * Finally, check the received data and perform actions according to the received command
         */
        if (IrReceiver.decodedIRData.command == USB_POWER_OFF)
        {// TURNS OFF USB PORT POWER
            Serial.println(F("User pressed USB POWER OFF button"));
            digitalWrite(USB_CONTROL_PIN, LOW);
        }
        else if (IrReceiver.decodedIRData.command == USB_POWER_ON)
        {// GENERATES BLUE LED COLOR
            Serial.println(F("User pressed USB POWER ON button"));
            digitalWrite(USB_CONTROL_PIN, HIGH);
        }
        else
        {
           Serial.println(F("Button NOT recognized!"));
           // You can continue adding more button support as above!
        }

    }
}
