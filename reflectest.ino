#include <adc.h>
#include <fet.h>
#include <keypad.h>
#include <pressure.h>
#include <servos.h>
#include <temp.h>
#include <timer.h>
#include <ui.h>
#include <util.h>
#include <avr/io.h>  
#include <LiquidCrystal.h>

unsigned int samplePeriod = 1000; // length of time between samples (call to loop())
long unsigned int nextTime = 0;
int lines = 0;

LiquidCrystal lcd(39,41,22,23,24,25); // LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
  pinMode(40,OUTPUT); // LCD R/W pin to ground
  lcd.begin(20, 4); // set up the LCD's number of columns and rows:
  lcd.print("Optifold Cell Tester");
  nextTime = millis() + samplePeriod; // timer initialization
  DoHeartBeatLED();
  delay(1);
  //Specify GCU: (version,fill,psequence)
  //version = V2 or V3 / 2.0 or 3.0
  //fill = FULLFILL | HALFFILL | LITEFILL / full, half, or lite fill boards
  //psequence: for V3.01 boards, check sequence of P sensor part numbers (e.g. MXP7007 or MXP7002, and use last digit). V3.02 boards use P777722:
  // P777222, P222777, P777722
  GCU_Setup(V3,FULLFILL,P777722);
  Kpd_Init();
  ADC_Init();
  Temp_Init();
  Press_Init();
  Fet_Init();
  Servo_Init();
  Timer_Init();

  Kpd_Reset();
  UI_Reset();
  ADC_Reset();
  Temp_Reset();
  Press_Reset();
  Fet_Reset();
  Servo_Reset();
  Timer_Reset();

  Serial.begin(115200);
  analogReference(EXTERNAL); // DEFAULT, INTERNAL, INTERNAL1V1, INTERNAL2V56, or EXTERNAL
  UI_NextScr(); // go to second screen
  UI_NextScr(); // go to third screen
  UI_NextScr(); // go to fourth screen
}

void loop() {
  int key;
  // first, read all KS's sensors
  Temp_ReadAll();  // reads into array Temp_Data[], in 10X oversampled analogReads
  Press_ReadAll(); // reads into array Press_Data[], in hPa
  Timer_ReadAll(); // reads pulse timer into Timer_Data, in RPM ??? XXX

// INSERT USER CONTROL CODE HERE
  if (millis() >= nextTime) {
    nextTime += samplePeriod;
    DoDatalogging();
  }
  
// END USER CONTROL CODE
  key = Kpd_GetKeyAsync();
                    // get key asynnchronous (doesn't wait for a keypress)
                    // returns -1 if no key

  UI_HandleKey(key);  // the other two thirds of the UI routines:
                      // given the key press (if any), then update the internal
                      //   User Interface data structures
                      // ALSO: Manipulate the various output data structures
                      //   based on the keypad input

  Fet_WriteAll();   // Write the FET output data to the PWM hardware
  Servo_WriteAll(); // Write the Futaba hobby servo data to the PWM hardware
        
  PORTJ ^= 0x80;    // toggle the heartbeat LED
  delay(10);
}

void LogTempInputs(boolean header = false) {
  if (header) {
      for (int i= 8; i<16; i++) { // THIS SETS HOW MANY TO PRINT TITLES FOR
        if (Temp_Available[i]) {
          PrintColumnHeader("T",i);
        }
      }
  } else {
    for (int i= 8; i<16; i++) { // THIS SETS HOW MANY THERMOCOUPLES TO PRINT
      if (Temp_Available[i]) {
        PrintColumnInt(Temp_Data[i]);
      }
    }
  }
}

void LogPressureInputs(boolean header = false) {
  if (header) {
      for (int i= 0; i<6; i++) {
        if (Press_Available[i]) {
          PrintColumnHeader("P",i);
        }
      }
  } else {
    for (int i= 0; i<6; i++) {
      if (Press_Available[i]) {
        PrintColumnInt(Press_Data[i]);
      }
    }
  }
}

void LogAnalogInputs(boolean header = false) {
  float analogs[8];
  if (header) {
    for (int i= 0; i<8; i++) {
      if (ANA_available[i] == 1) {
        PrintColumnHeader("ANA",i);
      }
    }
  } else {
    for (int i= 0; i<8; i++) {
      analogs[i] = 0;
      for (int t= 0; t<10; t++) {
        analogs[i] += analogRead(ANA0 + i);
      }
      PrintColumn(analogs[i] / 100.0);
    }
  }
}

void LogSolarData(boolean header = false) {
#define HOWMANY_AN 5
#define VOLTCOEFF (3.998 / 10230) // voltage reference / (10 * 1023)
#define COMPENSATE(x) ((((x) * (3998000l / (100ul * 41ul))) / 1024l) + 25)
  byte mosfets[8] = {5,2,3,6,7,8,46,45};
  float analogs[HOWMANY_AN];
  if (header) {
    for (int i= 0; i<HOWMANY_AN; i++) PrintColumnHeader("ANA",i);
    Serial.print("W*m^2, Therm0, ");
    for (int i= 0; i<4; i++) PrintColumnHeader("Isc",i);
  } else {
    for (int i= 0; i<HOWMANY_AN; i++) {
      analogs[i] = 0;
      for (int t= 0; t<10; t++) analogs[i] += analogRead(ANA0 + i);
      PrintColumn(analogs[i] * VOLTCOEFF);
      lcdPrintFloat(analogs[i] * VOLTCOEFF,i * 5,0); // fifth analog ends up on 3rd line...
    }
    lcdPrint("5th cell volts",5,2); // label fifth analog
    lcdPrintInt((int)Temp_Data[10]*2.01579,0,3);  lcdPrint("Wm2",4,3);
    Serial.print(Temp_Data[10]*2.01579,0);
    Serial.print(", ");
    lcdPrintFloat((float)COMPENSATE((float)Temp_Data[0]/10),9,3);  lcdPrint("C   ",13,3);
    Serial.print((float)COMPENSATE((float)Temp_Data[0]/10),1);
    Serial.print(", ");
    for (int i= 0; i<4; i++) digitalWrite(mosfets[i],HIGH); // turn on shorter MOSFETs
    delay(100);
    Temp_ReadAll();  // reads into array Temp_Data[], in 10X oversampled analogReads
    for (int i= 0; i<4; i++) {
      digitalWrite(mosfets[i],LOW); // turn OFF shorter MOSFETs
      PrintColumn(Temp_Data[i+12] * VOLTCOEFF * 100); // print in milliAmperes
      lcdPrintInt((int)Temp_Data[i+12] * VOLTCOEFF * 100,i * 5,1); // print currents
    }
  }
}

void LogTime(boolean header = false) {
  if (header) {
    PrintColumn("Time");
  } else {
    PrintColumnInt(millis()/1000.0); // time since restart in seconds
  }
}

void lcdPrint(String str, byte col, byte row) {
  lcd.setCursor(col,row);
  lcd.print(str);
  lcd.print("   ");
}
void lcdPrintInt(int str, byte col, byte row) {
  lcd.setCursor(col,row);
  lcd.print(str);
  lcd.print("   ");
}
void lcdPrintFloat(float str, byte col, byte row) {
  lcd.setCursor(col,row);
  lcd.print(str,2);
  lcd.print("   ");
}

void PrintColumnHeader(String str,int n) {
   Serial.print(str);
   Serial.print(n);
   Serial.print(", ");  
}

void PrintColumn(float str) {
   Serial.print(str,2);
   Serial.print(", ");  
}

void PrintColumn(String str) {
   Serial.print(str);
   Serial.print(", "); 
} 

void PrintColumnInt(int str) {
   Serial.print(str);
   Serial.print(", ");  
}

void DoHeartBeatLED() {
  DDRJ |= 0x80;
  PORTJ |= 0x80;
}

void DoDatalogging() {
    boolean header;
    if (lines == 0) {
      header = true;
    } else {
      header = false;
    }
    // Serial output for datalogging
    LogTime(header);
    //LogTempInputs(header);
    //LogPressureInputs(header);
    //LogAnalogInputs(header);
    LogSolarData(header);
    Serial.print("\r\n"); // end of line
    lines++;
}

/* LCD RS pin to digital pin 12     DISPRS  PG2     39
 * LCD Enable to digital pin 11     DISPSTB PG0     41
 * LCD D4 pin to digital pin 5      SCAN0   PA0     22
 * LCD D5 pin to digital pin 4      SCAN1   PA1     23
 * LCD D6 pin to digital pin 3      SCAN2   PA2     24
 * LCD D7 pin to digital pin 2      SCAN3   PA3     25
 * LCD R/W pin to ground            DISPRW  PG1     40  */
