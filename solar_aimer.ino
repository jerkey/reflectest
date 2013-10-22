/*  solar_aimer by jake sternberg 2013-6-28
    this program runs on an atmega328 or similar, to monitor voltages
    of four solar cells connected in series, arranged in the four cardinal
    directions.  It controls motors to aim the assembly toward the sun to
    maximize solar collection.
    
*/

#define c1p A1
#define c2p A2
#define c3p A3
#define c4p A4
#define c1coeff 13.857 // divide ADC reading by this to get voltage
#define c2coeff 13.857
#define c3coeff 13.857
#define c4coeff 13.857
#define iPin A5 // current sensor
#define iCoeff 123.34 // divide ADC reading by this to get current
#define northVoltage c1voltage  // change these to match cell wiring pattern
#define southVoltage c2voltage
#define eastVoltage c3voltage
#define westVoltage c4voltage

#define EWNull 3035  // eastwest midpoint
#define NSNull 3000  // northsouth midpoint
#define EWRange 2000 // eastwest range away from midpoint
#define NSRange 1000 // northsouth range away from midpoint
#define EWControl OCR1A       // eastwest servo is connected to Arduino pin9
#define NSControl OCR1B     // northsouth servo is connected to Arduino pin10
#define EWPin 9  // pin number for eastwest servo
#define NSPin 10 // pin number for northsouth servo
#define buckPin 11 // pin for DC-DC buck converter

#define trackEWTime 100  // time between eastwest tracking calls
#define trackNSTime 400  // time between northsouth tracking calls
#define trackBuckTime 25     // time between buck converter tracking calls

float c1voltage,c2voltage,c3voltage,c4voltage,current,wattage,lastWattage = 0;

int EWVector,NSVector,buckVector = 0;  // direction and speed of change for tracking
int EW,NS,Buck = 0; // position value of eastwest motor, northsouth motor, and buck converter

void setup() {
  Serial.begin(57600);
  pinMode(EWPin,OUTPUT);
  pinMode(NSPin,OUTPUT);
  pinMode(buckPin,OUTPUT);
  
  DON'T FORGET TO MAKE BUCKPIN GO FAST!
  
  TCCR1B = 0b00011010;        // Fast PWM, top in ICR1, /8 prescale (.5 uSec)
  TCCR1A = 0b10100010;        // clear on compare match, fast PWM
  ICR1 =  39999;              // 40,000 clocks @ .5 uS = 20mS
  EWControl = EWNull;         // controls chip pin 15  ARDUINO pin 9
  NSControl = NSNull;         // controls chip pin 16  ARDUINO pin 10
}                             // valid range is servonull +/- 1000

unsigned long timenow, lastNS, lastEW, lastBuck = 0; // keep track of time

void loop() {
  timenow = millis();  // get the system time for this instance of loop()
  getVoltages();  // measure all voltages and current wattage
  
  if (timenow - lastBuck > trackBuckTime) { // run only one of these tracks per loop cycle
    trackBuck();
    lastBuck = timenow;
  } else if (timenow - lastEW > trackEWTime) {
    trackEW();
    lastEW = timenow;
  } else if (timenow - lastNS > trackNSTime) {
    trackNS();
    lastNS = timenow;
  }


}

void printDisplay() {
  Serial.print(c4voltage,1);
  Serial.print(" Volts, ");
  Serial.print(current);
  Serial.print(" Amps, ");
  Serial.print(EWControl);
  Serial.print(" EW, ");
  Serial.print(NSControl);
  Serial.println(" NS");
}

void trackNS() {
/*  compare north and south voltages, with remembered coefficient (compare north*NSCoeff to south) 
    and only track if the difference is more than a certain factor.  
    or if the total production of power (into sufficient load?) is too low, maybe we need to seek or do a random walk
*/
}
void trackEW() {
/*  compare north and south voltages, with remembered coefficient (compare north*NSCoeff to south) 
    and only track if the difference is more than a certain factor.  
    or if the total production of power (into sufficient load?) is too low, maybe we need to seek or do a random walk
*/}
void trackBuck() {
  /*  does calling analogWrite actually reset the clock?
      maybe we have to write the register (for pin11) directly
      for best results.  But we should always be tuning the buck converter for MPPT
  */
}


void getVoltages() {
  c1voltage = analogRead(c1p) / c1coeff;
  c2voltage = (analogRead(c2p) / c2coeff) - c1voltage;
  c3voltage = (analogRead(c3p) / c3coeff) - c2voltage;
  c4voltage = (analogRead(c4p) / c4coeff) - c3voltage;
  current = analogRead(iPin) / iCoeff;
  lastWattage = wattage;
  wattage = current * c4voltage;
}

