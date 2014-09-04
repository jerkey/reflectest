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

#define N 0 // for arrays
#define W 1
#define S 2
#define E 3
const String nwse = "NWSE"; // for printing info
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
#define MPPTTIME 50     // time between load tracking calls

float voltage[4],current[4],wattage[4] = {0};
float nwseWattAdder[4],MPPTWattAdder[4],printWattAdder[4] = {0}; // for averaging wattages for trackers
int nsWattAdds,ewWattAdds,MPPTWattAdds,printWattAdds = 0; // how many times adder was added

int pwmval[4] = {0}; // what we last sent to analogWrite

int EWVector,NSVector = 0;  // direction and speed of change for tracking
int EW,NS,Buck = 0; // position value of eastwest motor, northsouth motor, and buck converter

#include <Servo.h>
Servo ewServo,nsServo; // create servo objects

void setup() {
  Serial.begin(57600);
  ewServo.attach(EWPIN);
  nsServo.attach(NSPIN);
  pinMode(LOAD_N,OUTPUT);
  pinMode(LOAD_W,OUTPUT);
  pinMode(LOAD_S,OUTPUT);
  pinMode(LOAD_E,OUTPUT);
}

unsigned long timenow, lastNS, lastEW, lastMPPT, lastPrint= 0; // keep track of time

void loop() {
  timenow = millis();  // get the system time for this instance of loop()
  getVoltages();  // measure all voltages and current wattage

  for (int dir = 0; dir < 4; dir++) { // for averaging
    nwseWattAdder[dir] += wattage[dir];
    MPPTWattAdder[dir] += wattage[dir];
    printWattAdder[dir] += wattage[dir];
    nsWattAdds++;ewWattAdds++;MPPTWattAdds++;printWattAdds++;
  }
  
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
  String line = "";
  for (int dir = 0; dir < 4; dir++) {
    line=nwse[dir]+": ";
    Serial.print(line);
    Serial.print(printWattAdder[dir]/printWattAdds,1);
    printWattAdder[dir] = 0; // clear out the adder
    Serial.print("W  ");
    Serial.print(voltage[dir],1);
    Serial.print("V  ");
    Serial.print(current[dir]);
    Serial.print("A  ");
  }
  printWattAdds = 0;
  Serial.print("EW:");
  Serial.print(lastEW);
  Serial.print("  NS:");
  Serial.println(lastNS);
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
  if (MPPTWattAdds < 2) return; // we need averaged wattage
  static float watt_last[4] = {0};
  static int vector[4] = {1}; // which direction we're changing pwmval this time
  static int pwmval[4] = {0}; // what we last sent to analogWrite
  float wattage[4]; // note this should obscure the global wattage[] !!!
  for (int dir = 0; dir < 4; dir++) {
    wattage[dir] = MPPTWattAdder[dir] / MPPTWattAdds;
    MPPTWattAdder[dir] = 0; // clear them out
    if (watt_last[dir] > wattage[dir]) vector[dir] *= -1; // if we had more power last time, change direction
    pwmval[dir] += vector[dir]; // change (up or down) PWM value
    if (pwmval[dir] > 254) vector[dir] = -1;
    if (pwmval[dir] < 1) vector[dir] = 1;
    watt_last[dir] = wattage[dir]; // store previous cycle's data
  }
  MPPTWattAdds = 0; // clear it out
}

void getVoltages() {
  voltage[N] = (analogRead(VOLT_N) / V_COEFF);
  voltage[W] = (analogRead(VOLT_W) / V_COEFF);
  voltage[S] = (analogRead(VOLT_S) / V_COEFF);
  voltage[E] = (analogRead(VOLT_E) / V_COEFF);
  current[N] = (analogRead(CURRENT_N) / I_COEFF);
  current[W] = (analogRead(CURRENT_W) / I_COEFF);
  current[S] = (analogRead(CURRENT_S) / I_COEFF);
  current[E] = (analogRead(CURRENT_E) / I_COEFF);
  for (int dir = 0; dir < 4; dir++) wattage[dir] = voltage[dir] * current[dir];
}
