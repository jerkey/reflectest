/*  solar_aimer by jake sternberg 2013-6-28
    this program runs on an atmega328 or similar, to monitor voltages
    of four solar cells connected to FETs driven by PWM channels.

    Analog channels A0-A3 measure NWSE cells' voltage (positive terminal,
    their minus terminal is grounded)

    Analog channels A4-A7 measure 1-ohm current-sensing resistors with a
    gain of 3.0303 (1Mohm/330Kohm) which are connected between the cells'
    positive terminal and the drain of N-channel FETs

    The N-channel FETs' sources are grounded.  Their gates are connected
    to a 0.1uF capacitor to ground, and a 2K resistor to digital pins 3,
    5, 6 and 11 (North West South East in that order for everything)
    This way, those PWM channels can control the gate voltages precisely.

    servos to aim the assembly sunward are connected to digital pins 9
    and 10.  the servo on pin 9 aims North/South and pin 10 East/West.
    The motors aim the assembly toward the sun to maximize solar collection.
    
*/
#define BAUDRATE 57600
#define EWHYST 0.01 // how much difference calls for tracking
#define NSHYST 0.01
#define EWWAYOFF 0.05 // total east+west wattage below this means aim is way off
#define NSWAYOFF 0.05 // total north+south wattage below this means aim is way off
#define MINVOLT 0.7 // voltage below which PWM load must be restarted
#define MINWATT 0.7 // voltage below which PWM load must only increase
#define FET_THRESHOLD 55 // the analogWrite() value corresponding to transistor fully open
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
#define V_COEFF 205.37 // ADC ratio 1023 / 3.3v = 310.0
#define I_COEFF 623.4 // ADC ratio 1023 / 3.3v * 1000K / 330K = 102.3

#define EDIR 1 // which direction servo value increments for east
#define NDIR -1 // which direction servo value increments for north
#define EWNULL 1500 // eastwest midpoint
#define NSNULL 1425 // northsouth midpoint
#define EWRANGE 900 // eastwest range away from midpoint
#define NSRANGE 375 // northsouth range away from midpoint
#define NSPIN 10 // pin number for northsouth servo
#define EWPIN 9 // pin number for eastwest servo

#define TRACKEWTIME 40  // time between eastwest tracking calls
#define TRACKNSTIME 40  // time between northsouth tracking calls
#define PRINTTIME 500     // time between printing display
#define MPPTTIME 50     // time between load tracking calls

float voltage[4],current[4],wattage[4] = {0};
float nwseWattAdder[4],MPPTWattAdder[4],printWattAdder[4] = {0}; // for averaging wattages for trackers
int nsWattAdds,ewWattAdds,MPPTWattAdds,printWattAdds = 0; // how many times adder was added
const byte pwmPin[4] = {LOAD_N, LOAD_W, LOAD_S, LOAD_E};
int pwmVal[4] = {FET_THRESHOLD}; // what we last sent to analogWrite

int EW = EWNULL; // position value of eastwest servo
int NS = NSNULL; // position value of northsouth servo

#include <Servo.h>
Servo ewServo,nsServo; // create servo objects

void setup() {
  Serial.begin(BAUDRATE);
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
  }
  nsWattAdds++;ewWattAdds++;MPPTWattAdds++;printWattAdds++;
  
  if (timenow - lastPrint > PRINTTIME) { // run only one of these tracks per loop cycle
    printDisplay();
    lastPrint = timenow;
  } else if (timenow - lastEW > TRACKEWTIME) {
    trackEW();
    lastEW = timenow;
  } else if (timenow - lastNS > TRACKNSTIME) {
    trackNS();
    lastNS = timenow;
  } else if (timenow - lastMPPT > MPPTTIME) {
    trackMPPT();
    lastMPPT = timenow;
  }
}

void printDisplay() {
  String line = "";
  for (int order = 0; order < 4; order++) {
    int dir = order;
    if (order == W) dir = S; // i want them to print in order
    if (order == S) dir = W; // NORTH SOUTH WEST EAST
    line=String(nwse[dir])+": "; // print the letter of the direction
    Serial.print(line);
    Serial.print(voltage[dir],2);
    Serial.print("V  ");
    Serial.print(current[dir],2);
    Serial.print("A  ");
    Serial.print(printWattAdder[dir]/printWattAdds,3);
    printWattAdder[dir] = 0; // clear out the adder
    Serial.print("W  ");
    Serial.print(pwmVal[dir]);
    Serial.print("PWM   ");
  }
  printWattAdds = 0;
  Serial.print("NS:");
  Serial.print(NS);
  Serial.print("   EW:");
  Serial.println(EW);
}

void trackEW() {
  static int walkVector = 1; // which direction we will wander
  // static float minimumRatio = 10000; // lowest/best ratio we have achieved so far
  float east = voltage[E]; // nwseWattAdder[E] / ewWattAdds;
  float west = voltage[W]; // nwseWattAdder[W] / ewWattAdds;
  nwseWattAdder[E] = 0; nwseWattAdder[W] = 0; ewWattAdds = 0; // clear them out
  // float ratio = east / west;
  // if (abs(ratio) < abs(minimumRatio)) minimumRatio = ratio; // store our best score

  // if (east + west < EWWAYOFF) { // almost no wattage!  we need to wander to find power
  //   if (abs(EW - EWNULL) >= EWRANGE) walkVector *= -1; // if at end of travel, change direction
  //   EW += walkVector; // go in the direction we're wandering
  //   ewServo.writeMicroseconds(EW); // send it to the new location
  // }
  // else 
  if ((east > west + EWHYST) && (EW!=EWNULL-EDIR*EWRANGE)) {
    EW -= EDIR; // move the servo WEST
    ewServo.writeMicroseconds(EW); // send it to the new location
  }
  else if ((west > east + EWHYST) && (EW!=EWNULL+EDIR*EWRANGE)) {
    EW += EDIR; // move the servo EAST
    ewServo.writeMicroseconds(EW); // send it to the new location
  }
}

void trackNS() {
  static int walkVector = 1; // which direction we will wander
  // static float minimumRatio = 10000; // lowest/best ratio we have achieved so far
  float north = voltage[N]; // nwseWattAdder[N] / nsWattAdds;
  float south = voltage[S]; // nwseWattAdder[S] / nsWattAdds;
  nwseWattAdder[N] = 0; nwseWattAdder[S] = 0; nsWattAdds = 0; // clear them out
  // float ratio = north / south;
  // if (abs(ratio) < abs(minimumRatio)) minimumRatio = ratio; // store our best score

  // if (north + south < NSWAYOFF) { // almost no wattage!  we need to wander to find power
  //   if (abs(NS - NSNULL) >= NSRANGE) walkVector *= -1; // if at end of travel, change direction
  //   NS += walkVector; // go in the direction we're wandering
  //   nsServo.writeMicroseconds(NS); // send it to the new location
  // }
  // else 
  if ((north > south + NSHYST) && (NS!=NSNULL-NDIR*NSRANGE)) {
    NS -= NDIR; // move the servo SOUTH
    nsServo.writeMicroseconds(NS); // send it to the new location
  }
  else if ((south > north + NSHYST) && (NS!=NSNULL+NDIR*NSRANGE)) {
    NS += NDIR; // move the servo NORTH
    nsServo.writeMicroseconds(NS); // send it to the new location
  }
}

void trackMPPT() {
  if (MPPTWattAdds < 2) return; // we need averaged wattage
  static float watt_last[4] = {0};
  static int vector[4] = {1}; // which direction we're changing pwmVal this time
  float wattage[4]; // note this should obscure the global wattage[] !!!
  for (int dir = 0; dir < 4; dir++) {
    wattage[dir] = round((MPPTWattAdder[dir] / MPPTWattAdds) * 1000); // round to 1mW
    MPPTWattAdder[dir] = 0; // clear them out
    if (watt_last[dir] > wattage[dir]) vector[dir] *= -1; // if we had more power last time, change direction
    if (voltage[dir] < MINVOLT) {
      vector[dir] = 1; // start over
      pwmVal[dir] = FET_THRESHOLD; // start over
    }
    if (wattage[dir] < MINWATT) vector[dir] = 1; // increase the pwm until more wattage or we go below MINVOLT
    if (pwmVal[dir] > 254) vector[dir] = -1; // important bounds checking
    if (pwmVal[dir] <= FET_THRESHOLD) vector[dir] = 1; // important bounds checking
    pwmVal[dir] += vector[dir]; // change (up or down) PWM value
    analogWrite(pwmPin[dir],pwmVal[dir]); // actually set the load
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
