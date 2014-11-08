/*
  ScrubBOT: robot for cleaning grout
 */
 
#include <Servo.h>
Servo leftServo; //yellow wheel
Servo rightServo; //blue wheel
int leftForward = 121;
int leftBackward = 78;
int rightForward = 79;
int rightBackward = 121;


//pin assignments:
int emitterPin1 = 50; //left side IR LED PWR
int emitterPin2 = 48; //right side IR LED PWR
int emitterPin3 = 46; //center IR LED PWR
int sensorPin1 = 15; //left side IR sensor
int sensorPin2 = 14; //right side IR sensor
int sensorPin3 = 13; //center IR sensor
int bumpButton = 45;  //front crash button
int LED = 13; //onboard LED

//variable assignments:
int irSensor1 = 0;
int irSensor2 = 0;
int irSensor3 = 0;

//store the AD values from the IR sensors
int onTile1 = 0;
int onTile2 = 0;
int onTile3 = 0;
int onGrout1 = 0;
int onGrout2 = 0;
int onGrout3 = 0;
boolean onTrack = false; //set for off grout
int trackHistory = 0;

int bumpVal = 1; //front crash button value

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  // setup IR emitter/receivers:
  pinMode(emitterPin1, OUTPUT);
  digitalWrite(emitterPin1, LOW); //IR emitter 1 off
  pinMode(emitterPin2, OUTPUT);
  digitalWrite(emitterPin2, LOW); //IR emitter 2 off
  pinMode(emitterPin3, OUTPUT);
  digitalWrite(emitterPin3, LOW); //IR emitter 3 off
  // setup onboard LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  //setup front crash button
  pinMode(bumpButton, INPUT_PULLUP); //set pin to input with internal pullup resistor
  calibrate();  //run sensor calibration
  Serial.println("Place ScrubBOT inline with grout - detectors on either side.");
  Serial.println("...");
  Serial.println("Press 'Bump Button' to start cleaning");
  delay(100);
  while(bumpVal == 1){
    bumpVal = digitalRead(bumpButton);
    //do nothing... wait for input
    delay(5);
  }
  bumpVal = 1;  //reset bump value
  digitalWrite(LED, HIGH);
  delay(2000);
  leftServo.attach(2);
  leftServo.write(95); //stalled, no movement
  rightServo.attach(4);
  rightServo.write(95); //stalled, no movement
  onTrack = true;  //set for on grout
}

// ---------------------------------------------------------------------------------
// the loop routine runs over and over again forever:
void loop() {
  followGrout();
  delay(100);
}
// ----------------------------------------------------------------------------------

//IR sensor calibration (grout vs. tile...)
void calibrate(){
  digitalWrite(LED, LOW);
  //display message to user
  Serial.println("Place all three sensors over tile, then press 'Bump Button'.");
  wait_for_bump_button_press();
  Serial.println("OK...");
  digitalWrite(LED, HIGH);
  onTile1 = IR_sensor(sensorPin1, emitterPin1);
  onTile2 = IR_sensor(sensorPin2, emitterPin2);
  onTile3 = IR_sensor(sensorPin3, emitterPin3);
  Serial.print("On tile, left side: ");
  Serial.println(onTile1);
  Serial.print("On tile, right side: ");
  Serial.println(onTile2);
  Serial.print("On tile, center: ");
  Serial.println(onTile3);
  delay(2500);
  digitalWrite(LED, LOW);  
  Serial.println("...");
  
  Serial.println("Place all three sensors over grout line intersection (+), then press 'Bump Button'.");
  wait_for_bump_button_press();
  Serial.println("OK...");
  digitalWrite(LED, HIGH);
  onGrout1 = IR_sensor(sensorPin1, emitterPin1);
  onGrout2 = IR_sensor(sensorPin2, emitterPin2);
  onGrout3 = IR_sensor(sensorPin3, emitterPin3);
  Serial.print("On grout, left side: ");
  Serial.println(onGrout1);
  Serial.print("On grout, right side: ");
  Serial.println(onGrout2);
  Serial.print("On grout, center: ");
  Serial.println(onGrout3);
  delay(1500);
  digitalWrite(LED, LOW);
  Serial.println("...");
}

void wait_for_bump_button_press(){
  while(bumpVal == 1){
    bumpVal = digitalRead(bumpButton);
    //do nothing... wait for input
    delay(5);
  }
  bumpVal = 1;  //reset bump value
}

//read IR sensor, pass which pin to read
//returns the integer value of the read analog pin
int IR_sensor(int sensorPin, int emitterPin){  
  int n = 3; //right bit shift by this many bits to divide by 2^n, limit this to 2,3, or 4 (4,8, or 16 samples)
  int averageNum = 1 << n; //2^n
  unsigned int background = 0; //value with IR emitter off, must be unsigned to avoid sign extension when dividing
  unsigned int analogVal = 0; //value with IR emitter on
  int ms = 5;
  // read background input on analog:
  digitalWrite(emitterPin, LOW);
  delay(ms);
  for(int i = 0; i <= averageNum; i++){ //AD is 12-bit, int is 16-bit, roll-over isn't an issue as long as you collect less than 16 samples
    background += analogRead(sensorPin);
    delay(ms);
  }
  background >>= n; //get the average value
  //read illuminated input on analog
  digitalWrite(emitterPin, HIGH);
  delay(ms);
  for(int i = 0; i <= averageNum; i++){
    analogVal += analogRead(sensorPin);
    delay(ms);
  }
  analogVal >>= n; //get the average value
  //Serial.println(analogVal);
  analogVal -= background; //subtract average background
  digitalWrite(emitterPin, LOW);
  return analogVal;
}

void scrubRight() {
 leftServo.write(leftForward); //drive left wheel forward
 delay(6);
}

void scrubLeft() {
 rightServo.write(rightForward); //drive right wheel forward
 delay(6);
}

void scrubForward() { //drive both wheels forward
  leftServo.write(leftForward);
  rightServo.write(rightForward);
  delay(10);
}

void scrubBackward() { //drive back and to the right
  leftServo.write(leftBackward);
  rightServo.write(rightBackward);
  delay(750);
  leftServo.write(76);
  rightServo.write(76);
  delay(290);
  leftServo.write(76);
  rightServo.write(95);
  delay(550);
}

int overGrout(int sensorPin, int emitterPin, int onTile, int onGrout) {
 int irSensor = IR_sensor(sensorPin, emitterPin);
 if( abs(irSensor - onTile) <= abs(irSensor - onGrout)) {
   return 1; //over grout
 }
 else {
   return 0; // over tile
 }
 
}

void followGrout() {
  int leftStatus = 0; //could save memory by using status bits rather than whole integers
  int rightStatus = 0;
  int centerStatus = 0;
  
  leftStatus = overGrout(sensorPin1, emitterPin1, onTile1, onGrout1);
  rightStatus = overGrout(sensorPin2, emitterPin2, onTile2, onGrout2);
  centerStatus = overGrout(sensorPin3, emitterPin3, onTile3, onGrout3);
  
 
  if (leftStatus == 0 && centerStatus == 0 && rightStatus == 0) {
    onTrack = false;
    digitalWrite(LED, HIGH);
    scrubForward(); //go forward until you see any grout
  }
  else if (leftStatus == 0 && centerStatus == 0 && rightStatus == 1) {
    onTrack = false;
    digitalWrite(LED, HIGH);
    scrubRight(); //go toward the grout
  }
  else if (leftStatus == 0 && centerStatus == 1 && rightStatus == 0) {
    onTrack = true;
    digitalWrite(LED, LOW);
    scrubForward(); //on track
  }
  else if (leftStatus == 0 && centerStatus == 1 && rightStatus == 1) {
    onTrack = true;
    digitalWrite(LED, LOW);
    scrubRight(); //adjust alignmnet
  }
  else if (leftStatus == 1 && centerStatus == 0 && rightStatus == 0) {
    onTrack = false;
    digitalWrite(LED, HIGH);
    scrubLeft(); //go toward the grout
  }
  else if (leftStatus == 1 && centerStatus == 0 && rightStatus == 1) {
    onTrack = false;
    digitalWrite(LED, HIGH);
    scrubBackward();
    scrubLeft(); //crossing grout, try to center on it
  }
  else if (leftStatus == 1 && centerStatus == 1 && rightStatus == 0) {
    onTrack = true;
    digitalWrite(LED, LOW);
    scrubLeft(); //adjust alignment
  }
  else if (leftStatus == 1 && centerStatus == 1 && rightStatus == 1) {
    onTrack = true;
    digitalWrite(LED, LOW);
    scrubForward(); //on track at a grout crossing, opportunity for re-calibration
  }
  else {
    onTrack = false;
    digitalWrite(LED, HIGH);
    scrubForward(); //catch-all
  }

  bumpVal = digitalRead(bumpButton);
  if(bumpVal == 0) {
    bumpVal = 1;
    digitalWrite(LED, HIGH);
    scrubBackward();
  }
  if(onTrack == false) {
     off_track();
  }
}

void off_track() { //if the scrubBOT is off track
  trackHistory += trackHistory;
  if(trackHistory > 5) {
   trackHistory = 0; //reset history
  }
}

