
/* 
 Firmware for the Kaebot
 
 */

#include <Makeblock.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <math.h>


const int stepsPerRev = 200;  // change this to fit the number of steps per revolution
                                     // for your motor



const unsigned long MAXCOMMANDWAIT=360000; 
                                     
unsigned long lastcommandtime=0;
                                     
int dirPinX = mePort[PORT_1].s1;//the direction pin connect to Base Board PORT1 SLOT1
int stpPinX = mePort[PORT_1].s2;//the Step pin connect to Base Board PORT1 SLOT2

int dirPinY = mePort[PORT_2].s1;
int stpPinY = mePort[PORT_2].s2;

int lampPin = 8; //orion port 7 black wire

MeLimitSwitch xplimitSwitch(PORT_6,1);
MeLimitSwitch xmlimitSwitch(PORT_6,2);
MeLimitSwitch yplimitSwitch(PORT_3,1);
MeLimitSwitch ymlimitSwitch(PORT_3,2);                                     

int curr_x=0;
int curr_y=0;
int x_max=0;
int y_max=0;

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;


const int smallestDelay = 175; // inverse of motor's top speed
const int numAccelSteps = 400; // number of steps before reaching top speed
          
// distrubtions used in controlling motor speed
double normal[100];
int quadratic[100];
int inverse[numAccelSteps];


// *** Functions for initializing distributions ***

void initInverse() {
  double a = 80000.0;
  int b = 30;
  double c = a / (numAccelSteps + b) - smallestDelay;
  for (int i = 0; i < numAccelSteps; i++) {
    int num = (int) (a / (i + b) - c);
    //int num = (int) (-900 * log((double) (i + 1) / numAccelSteps) + smallestDelay);
    inverse[i] = num;
    //Serial.println(num);
  }
}

void initNormal() {
  // setup normal distribution
  double pi = 3.14;
  double e = 2.72;
  double a = 2.5 / sqrt(2 * pi);
  for (int i = 0; i < 100; i++) {
    double b = -1 * pow(0.03 * i - 1.5, 2) / 2;
    normal[i] = 1 / (a * pow(e, b));
    //Serial.println(normal[i]);
  }
}

void initQuadratic() {
  // setup quadratic distribution
  for (int i = 0; i < 100; i++) {
    quadratic[i] = (int) (pow(i - 50, 4) / 6000) + 200;
  }
}

void calibrate() {
  int ms = 300;
  
  // find zero
  digitalWrite(dirPinX,0);
  delay(ms);
  digitalWrite(dirPinY,0);
  delay(ms);
  while (!xmlimitSwitch.touched()){
    digitalWrite(stpPinX, HIGH);
    delayMicroseconds(ms);
    digitalWrite(stpPinX, LOW);
    delayMicroseconds(ms);
  }//while not at x==0
  while (!ymlimitSwitch.touched()){
    digitalWrite(stpPinY, HIGH);
    delayMicroseconds(ms);
    digitalWrite(stpPinY, LOW);
    delayMicroseconds(ms);  
  }//while not at x==0
  
  curr_x = 0;
  curr_y = 0;
  
  // calibrate max
  digitalWrite(dirPinX,1);
  delay(ms);
  digitalWrite(dirPinY,1);
  delay(ms);
  while (!xplimitSwitch.touched()){
    digitalWrite(stpPinX, HIGH);
    delayMicroseconds(ms);
    digitalWrite(stpPinX, LOW);
    delayMicroseconds(ms);
    curr_x++;
  }//while not at x==max
  while (!yplimitSwitch.touched()){
    digitalWrite(stpPinY, HIGH);
    delayMicroseconds(ms);
    digitalWrite(stpPinY, LOW);
    delayMicroseconds(ms);
    curr_y++;
  }//while not at y==max
  
  x_max = curr_x;
  y_max = curr_y;
  
  goto_machine_zero();
}


void goto_machine_zero(void){

  move_to_xy(-100,-100);
  
  curr_x=0;
  curr_y=0;
 // Serial.println(curr_x);
 // Serial.println(curr_y);
  
}//end goto maching zero

void goto_machine_max(void){
  
  move_to_xy(x_max + 100, y_max + 100);

  x_max = curr_x;
  y_max = curr_y;
  
  
}//end goto maching zero


int move_mx(int mx){
  if (mx <1) mx=1;
  digitalWrite(dirPinX,0);
  for (int x=0; x < mx; x++){
    if (!xmlimitSwitch.touched()){
      digitalWrite(stpPinX, HIGH);
      delayMicroseconds(200);
      digitalWrite(stpPinX, LOW);
      delayMicroseconds(200);
      curr_x--;
    }//if not at x==0
  }//end for each
}//end move_mx

int move_px(int mx){
  if (mx <1) mx=1;
  digitalWrite(dirPinX,1);
  for (int x=0; x < mx; x++){
    if (!xplimitSwitch.touched()){
      digitalWrite(stpPinX, HIGH);
      delayMicroseconds(200);
      digitalWrite(stpPinX, LOW);
      delayMicroseconds(200);
      curr_x++;
    }//if not at x==max
  }//end for each x
  
}//end move_px

int move_my(int my){
  if (my <1) my=1;
  digitalWrite(dirPinY,0);
  for (int y=0; y < my; y++){
    if (!ymlimitSwitch.touched()){
      digitalWrite(stpPinY, HIGH);
      delayMicroseconds(200);
      digitalWrite(stpPinY, LOW);
      delayMicroseconds(200);
      curr_y--;
    }//if not at y==0
  }//end for 
  
}//end move_my


int move_py(int my){
  if (my <1) my=1;
  digitalWrite(dirPinY,1);
  for (int y=0; y < my; y++){
    if (!yplimitSwitch.touched()){
      digitalWrite(stpPinY, HIGH);
      delayMicroseconds(200);
      digitalWrite(stpPinY, LOW);
      delayMicroseconds(200);
      curr_y++;
      
    }//if not at y==0
  }//end for 
  
}

void move_to_xy(int x, int y) {
 
  int dx = x - curr_x;
  int dy = y - curr_y; 
  if (dx == 0 && dy == 0) return;
  
  MeLimitSwitch limitSwitchX;
  MeLimitSwitch limitSwitchY;
  int numStepsX;
  int numStepsY;
  
  if (dx < 0) {
    digitalWrite(dirPinX, 0);
    limitSwitchX = xmlimitSwitch;
    numStepsX = -1 * dx;
  } else {
    digitalWrite(dirPinX, 1);
    limitSwitchX = xplimitSwitch;
    numStepsX = dx;
  }
  
  if (dy < 0) {
    digitalWrite(dirPinY, 0);
    limitSwitchY = ymlimitSwitch;
    numStepsY = -1 * dy;
  } else {
    digitalWrite(dirPinY, 1);
    limitSwitchY = yplimitSwitch;
    numStepsY = dy;
  }
  
  
  int i_x = 0;
  int i_y = 0;
  boolean x_reached = (numStepsX == 0) || limitSwitchX.touched();
  boolean y_reached = (numStepsY == 0) || limitSwitchY.touched();
  
 
    while(!x_reached) {
      int ms;
      if (i_x < numAccelSteps)                  ms = inverse[i_x];
      else if (i_x > numStepsX - numAccelSteps) ms = inverse[numStepsX - i_x];
      else                                      ms = inverse[numAccelSteps - 1];
    
      digitalWrite(stpPinX, HIGH);
      delayMicroseconds(ms);
      digitalWrite(stpPinX, LOW);
      delayMicroseconds(ms);
      
      i_x++;
      x_reached = (i_x == numStepsX) || limitSwitchX.touched();
    }
    
    delay(175);
    

    while (!y_reached) {
      int ms;
      if (i_y < numAccelSteps)                  ms = inverse[i_y];
      else if (i_y > numStepsY - numAccelSteps) ms = inverse[numStepsY - i_y];
      else                                      ms = inverse[numAccelSteps - 1];
    
      digitalWrite(stpPinY, HIGH);
      delayMicroseconds(ms);
      digitalWrite(stpPinY, LOW);
      delayMicroseconds(ms);
      
      i_y++;
      y_reached = (i_y == numStepsY) || limitSwitchY.touched();
    }
    

  
  if (dx < 0) curr_x -= i_x;
  else        curr_x += i_x;
  
  if (dy < 0) curr_y -= i_y;
  else        curr_y += i_y;
  
}//end move to x y


void move_to_x(int x) {
  move_to_xy(x, curr_y);
}

void move_to_y(int y) {
  move_to_xy(curr_x, y);
}

void setLamp(int intensity){
  //if (intensity >= 99) digitalWrite(lampPin, HIGH);
 // if (intensity <= 0) digitalWrite(lampPin,LOW);
   analogWrite(lampPin, intensity);
  
  
}//end setLamp


void setup() {
  
  
  // initialize the serial port:
  Serial.begin(9600);
  pinMode(dirPinX, OUTPUT);
  pinMode(stpPinX, OUTPUT);
  pinMode(dirPinY, OUTPUT);
  pinMode(stpPinY, OUTPUT);
  pinMode(lampPin, OUTPUT);
  
  //initNormal();
  //initQuadratic();
  //Serial.println("Initializing movement curves...");
  initInverse();
  
  calibrate();
 //goto_machine_zero();
 //goto_machine_max();
 //goto_machine_zero();
  Serial.println("RR");
  
}//end setup

void loop(){
  
  //DEBUG LIMIT
  /*
  if(yplimitSwitch.touched()) Serial.println("y positive");
  if(ymlimitSwitch.touched()) Serial.println("y negative");
  if(xplimitSwitch.touched()) Serial.println("x positive");
  if(xmlimitSwitch.touched()) Serial.println("x negative");
 */ 
  
  if (stringComplete){
    //parse command
    //
    
    
    if (inputString.indexOf("D") >=0){
      String moveamnt = inputString.substring(inputString.indexOf("D")+1);
      
      move_px(moveamnt.toInt());
    } else //end if D
    if  (inputString.indexOf("A") >=0){
      String moveamnt = inputString.substring(inputString.indexOf("A")+1);
      
      move_mx(moveamnt.toInt());
    }else //end if A
    if  (inputString.indexOf("W") >=0){
      String moveamnt = inputString.substring(inputString.indexOf("W")+1);
      move_my(moveamnt.toInt());
    } else //end if W
    if  (inputString.indexOf("S") >=0){
      String moveamnt = inputString.substring(inputString.indexOf("S")+1);
      move_py(moveamnt.toInt());
    } else //end if S 
    if  (inputString.indexOf("ZZ") >=0){
      goto_machine_zero();
    } else 
    if  (inputString.indexOf("LL") >=0){
      goto_machine_max();
    } else
    if (inputString.indexOf("CC")>=0){
	calibrate();
    } else
    if (inputString.indexOf("IL") >=0){
        String lightamount = inputString.substring(inputString.indexOf("IL")+2);
        int lumos = lightamount.toInt();
        setLamp(lumos);
        
    } else
    if (inputString.indexOf("P")>=0){ 
      Serial.print (curr_x);
      Serial.print (",");
      Serial.println(curr_y);
    }else //end if p
    if(inputString.indexOf("MX") >=0){
      String moveamnt = inputString.substring(inputString.indexOf("MX")+2);
      long mv_x=moveamnt.toInt();
      if (mv_x <0) mv_x=0;
      move_to_x(mv_x);
    } //
    else if(inputString.indexOf("MY") >=0){
      String moveamnt = inputString.substring(inputString.indexOf("MY")+2);
      long mv_y=moveamnt.toInt();
      if (mv_y <0) mv_y=0;
      move_to_y(mv_y);
    } else if (inputString.indexOf("M") >= 0 && inputString.indexOf(",") >= 0) {
      int x = inputString.substring(inputString.indexOf("M") + 1, inputString.indexOf(",")).toInt();
      String y_str = inputString.substring(inputString.indexOf(",") + 1);
      int y = y_str.toInt();
      move_to_xy(x, y);
    }
    
    
    
    //Serial.println(inputString);
    inputString="";
    stringComplete=false;
    Serial.println("RR");
    //reset command timer
    lastcommandtime=millis();
    
  }//end if complete
  
  unsigned long currtime;
  currtime = millis();
  
  //if ((currtime - lastcommandtime) > MAXCOMMANDWAIT){
   // Serial.println("RR");
   // lastcommandtime=millis();
 // }//end if timeout  
  
}//end loop

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    } 
  }
}
