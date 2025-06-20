static int pinA = 2; // Our first hardware interrupt pin is digital pin 2
static int pinB = 3; // Our second hardware interrupt pin is digital pin 3
volatile byte aFlag = 0; // let's us know when we're expecting a rising edge on pinA to signal that the encoder has arrived at a detent
volatile byte bFlag = 0; // let's us know when we're expecting a rising edge on pinB to signal that the encoder has arrived at a detent (opposite direction to when aFlag is set)
volatile long encoderPos = 0x70000000; //this variable stores our current value of encoder position. Change to int or uin16_t instead of byte if you want to record a larger range than 0-255
volatile byte oldEncPos = 0; //stores the last encoder position value so we can compare to the current reading and see if it has changed (so we know when to print to the serial monitor)
volatile byte reading = 0; //somewhere to store the direct values we read from our interrupt pins before checking to see if we have moved a whole detent



// our mark where we start from, with this we can calc the current position
int startEncoderInches = 4;
int startEncoderPartial = 17;      // 64ths of an inch
long startEncoderPos = 0x70000000;

// our current head position, this is calc'd from the start info and the
// current encoder pos
int currentHeadInches = 0;
int currentHeadPartial = 0;

// info for stepping to next position
int currentStepInches = 1;
int currentStepPartial = 8;
long stepCount = 675;

long last_accuset_down_pos = 0;  // this is the last position we auto positioned to, if head is moved up for gig back, we have this position
                                 // to base the next move down from.

long target_accuset_pos = 0;  // this is our target encoder position

long last_speed_adjustment = 0;  // time we last made a speed adjustment based on position


const int PULSES_PER_INCH = 600;

void initRotaryEncoder()
{
#ifndef NO_ENCODER
  EEPROM.get(200, stepCount);

  // convert Encode pulse count for a step to inches
  currentStepInches = stepCount / 100;
  currentStepPartial = stepCount - (currentStepInches * 100);
  // convert to 64ths
  currentStepPartial = (currentStepPartial * 64) / 100;


  pinMode(pinA, INPUT_PULLUP); // set pinA as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  pinMode(pinB, INPUT_PULLUP); // set pinB as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  //pinMode(ledPin, OUTPUT);      

  attachInterrupt(digitalPinToInterrupt(pinA), PinA, RISING); // set an interrupt on PinA, looking for a rising edge signal and executing the "PinA" Interrupt Service Routine (below)
  attachInterrupt(digitalPinToInterrupt(pinB), PinB, RISING); // set an interrupt on PinB, looking for a rising edge signal and executing the "PinB" Interrupt Service Routine (below)

  Serial.println("Rotary Encoder Initialized");
#endif
}

void PinA()
{
  cli(); //stop interrupts happening before we read pin values
  
  reading = PINE & 0x30; // read all eight pin values then strip away all but pinA and pinB's values
  //reading = digitalRead(2) << 2;
  //reading += digitalRead(3) << 3;
  
  //Serial.println(digitalRead(2));
  
  if (reading == B00110000 && aFlag)
  { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    encoderPos --; //decrement the encoder's position count
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
    //Serial.println("A");
  }
  else if (reading == B00010000) 
  {
    bFlag = 1; //signal that we're expecting pinB to signal the transition to detent from free rotation
    //Serial.println("B");    
  }
  sei(); //restart interrupts
  //Serial.print("Z");
  /*
  // ai0 is activated if DigitalPin nr 2 is going from LOW to HIGH
  // Check pin 3 to determine the direction
  if(digitalRead(3)==LOW) {
    encoderPos++;
  }else{
    encoderPos--;
  }
  */
}

void PinB()
{
  cli(); //stop interrupts happening before we read pin values
  reading = PINE & 0x30; //read all eight pin values then strip away all but pinA and pinB's values
  //reading = digitalRead(2) << 2;
  //reading += digitalRead(3) << 3;

  //Serial.println(digitalRead(3));

  if (reading == B00110000 && bFlag) 
  { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    encoderPos ++; //increment the encoder's position count
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
    //Serial.println("A");
  }
  else if (reading == B00100000) 
  {
    aFlag = 1; //signal that we're expecting pinA to signal the transition to detent from free rotation
    //Serial.println("D");
  }
  
  sei(); //restart interrupts

  //Serial.print(reading);
  /*
  if(digitalRead(2)==LOW) {
    encoderPos--;
  }else{
    encoderPos++;
  }
  */
}


void accuset_down()
{
  if(last_accuset_down_pos != 0)
  {
    // work from last position we went to
    target_accuset_pos = last_accuset_down_pos + stepCount;
  }
  else
  {
    // work from current position
    target_accuset_pos = encoderPos + stepCount;
  }

  operation_mode = MODE_ACCUSET;

  // start the motor, speed is based on how far we have to go
  accuset_set_motor_speed();
}


void accuset_up()
{
  // we alway bump up from current position
  target_accuset_pos = encoderPos - stepCount;
  operation_mode = MODE_ACCUSET;

  accuset_set_motor_speed();
}

void accuset_set_motor_speed()
{
  int speed;
  long currentPos = encoderPos;
  
  // how far do we have to go.
  long dif = currentPos - target_accuset_pos;
  if(dif < 0)
    dif = -dif;

  if(dif > 600) // more than an inch
    speed = 100;
  else if(dif > 300)
    speed = 75;
  else if(dif > 150)
    speed = 50;
  else if(dif > 100)
    speed = 30;
  else if(dif > 50)
    speed = 10;
  else
    speed = 5;

  if(currentPos > target_accuset_pos)
    MotorForward(UPDOWN_DEVICEID, speed);
  else
    MotorReverse(UPDOWN_DEVICEID, speed);
  updown_active = speed;
      
  last_speed_adjustment = millis();
} 

void accuset_loop()
{
  //see if we are within our range to stop
  long pos = encoderPos;
  long dif = pos - target_accuset_pos;
  
  if(dif > -5 && dif < 5)
  {
    MotorStop(UPDOWN_DEVICEID);
    operation_mode = MODE_IDLE;
  }
  else
  {
    // see if time for speed adjustment
    long m = millis();
    if(m - last_speed_adjustment > 100)
      accuset_set_motor_speed();
  }
}

// Real time clock / EEPROM chip reading lib
//https://github.com/cyberp/AT24Cx

//https://thecavepearlproject.org/tag/at24c32/

//EEPROM id is 0x57 on I1C bus

//https://forum.arduino.cc/index.php?topic=140205.75

// https://github.com/ivanseidel/DueTimer/blob/master/TimerCounter.md
/* arduino due code to handle rotary encoder

TIOA0, TIOB0 and TIOA1 to decode 2 channel quadrature plus an index. 
(SAM3X manual p. 855) If my reading of the pinout is correct, this would be pins 2, 13 and AD7.

void setup() {
  // Setup the quadrature encoder
  // For more information see http://forum.arduino.cc/index.php?topic=140205.30
  REG_PMC_PCER0 = PMC_PCER0_PID27;   // activate clock for TC0
  REG_TC0_CMR0 = TC_CMR_TCCLKS_XC0;  // select XC0 as clock source

  // activate quadrature encoder and position measure mode, no filters
  REG_TC0_BMR = TC_BMR_QDEN
              | TC_BMR_POSEN
              | TC_BMR_EDGPHA;

  // enable the clock (CLKEN=1) and reset the counter (SWTRG=1)
  REG_TC0_CCR0 = TC_CCR_CLKEN | TC_CCR_SWTRG;
}

// The encoder position is then accessed from
void loop() {
  int newPosition = REG_TC0_CV0; // Read the encoder position from register
}
*/

/* TFT shield

 use use d0-d7 on due USE_DUE_8BIT_PROTOSHIELD 
https://github.com/prenticedavid/MCUFRIEND_kbv/issues/9

*/

void setup() {

  pmc_enable_periph_clk(ID_TC0);
  TC_Configure(TC0,0, TC_CMR_TCCLKS_XC0); 
 
  TC0->TC_BMR = TC_BMR_QDEN          // Enable QDEC (filter, edge detection and quadrature decoding)
                | TC_BMR_POSEN       // Enable the position measure on channel 0
                //| TC_BMR_EDGPHA    // Edges are detected on PHA only
                //| TC_BMR_SWAP      // Swap PHA and PHB if necessary (reverse acting)
                | TC_BMR_FILTER      // Enable filter
                | TC_BMR_MAXFILT(63) // Set MAXFILT value
               ;                       

  // Enable and trigger reset
  TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG | TC_CCR_CLKEN;
 
}

void loop() {
  int16_t count = REG_TC0_CV0;
}