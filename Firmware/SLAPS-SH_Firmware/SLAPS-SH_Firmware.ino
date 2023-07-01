#include "src/MS5607SLAPS.h"
#include <SingleFileDrive.h>
#include <LittleFS.h>

uint32_t cnt = 0;
bool okayToWrite = true;
MS5607 P_Sens;
float P_val,T_val,H_val;

// Make the CSV file and give it a simple header
void headerCSV() {
  File f = LittleFS.open("data.csv", "w");
  f.printf("sample,millis,temp,pressure,alt\n");
  f.close();
  cnt = 0;
}

// Called when the USB stick connected to a PC and the drive opened
// Note this is from a USB IRQ so no printing to SerialUSB/etc.
void plug(uint32_t i) {
  (void) i;
  okayToWrite = false;
}

// Called when the USB is ejected or removed from a PC
// Note this is from a USB IRQ so no printing to SerialUSB/etc.
void unplug(uint32_t i) {
  (void) i;
  okayToWrite = true;
}

// Called when the PC tries to delete the single file
// Note this is from a USB IRQ so no printing to SerialUSB/etc.
void deleteCSV(uint32_t i) {
  (void) i;
  LittleFS.remove("data.csv");
  headerCSV();
}

void setup(void){

  LittleFS.begin();

  // Set up the USB disk share
  singleFileDrive.onDelete(deleteCSV);
  singleFileDrive.onPlug(plug);
  singleFileDrive.onUnplug(unplug);
  singleFileDrive.begin("data.csv", "data.csv");

  // Find the last written data
  File f = LittleFS.open("data.csv", "r");
  if (!f || !f.size()) {
    cnt = 1;
    headerCSV();
  } else {
    if (f.size() > 2048) {
      f.seek(f.size() - 1024);
    }
    do {
      String s = f.readStringUntil('\n');
      sscanf(s.c_str(), "%lu,", &cnt);
    } while (f.available());
    f.close();
    cnt++;
  }

  P_Sens.begin();
}

void loop(void){
  if (P_Sens.readDigitalValue()) {
    T_val = P_Sens.getTemperature();
    P_val = P_Sens.getPressure();
    H_val = P_Sens.getAltitude();

    // Make sure the USB connect doesn't happen while we're writing!
    noInterrupts();
    if (okayToWrite) {
      Serial.printf("Sampling...%lu\n", cnt);
      // Don't want the USB to connect during an update!
      File f = LittleFS.open("data.csv", "a");
      if (f) {
        f.printf("%lu,%lu,%f,%f,%f\n", cnt++, millis(), T_val, P_val, H_val);
        f.close();
      }
    }
    interrupts();
  }
  
  

  delay(1000);
}
