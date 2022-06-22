/********************************************************
 NEUTRONA WAND SPEAKER CODE FOR A ARDUINO UNO
 v1.0 (Michael Rajotte) - May 2022.
 // https://github.com/robertsonics/WAV-Trigger-Arduino-Serial-Library

// NOTE: For the wav trigger, you need to solder the 5V connection to enable it (it is located just below the speaker pins). Then you can draw 5V from the Arduino to power it. Also do not use the 12V barrel power input after using the 5V.
// NOTE: You need to edit the wavTrigger.h and make sure you are using __WT_USE_ALTSOFTSERIAL__
// NOTE: AltSoftSerial uses: Pin 9 = TX & Pin 8 = RX. So Pin 9 goes to the RX of the WavTrigger and Pin 8 goes to the TX of the WavTrigger. 
// NOTE: Do not use neopixels with this, as altsoftserial uses timer1, which neopixels interrupts.
********************************************************/
#include <wavTrigger.h>
#include <AltSoftSerial.h>
#include <Metro.h>
#include <millisDelay.h> 
#include <SPI.h>
#include <SD.h>

wavTrigger wTrig;             // Our WAV Trigger object

uint8_t volume = -5;

int rxByte = 0;


void setup() {
  Serial.begin(9600);

  // If the Arduino is powering the WAV Trigger, we should wait for the WAV trigger to finish reset before trying to send commands.
  delay(1000);
  
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
}

void loop() {
    if(Serial.available() > 0) {
      rxByte = Serial.read();

      if(rxByte == 1) {
        boot_up();
      }
      else if(rxByte == 2) {
        shut_down();
      }
      else if(rxByte == 3) {
        firing();
      }
      else if(rxByte == 4) {
        firing_stop();
      }
      
    }
}

void boot_up() {
  if(!wTrig.isTrackPlaying(1) || !wTrig.isTrackPlaying(2)) {
    wTrig.trackGain(1, -3);                // Preset Track 1 gain to -40dB
    wTrig.trackPlayPoly(1);               // Start Track 1

    wTrig.trackGain(3, -20);              // Preset Track gain to -20dB
    wTrig.trackPlayPoly(3);               // Start Track 3
    wTrig.trackFade(3, -1, 2000, 0);       // Fade Track up to 0dB over 2 secs
    wTrig.trackLoop(3, 1);                // Enable track to loop
  }
}

void shut_down() {
  wTrig.stopAllTracks(); 
  wTrig.trackPlayPoly(2);
}

void firing() {
  // Some sparks for firing start.
  wTrig.trackPlayPoly(6);

  // Firing start.
  wTrig.trackPlayPoly(5);

  // Fire main loop.
  wTrig.trackGain(8, -20);              // Preset Track gain to -20dB
  wTrig.trackPlayPoly(8);               // Start Track
  wTrig.trackFade(8, 0, 1000, 0);       // Fade Track up to 0dB over 2 secs
  wTrig.trackLoop(8, 1);                // Enable track to loop
}

void firing_stop() { 
  // Stop all other firing sounds.
  wTrig.trackStop(5);
  wTrig.trackStop(6);
  wTrig.trackStop(7);
  wTrig.trackStop(8);

  wTrig.trackPlayPoly(10);
}
