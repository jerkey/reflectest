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

#define EWNULL 90  // eastwest midpoint
#define NSNULL 70  // northsouth midpoint
#define EWRANGE 80 // eastwest range away from midpoint
#define NSRANGE 60 // northsouth range away from midpoint
#define NSPIN 9 // pin number for northsouth servo
#define EWPIN 10 // pin number for eastwest servo

#define TRACKEWTIME 100  // time between eastwest tracking calls
#define TRACKNSTIME 400  // time between northsouth tracking calls
#define PRINTTIME 500     // time between printing display
#define MPPTTIME 25     // time between load tracking calls

float volt_n,volt_w,volt_s,volt_e,current_n,current_w,current_s,current_e,watt_n,watt_w,watt_s,watt_e;
int pwmval_n,pwmval_w,pwmval_s,pwmval_e = 0; // what we last sent to analogWrite

int EWVector,NSVector = 0;  // direction and speed of change for tracking
int EW,NS,Buck = 0; // position value of eastwest motor, northsouth motor, and buck converter

#include <Servo.h>
servo ewServo,nsServo; // create servo objects

void setup() {
  Serial.begin(57600);
  ewServo.attach(EWPin);
  nsServo.attach(NSPin);
  pinMode(LOAD_N,OUTPUT);
  pinMode(LOAD_W,OUTPUT);
  pinMode(LOAD_S,OUTPUT);
  pinMode(LOAD_E,OUTPUT);
}

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
  static float watt_last_n,watt_last_w,watt_last_s,watt_last_e = 0;
  static int vector_n,vector_w,vector_s,vector_e = 1; // which direction we're changing pwmval this time
  static int pwmval_n,pwmval_w,pwmval_s,pwmval_e = 0; // what we last sent to analogWrite
  if (watt_last_n > watt_n) vector_n *= -1; // if we had more power last time, change direction
  if (watt_last_w > watt_w) vector_w *= -1;
  if (watt_last_s > watt_s) vector_s *= -1;
  if (watt_last_e > watt_e) vector_e *= -1;
  pwmval_n += vector_n; // change (up or down) PWM value
  pwmval_w += vector_w;
  pwmval_s += vector_s;
  pwmval_e += vector_e;
  if (pwmval_n > 254) vector_n = -1;
  if (pwmval_w > 254) vector_w = -1;
  if (pwmval_s > 254) vector_s = -1;
  if (pwmval_e > 254) vector_e = -1;
  if (pwmval_n < 1) vector_n = 1;
  if (pwmval_w < 1) vector_w = 1;
  if (pwmval_s < 1) vector_s = 1;
  if (pwmval_e < 1) vector_e = 1;
  watt_last_n = watt_n; // store previous cycle's data
  watt_last_w = watt_w;
  watt_last_s = watt_s;
  watt_last_e = watt_e;
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
  watt_n = volt_n * current_n;
  watt_w = volt_w * current_w;
  watt_s = volt_s * current_s;
  watt_e = volt_e * current_e;
}

