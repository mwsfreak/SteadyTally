/* ----------- SteadyTallyBase.ino -------------

Version 1.0

Created 29-06-2020 by Magnus Bisgård Franks

This file is a modification of the file ATEM_Tally_Transmitter.ino from the ATEM_Wireless_Tally_Light repository
  (https://github.com/kalinchuk/ATEM_Wireless_Tally_Light)

   -------------- End of header ----------------
*/

#include <SPI.h>
#include <Ethernet.h>
#include <TextFinder.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>
#include <ATEMstd.h>
#include <ATEMTally_Steady.h>
#include <RadioLib.h>

// set the default MAC address
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0F, 0x1B, 0x82};

// set the default IP address
byte ip[] = {192, 168, 0, 203};

// set the default IP address of the ATEM switcher
byte switcher_ip[] = {192, 168, 0, 201};

// set the default PORT of the ATEM switcher
long switcher_port = 49910;

// initialize the ethernet server (for settings page)
EthernetServer server(80);

// set to false initially; set to true when ATEM switcher initializes
boolean ranOnce = false;

// create a new AtemSwitcher and ATEMTally object
ATEMstd AtemSwitcher;
ATEMTally_Steady ATEMTally;

// Define variables for determining whether tally is on
boolean programOn;
boolean previewOn;

// define a structure for sending over the radio
struct {
  byte program_1;
  byte program_2;
  byte preview;
} payload;

/*
// ---------------------- Radio Initialization -----------------------
// SX1278 has the following connections:
// NSS pin:   7     (SPI chip-select)
// DIO0 pin:  2     (RxDone/TxDone)
// RESET pin: A0    
// DIO1 pin:  3     (RXtimeout) (optional

RFM98 radio = new Module(7, 2, 9);
//RFM98 radio = new Module(7, 2, 9, 3);

// save transmission state between loops
int transmissionState = ERR_NONE;

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

// this function is called when a complete packet
// is transmitted by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // check if the interrupt is enabled
  if(!enableInterrupt) {
    return;
  }

  // we sent a packet, set the flag
  transmittedFlag = true;
}

// ------------------- End of Radio Initialization -------------------
*/

// ------------------- LED-shield driver for testing ATEM-connection -------------------

//Define pins for LED-shield
int PreviewLed[] = {2, 3, 5, 6, 7};
int ProgramLed[] = {A4, A3, A2, A1, A0};

long lastTime = 0;
int incomingByte = 0;

void initLEDshield() {
  for (int i = 0; i < 5; i++) {
    pinMode(PreviewLed[i], OUTPUT);
    pinMode(ProgramLed[i], OUTPUT);
  }
}

void updateLEDs() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(PreviewLed[i], LOW);
    digitalWrite(ProgramLed[i], LOW);
  }
  
  digitalWrite(PreviewLed[payload.preview], HIGH);
  digitalWrite(ProgramLed[payload.program_1], HIGH);
}

// ------------------------------ End of LED-shield driver ------------------------------

void setup()
{
  /*
  Serial.begin(9600);
  
  // initialize the RFM9x radio with default settings
  Serial.print(F("[RFM98] Initializing ... "));
  int state = radio.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set the function that will be called
  // when packet transmission is finished
  radio.setDio0Action(setFlag);

  // start transmitting the first packet
  Serial.print(F("[RFM98] Sending first packet ... "));
  transmissionState = radio.startTransmit(payload);

  */
  
  // initialize the ATEMTally object
  ATEMTally.initialize(9, 9, 9, 8); //Tally status pins: Red LED, Green LED, Blue LED, EEProm reset pin
  
  // set the LED to RED
  ATEMTally.change_LED_state(2);
  
  // setup the Ethernet
  ATEMTally.setup_ethernet(mac, ip, switcher_ip, switcher_port);
  
  // start the server
  server.begin();

  //Init LED test shield
  initLEDshield();                              // --------- LED TEST ---------

  //Init serial
  Serial.begin(9600);                           // --------- SERIAL TEST ---------
  
  // delay a bit before looping
  delay(1000);
}

void loop()
{
  
  // run this code only once
  if (!ranOnce) {
    // initialize the AtemSwitcher
    AtemSwitcher.begin(IPAddress(switcher_ip[0], switcher_ip[1], switcher_ip[2], switcher_ip[3]), switcher_port);    
    
    // attempt to connect to the switcher
    AtemSwitcher.connect();

    // change LED to RED
    ATEMTally.change_LED_state(2);
    
    ranOnce = true;
  }

  // display the setup page if requested
  EthernetClient client = server.available();
  ATEMTally.print_html(client, mac, ip, switcher_ip, switcher_port);

  // AtemSwitcher function for retrieving the program and preview camera numbers
  AtemSwitcher.runLoop();

  // if connection is gone anyway, try to reconnect
  if (AtemSwitcher.hasInitialized())  {
    // Reset the previous program and preview numbers
    payload.program_1 = 0;
    payload.program_2 = 0;
    payload.preview = 0;

    // assign the program and preview numbers to the payload structure
    for (int i = 1; i <= 16; i++) {
      programOn = AtemSwitcher.getProgramTally(i);
      previewOn = AtemSwitcher.getPreviewTally(i);

      if (programOn) {
        if (payload.program_1 == 0) {
          payload.program_1 = i;
        } else {
          payload.program_2 = i;
        }
      }

      if (previewOn) {
        payload.preview = i;
      }
    }
  }

/*
  // when radio is available, transmit the structure with program and preview numbers
  // check if the previous transmission finished
  if(transmittedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;

    // reset flag
    transmittedFlag = false;

    if (transmissionState == ERR_NONE) {
      // packet was successfully sent
      Serial.println(F("transmission finished!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(transmissionState);
    }

    // wait a second before transmitting again - uncomment if necessary
    //delay(1000);

    // send new packet
    Serial.print(F("[SX1278] Sending another packet ... "));
    transmissionState = radio.startTransmit(payload);

    ATEMTally.change_LED_state(3);
    
    // we're ready to send more packets,
    // enable interrupt service routine
    enableInterrupt = true;
  }
*/

/*
  // ------ TEST: Increment tally every second ------
  
  if (millis() - lastTime > 1000) {
    if (payload.preview < 4) {
      payload.preview++;
    } else {
      payload.preview = 0;
    }

    if (payload.program_1 < 4) {
      payload.program_1++;
    } else {
      payload.program_1 = 0;
    }
    
    lastTime = millis();
  }

  // ------ END OF TEST ------
*/

/**/
  // ------ TEST: Control tally from serial prompt ------
  // Preview: z-b    Program: a-g 
  
  if (Serial.available()) {
    incomingByte = Serial.read();

    switch (incomingByte) {
      case 'a':
        payload.program_1 = 0;
        break;
      case 's':
        payload.program_1 = 1;
        break;
      case 'd':
        payload.program_1 = 2;
        break;
      case 'f':
        payload.program_1 = 3;
        break;
      case 'g':
        payload.program_1 = 4;
        break;
      case 'z':
        payload.preview = 0;
        break;
      case 'x':
        payload.preview = 1;
        break;
      case 'c':
        payload.preview = 2;
        break;
      case 'v':
        payload.preview = 3;
        break;
      case 'b':
        payload.preview = 4;
        break;
    }
  }
  // ------ END OF TEST ------
/**/

  updateLEDs();                         // --------- LED TEST ---------
  
  //OPTIMIZE THIS
  // a delay is needed due to some weird issue
  delay(10);

  ATEMTally.change_LED_state(1);

  // monitors for the reset button press
  ATEMTally.monitor_reset();
  
}
