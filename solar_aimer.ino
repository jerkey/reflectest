/*  solar_aimer by jake sternberg 2013-6-28
    this program runs on an atmega328 or similar, to monitor voltages
    of four solar cells connected to FETs driven by PWM channels.

    Analog channels A0-A3 measure NWSE cells' voltage (positive terminal,
    their minus terminal is grounded)

    Analog channels A4-A7 measure 1-ohm current-sensing resistors with a
    gain of 3.0303 (1Mohm/330Kohm) which are connected between the cells'
    positive terminal and the drain of N-channel FETs

    The N-channel FETs' sources are grounded.  Their gates are connected
    to a 0.1uF capacitor to ground, and a 1K resistor to digital pins 3,
    5, 6 and 11 (North West South East in that order for everything)
    This way, those PWM channels can control the gate voltages precisely.

    servos to aim the assembly sunward are connected to digital pins 9
    and 10.  the servo on pin 9 aims North/South and pin 10 East/West.
    The motors aim the assembly toward the sun to maximize solar collection.
    
*/

#define VOLT_N A0
#define VOLT_W A1
#define VOLT_S A2
#define VOLT_E A3
#define CURRENT_N A4
#define CURRENT_W A5
#define CURRENT_S A6
#define CURRENT_E A7
#define LOAD_N 3
#define LOAD_W 5
#define LOAD_S 6
#define LOAD_E 11
#define V_COEFF 123.34 // divide ADC reading by this to get voltage
#define I_COEFF 123.34 // divide ADC reading by this to get current

#define EWNULL 3035  // eastwest midpoint
#define NSNULL 3000  // northsouth midpoint
#define EWRANGE 2000 // eastwest range away from midpoint
#define NSRANGE 1000 // northsouth range away from midpoint
#define EWCONTROL OCR1A       // eastwest servo is connected to Arduino pin9
#define NSCONTROL OCR1B     // northsouth servo is connected to Arduino pin10
#define NSPIN 9 // pin number for northsouth servo
#define EWPIN 10 // pin number for eastwest servo

#define TRACKEWTIME 100  // time between eastwest tracking calls
#define TRACKNSTIME 400  // time between northsouth tracking calls
#define PRINTTIME 500     // time between printing display
#define MPPTTIME 25     // time between load tracking calls

float volt_n,volt_w,volt_s,volt_e,current_n,current_w,current_s,current_e;
int pwmval_n,pwmval_w,pwmval_s,pwmval_e = 0; // what we last sent to analogWrite

int EWVector,NSVector = 0;  // direction and speed of change for tracking
int EW,NS,Buck = 0; // position value of eastwest motor, northsouth motor, and buck converter

void setup() {
  Serial.begin(57600);
  pinMode(EWPin,OUTPUT);
  pinMode(NSPin,OUTPUT);
  pinMode(LOAD_N,OUTPUT);
  pinMode(LOAD_W,OUTPUT);
  pinMode(LOAD_S,OUTPUT);
  pinMode(LOAD_E,OUTPUT);
  
  TCCR1B = 0b00011010;        // Fast PWM, top in ICR1, /8 prescale (.5 uSec)
  TCCR1A = 0b10100010;        // clear on compare match, fast PWM
  ICR1 =  39999;              // 40,000 clocks @ .5 uS = 20mS
  EWControl = EWNull;         // controls chip pin 15  ARDUINO pin 9
  NSControl = NSNull;         // controls chip pin 16  ARDUINO pin 10
}                             // valid range is servonull +/- 1000

unsigned long timenow, lastNS, lastEW, lastMPPT, lastPrint= 0; // keep track of time

void loop() {
  timenow = millis();  // get the system time for this instance of loop()
  getVoltages();  // measure all voltages and current wattage
  
  if (timenow - lastPrint > PRINTTIME) { // run only one of these tracks per loop cycle
    printDisplay();
    lastPrint = timenow;
  } else if (timenow - lastEW > TRACKEWTIME) {
    trackEW();
    lastEW = timenow;
  } else if (timenow - lastNS > TRACKNSTIME) {
    trackNS();
    lastNS = timenow;
  } else if (timenow - lastMPPT > TRACKNSTIME) {
    trackMPPT();
    lastMPPT = timenow;
  }
}

void printDisplay() {
  Serial.print("N:");
  Serial.print(volt_n,1);
  Serial.print("V  ");
  Serial.print(current_n,1);
  Serial.print("A  W:");
  Serial.print(volt_w,1);
  Serial.print("V  ");
  Serial.print(current_w,1);
  Serial.print("A  S:");
  Serial.print(volt_s,1);
  Serial.print("V  ");
  Serial.print(current_s,1);
  Serial.print("A  E:");
  Serial.print(volt_e,1);
  Serial.print("V  ");
  Serial.print(current_e,1);
  Serial.print("A  EW:");
  Serial.print(EWControl);
  Serial.print("  NS:");
  Serial.println(NSControl);
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
void trackMPPT() {
  /*  does calling analogWrite actually reset the clock? yes
      maybe we have to write the register (for pin11) directly
      for best results.  But we should always be tuning the buck converter for MPPT
  */
}


void getVoltages() {
  volt_n = (analogRead(VOLT_N) / V_COEFF);
  volt_w = (analogRead(VOLT_W) / V_COEFF);
  volt_s = (analogRead(VOLT_S) / V_COEFF);
  volt_e = (analogRead(VOLT_E) / V_COEFF);
  current_n = (analogRead(CURRENT_N) / I_COEFF);
  current_w = (analogRead(CURRENT_W) / I_COEFF);
  current_s = (analogRead(CURRENT_S) / I_COEFF);
  current_e = (analogRead(CURRENT_E) / I_COEFF);
}

