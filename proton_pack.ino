/********************************************************
 PROTON PACK CODE FOR A ARDUINO MEGA
 v1.0 (Michael Rajotte) - May 2022.
 // https://github.com/robertsonics/WAV-Trigger-Arduino-Serial-Library

// NOTE: For the wav trigger, you need to solder the 5V connection to enable it (it is located just below the speaker pins). Then you can draw 5V from the Arduino to power it. Also do not use the 12V barrel power input after using the 5V.
// NOTE: You need to edit the wavTrigger.h and comment out define __WT_USE_ALTSOFTSERIAL__ and uncomment out the desired serial port you are going to use. In this case I am using __WT_USE_SERIAL3__ for serial 3.
********************************************************/
#include <wavTrigger.h>
#include <Metro.h>
#include <millisDelay.h> 
#include <SPI.h>
#include <SD.h>
#include <ezButton.h>


#include <BGSequence.h>
BGSequence BarGraph;


// **** Different Bargraph sequence modes **** //
enum BarGraphSequences { START, ACTIVE, FIRE1, FIRE2, BGVENT };
BarGraphSequences BGMODE;
unsigned long currentMillis = millis();
unsigned long CurrentBGInterval = 0;
unsigned long prevBGMillis = 0;


// ******************* Pack Status / State Arrays ******************* //
enum PackState { OFF, BOOT_UP, BOOTING, BOOTED, SHUTDOWN, SHUTTING_DOWN, FIRE_START, FIRING, FIRE_STOP };
enum PackState STATUS;

//#include <HT16K33.h>
//HT16K33 bargraph = HT16K33();
//unsigned long bargraphLastUpdate = 0;

wavTrigger wTrig;             // Our WAV Trigger object

/*
Metro gLedMetro(500);         // LED blink interval timer
Metro gSeqMetro(6000);        // Sequencer state machine interval timer

byte gLedState = 0;           // LED State
int  gSeqState = 0;           // Main program sequencer state
int  gRateOffset = 0;         // WAV Trigger sample-rate offset

int  gNumTracks;              // Number of tracks on SD card

char gWTrigVersion[VERSION_STRING_LEN];    // WAV Trigger version string
*/

int fireSwitchState;
ezButton fireSwitch(6); // create ezButton object that attach to pin 6;

int activateSwitchState;
ezButton activateSwitch(7);

// Volume (0 = loudest, 255 = quietest)
uint8_t volumeR = 50;
uint8_t volumeL = 50;

int packState = 0;

boolean bFiring = false;

void setup() {
  Serial.begin(9600);
  Serial.println(F("Proton Pack Initialising"));

  // This will be our communication to the controller which controls the Neutron Wand Speaker.
  Serial1.begin(9600);
  
  // Serial communication to the wav trigger for controlling our sounds.
  Serial3.begin(57600);

  // Initiate the bargraph.
  //bargraph.init(0x70);
  
   // If the Arduino is powering the WAV Trigger, we should wait for the WAV trigger to finish reset before trying to send commands.
  delay(1000);
  
  //bargraph.setBrightness(10);
  
  // WAV Trigger's startup at 57600
  wTrig.start();
  delay(10);
  
  // Send a stop-all command and reset the sample-rate offset, in case we have
  //  reset while the WAV Trigger was already playing.
  wTrig.stopAllTracks();
  wTrig.samplerateOffset(0); // Reset our sample rate offset        
  wTrig.masterGain(0); // Reset the master gain to 0dB (max volume).
  
  // Enable track reporting from the WAV Trigger
  wTrig.setReporting(true);
  
  // Allow time for the WAV Triggers to respond with the version string and
  //  number of tracks.
  delay(100); 

  /*
  // If bi-directional communication is wired up, then we should by now be able
  //  to fetch the version string and number of tracks on the SD card.
  if (wTrig.getVersion(gWTrigVersion, VERSION_STRING_LEN)) {
      Serial.print(gWTrigVersion);
      Serial.print("\n");
      gNumTracks = wTrig.getNumTracks();
      Serial.print("Number of tracks = ");
      Serial.print(gNumTracks);
      Serial.print("\n");
  }
  else {
      Serial.print("WAV Trigger response not available");
  }
  */

  fireSwitch.setDebounceTime(50); // set debounce time to 50 milliseconds
  activateSwitch.setDebounceTime(50); // set debounce time to 50 milliseconds

  
  BarGraph.BGSeq();            // Initiaze the BarGraph

  // Set the pack status to off.
  STATUS = OFF;
}


void boot_up() {
  if(!wTrig.isTrackPlaying(1)) {
    Serial.println(F("Proton Pack powering up"));

    // Tell the wand to power up.
    Serial1.write(1);

    wTrig.trackGain(1, 0);                // Preset Track 1 gain to -40dB
    wTrig.trackPlayPoly(1);               // Start Track 1

    wTrig.trackGain(3, -20);              // Preset Track gain to -20dB
    wTrig.trackPlayPoly(3);               // Start Track 3
    wTrig.trackFade(3, 0, 2000, 0);       // Fade Track up to 0dB over 2 secs
    wTrig.trackLoop(3, 1);                // Enable track to loop

    prevBGMillis = currentMillis;
  }

          //BarGraph.clearLEDs();
    //BarGraph.initiateVariables(START);
    //BarGraph.sequencePackOn(currentMillis);
    // BarGraph.IntervalBG = 80;
}

/*
// Bargraph modes: 
ACTIVE
START
FIRE1
FIRE2
BGVENT

// Bargraph sequences:
BarGraph.sequenceVent(currentMillis);
BarGraph.sequenceStart(currentMillis);
BarGraph.sequenceShutdown(currentMillis);
BarGraph.sequencePackOn(currentMillis);
BarGraph.sequenceFire1(currentMillis);
BarGraph.sequenceFire2(currentMillis);
*/

void shut_down() {
  Serial.println(F("Proton Pack powering down"));

  // Tell the wand to shut down.
  Serial1.write(2);

  wTrig.stopAllTracks(); 

  if(bFiring == true) {
    // Play fire shutdown sound.
    firing_stop();
    BarGraph.clearLEDs();
    BarGraph.initiateVariables(ACTIVE);
  }
  
  wTrig.trackPlayPoly(2);
}

void fire() {
  bFiring = true;

  // Tell the wand to fire.
  Serial1.write(3);

  // Some sparks for firing start.
  wTrig.trackPlayPoly(6);

  // Firing start.
  wTrig.trackPlayPoly(5);

  // Fire main loop.
  wTrig.trackGain(7, -20);              // Preset Track gain to -20dB
  wTrig.trackPlayPoly(7);               // Start Track
  wTrig.trackFade(7, 0, 1000, 0);       // Fade Track up to 0dB over 2 secs
  wTrig.trackLoop(7, 1);                // Enable track to loop

  BarGraph.clearLEDs();
  BarGraph.initiateVariables(FIRE1);
}

void firing_stop() {
  bFiring = false;  

  // Tell the wand to stop firing.
  Serial1.write(4);
  
  // Stop all other firing sounds.
  wTrig.trackStop(5);
  wTrig.trackStop(6);
  wTrig.trackStop(7);
  wTrig.trackStop(8);

  wTrig.trackPlayPoly(10);
}

void firing() {
  BarGraph.sequenceFire1(currentMillis);
  
  if((currentMillis - prevBGMillis) >= (CurrentBGInterval * 18.75)) {
    prevBGMillis = currentMillis;
  
    if(BarGraph.IntervalBG >= 1)  {
      BarGraph.changeInterval(BarGraph.IntervalBG - 1);
    }
  }
}

void loop() { 
  CurrentBGInterval = BarGraph.IntervalBG;        // Get the current bargraph interval
  currentMillis = millis();
  
 // BarGraph.IntervalBG = 80;
  
  // Must call the loop() function first for the button states.
  activateSwitch.loop();
  fireSwitch.loop();
  
  activateSwitchState = activateSwitch.getState();
  fireSwitchState = fireSwitch.getState();

  switch (STATUS) {
    case OFF:
      BarGraph.sequenceShutdown(currentMillis);
      break;

    case BOOT_UP:
      boot_up();
      
      BarGraph.initiateVariables(START);
      BarGraph.sequenceStart(currentMillis);
      
      STATUS = BOOTING;
      break;
      
    case BOOTING:
      BarGraph.sequenceStart(currentMillis);
      
      //if(!wTrig.isTrackPlaying(1)) {
      if(currentMillis - prevBGMillis > 3000) {       
        STATUS = BOOTED;
      }

      break;

    case BOOTED:
      BarGraph.IntervalBG = 30;

      BarGraph.sequenceStart(currentMillis);
      //BarGraph.sequencePackOn(currentMillis);

      break;
    
    case SHUTDOWN:
      BarGraph.IntervalBG = 80;
      BarGraph.sequenceShutdown(currentMillis);

      shut_down();

      STATUS = SHUTTING_DOWN;
      break;

    case SHUTTING_DOWN:
      BarGraph.sequenceShutdown(currentMillis);
      
      if(!wTrig.isTrackPlaying(2)) {
        STATUS = OFF;
      }
      break;
    
    case FIRING:
      firing();
      break;

    case FIRE_START:
      fire();
      STATUS = FIRING;
      break;

    case FIRE_STOP:
      firing_stop();

      BarGraph.clearLEDs();
      BarGraph.initiateVariables(ACTIVE);

      STATUS = BOOTED;
      break;
  }

  
  if(activateSwitchState == LOW && STATUS == OFF) {
    // Bootup / idle state.
    STATUS = BOOT_UP; 
  }
  else if(activateSwitchState == HIGH && STATUS != OFF && STATUS != SHUTDOWN && STATUS != SHUTTING_DOWN) {
    // Shutdown pack.
    STATUS = SHUTDOWN;
  }

  if(fireSwitchState == LOW && STATUS != OFF && STATUS != SHUTDOWN && STATUS != SHUTTING_DOWN && bFiring == false) {
    STATUS = FIRE_START;
  }
  else if(fireSwitchState == HIGH && bFiring == true) {
    STATUS = FIRE_STOP;
  }
}
