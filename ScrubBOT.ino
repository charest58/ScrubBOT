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
//Left --> Center --> Right
int emitterPins[] = {10, 11, 12, 13, 14, 15, 16, 17, 18};
int sensorPins[]= {45, 46, 47, 48, 49, 50, 51, 52, 53};

//store the AD values from the IR sensors
int onTile[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
int onGrout[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
boolean onTrack = false; //set for off grout

//initialize the PID control loop
int previousError = 0; 
int integral = 0;

int bumpVal = 1; //front crash button value

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  // setup IR emitter/receivers:
  int i;
  for (i = 0; i < 10; i = i + 1) {
    pinMode(emitterPins[i], OUTPUT);
    digitalWrite(emitterPins[i], LOW); //IR emitter off
  }
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
  Serial.println("Place all sensors over tile, then press 'Bump Button'.");
  wait_for_bump_button_press();
  Serial.println("OK...");
  digitalWrite(LED, HIGH);
  int i;
  for (i = 0; i < 10; i = i + 1) {
    onTile[i] = IR_sensor(sensorPins[i], emitterPins[i]);
  }
  Serial.print("On tile, from left to right: ");
  for (i = 0; i < 10; i = i + 1) {
    Serial.println(onTile[i]);
  }
  delay(2500);
  digitalWrite(LED, LOW);  
  Serial.println("...");
  
  Serial.println("Place all sensors over grout, then press 'Bump Button'.");
  wait_for_bump_button_press();
  Serial.println("OK...");
  digitalWrite(LED, HIGH);
  for (i = 0; i < 10; i = i + 1) {
    onGrout[i] = IR_sensor(sensorPins[i], emitterPins[i]);
  }
  Serial.print("On grout, from left to right: ");
  for (i = 0; i < 10; i = i + 1) {
    Serial.println(onGrout[i]);
  }
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

int overGrout(int sensorPin, int emitterPin, int onTile, int onGrout, int distance) {
 int irSensor = IR_sensor(sensorPin, emitterPin);
 if( abs(irSensor - onTile) <= abs(irSensor - onGrout)) {
   return distance; //over grout
 }
 else {
   return 0; // over tile
 } 
}

int pid() {
  int distances[] = {-4, -3, -2, -1, 0, 1, 2, 3, 4};
  int Kp = 100; //porportional constant
  int Ki = 50; //integral constant
  int Kd = 50; //derivitive constant
  
  int error = 0;
  int i;
  for (i = 0; i < 10; i = i + 1) {
    error += overGrout(sensorPins[i], emitterPins[i], onTile[i], onGrout[i], distances[i]);
  }
  
  integral+=error
  int derivitive = error - previousError;  
  int correction=(error*Kp) + (integral*Ki) + (derivitive*Kd);
  return correction;
  previousError=error;  
}

void followGrout() {
  leftServo.write(leftForward);
  rightServo.write(rightForward);
  delay(6)
  correction = pid();
  leftServo.write(leftForward*correction);
  rightServo.write(rightForward*correction);  
}
