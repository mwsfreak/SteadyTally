/* ----------- SteadyTallyNode.ino -------------

Created 29-06-2020 by Magnus Bisgård Franks

This file is a modification of the file ATEM_Tally_Receiver from the ATEM_Wireless_Tally_Light repo 
  (https://github.com/kalinchuk/ATEM_Wireless_Tally_Light)

*/
#include <RadioLib.h>

// DEBUG
const byte TALLY_DEBUG = 1;

// ---------------------- Radio Initialization -----------------------
// SX1278 has the following connections:
// NSS pin:   10     (SPI chip-select)
// DIO0 pin:  2     (RxDone/TxDone)
// RESET pin: A0    
// DIO1 pin:  3     (RXtimeout) (optional)

RFM98 radio = new Module(10, 2, A0);
//RFM98 radio = new Module(7, 2, 9, 3);

// ------------------- End of Radio Initialization -------------------

// RED LED pin
const byte RED_PIN = 3;

// GREEN LED pin
const byte GREEN_PIN = 5;

// BLUE LED pin
const byte BLUE_PIN = 6;

// PROGRAM LED pin
const byte PROGRAM_PIN = RED_PIN;

// PREVIEW LED pin
const byte PREVIEW_PIN = GREEN_PIN;

// NO SIGNAL pin
const byte ERROR_PIN = BLUE_PIN;

// POWER LED pin (blinks the node # on power up)
const byte POWER_PIN = A1;

// create an array of DIP pins (LSB -> MSB)
const int dipPins[] = {4, 7, 8, 9};

// last received radio signal time (to power off LEDs when no signal)
unsigned long last_radio_recv = 0;

// default Node # 200 (alias to 0) will blink constantly if signal exists
int this_node = 200;
int new_node;

// define array for receiving over the radio
//   Preview   => payload[0]
//   Program 1 => payload[1]
//   Program 2 => payload[2]
byte payload[3];

byte preview, program_1, program_2 = 0;

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // check if the interrupt is enabled
  if(!enableInterrupt) {
    return;
  }

  // we got a packet, set the flag
  receivedFlag = true;
}

// this function reads the DIP switches 
// and determines the Node #
byte setNodeID(int* dipPins, int numPins) {
  float j=0;
  
  for(int i=0; i < numPins; i++) {
    if (digitalRead(dipPins[i]) == 0) {
      j += pow(2, i);
    }
  }

  new_node = (int)(j + 0.5);
  
  // if the DIP switches are all OFF, assign 200 (alias to 0) to the Node #
  if (new_node == 0) {
    new_node = 200;
  }
  
  if (this_node != new_node) {
    this_node = new_node;
    
    Serial.print("\nNODE #: ");
    Serial.print(this_node);
  }
}

void setup() {
	// initialize all the defined pins
	pinMode(PROGRAM_PIN, OUTPUT);
	pinMode(PREVIEW_PIN, OUTPUT);
  pinMode(ERROR_PIN, OUTPUT);
	pinMode(POWER_PIN, OUTPUT);
	pinMode(dipPins[0], INPUT_PULLUP);
	pinMode(dipPins[1], INPUT_PULLUP);
	pinMode(dipPins[2], INPUT_PULLUP);
	pinMode(dipPins[3], INPUT_PULLUP);

	// turn all LEDs off
  analogWrite(PROGRAM_PIN, 0);
  analogWrite(PREVIEW_PIN, 0);
  analogWrite(ERROR_PIN, 0);
  digitalWrite(POWER_PIN, LOW);  
 
	// set the Node # according to the DIP pins
  setNodeID(dipPins, 4);
  
	if (this_node == 200) {
		// if the Node # is 0 (alias to 200), blink quickly (30 times) on power on
		for (int i=0; i < 30; i++) {
			digitalWrite(POWER_PIN, HIGH);
			delay(100);
			digitalWrite(POWER_PIN, LOW);
			delay(100);
		}
	} else {
		// if the Node # is a set number, blink the Node # on power on
		for (int i=0; i < this_node; i++) {
			digitalWrite(POWER_PIN, HIGH);
			delay(300);
			digitalWrite(POWER_PIN, LOW);
			delay(300);
		}
	}

	// set this variable to current time
  last_radio_recv = millis();

   Serial.begin(9600);

  // initialize RFM98 with default settings
  Serial.print(F("[RFM98] Initializing ... "));
  int state = radio.begin(450.0, 125, 9, 7, SX127X_SYNC_WORD, 10, 8, 0);
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set the function that will be called
  // when new packet is received
  radio.setDio0Action(setFlag);

  // start listening for LoRa packets
  Serial.print(F("[RFM98] Starting to listen ... "));
  state = radio.startReceive();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
  
  // if needed, 'listen' mode can be disabled by calling
  // any of the following methods:
  //
  // radio.standby() 
  // radio.sleep()
  // radio.transmit();
  // radio.receive();
  // radio.readData();
  // radio.scanChannel();

  //Turn on power led when initializing is finished
  digitalWrite(POWER_PIN, HIGH);
}

void loop() {
  if(receivedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;

    // reset flag
    receivedFlag = false;

    // read received data as byte array
    int state = radio.readData(payload, 3);

    if (state == ERR_NONE) {
      // a radio signal was received
      
      // get the program and preview numbers
      preview = payload[0];
      program_1 = payload[1];
      program_2 = payload[2];

      if (this_node == 200) {
        // if the Node # is 200 (which is also 0), blink POWER LED every 1 second if signal exists
        digitalWrite(POWER_PIN, HIGH);
        delay(250);
        digitalWrite(POWER_PIN, LOW);
        delay(750);
      } else {
        // if the Node # is a set number, trigger an LED accordingly
        if (program_1 == this_node || program_2 == this_node) {
          analogWrite(PREVIEW_PIN, 0);
          analogWrite(PROGRAM_PIN, 50);
        } else if (preview == this_node) {
          analogWrite(PREVIEW_PIN, 50);
          analogWrite(PROGRAM_PIN, 0);
        } else {
          analogWrite(PREVIEW_PIN, 0);
          analogWrite(PROGRAM_PIN, 0);
        }
      }
    
      // keep track of last radio signal time
      last_radio_recv = millis();

      // packet was successfully received
      Serial.println(F("[RFM98] Received packet!"));

      if (TALLY_DEBUG >= 1) {
        // print data of the packet
        Serial.print(F("[RFM98] Data:\t\t"));
        Serial.print(F("Preview: "));
        Serial.print(preview);
        Serial.print(F("\tProgram 1: "));
        Serial.print(program_1);
        Serial.print(F("\tProgram 2: "));
        Serial.print(program_2);

        if (TALLY_DEBUG >= 2) {
        // print RSSI (Received Signal Strength Indicator)
        Serial.print(F("[RFM98] RSSI:\t\t"));
        Serial.print(radio.getRSSI());
        Serial.println(F(" dBm"));
  
        // print SNR (Signal-to-Noise Ratio)
        Serial.print(F("[RFM98] SNR:\t\t"));
        Serial.print(radio.getSNR());
        Serial.println(F(" dB"));

        // print frequency error
        Serial.print(F("[RFM98] Frequency error:\t"));
        Serial.print(radio.getFrequencyError());
        Serial.println(F(" Hz"));
        }
      }

    } else if (state == ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("[RFM98] CRC error!"));

    } else {
      // some other error occurred
      Serial.print(F("[RFM98] Failed, code "));
      Serial.println(state);
    }

    // put module back to listen mode
    radio.startReceive();

    // we're ready to receive more packets,
    // enable interrupt service routine
    enableInterrupt = true;
  }

  
	// turn off LEDs when no radio signal exists (the past 1 second)
	if (millis() - last_radio_recv > 1000) {
		analogWrite(PREVIEW_PIN, 0);
		analogWrite(PROGRAM_PIN, 0);
		analogWrite(ERROR_PIN, 50);
	} else {
    analogWrite(ERROR_PIN, 0);
	}

  // Update Node # according to the DIP pins
  setNodeID(dipPins, 4);
}
