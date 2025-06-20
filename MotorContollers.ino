// current speed setting of each controller
int updown_active = 0;
int foreback_active = 0;

extern const int PS2_NEUTRAL;

// a mini-filter to smooth out the joystick inputs
unsigned int udboxcar[4] = { PS2_NEUTRAL, PS2_NEUTRAL, PS2_NEUTRAL, PS2_NEUTRAL };
int udboxcarPos = 0;
unsigned int udboxcarSum = 4 * PS2_NEUTRAL;

unsigned int fbboxcar[4] = { PS2_NEUTRAL, PS2_NEUTRAL, PS2_NEUTRAL, PS2_NEUTRAL };
int fbboxcarPos = 0;
unsigned int fbboxcarSum = 4 * PS2_NEUTRAL;

// device id of the motor controllers
const byte UPDOWN_DEVICEID = 1;
const byte FORWARDBACK_DEVICEID = 2;

int updown_range[2] = { 50, 100 };
int foreback_range[3] = { 33, 50, 100 };

extern Cytron_PS2Shield ps2;

void initMotorControllers()
{
  int i;
  int address = 100;

#ifndef NO_LCD
  for(i = 0; i < sizeof(updown_range) / sizeof(int); i++)
  {
    EEPROM.get(address, updown_range[i]);
    address += sizeof(int);
  }

  for(i = 0; i < sizeof(foreback_range) / sizeof(int); i++)
  {
    EEPROM.get(address, foreback_range[i]);
    address += sizeof(int);
  }
#else
  Serial.println("Using default motor speed factors.");
#endif

  Serial2.begin(9600);
  delay(500);

  Serial2.write(0xAA);
  
  exitSafeStart(UPDOWN_DEVICEID);
  exitSafeStart(FORWARDBACK_DEVICEID);

  Serial.println("Motor controllers initialized");
}


// returns true if any motor is running
bool ControlMotors()
{
  unsigned int udspeed;
  unsigned int fbspeed;

  int multiplier = 0;
  
  // look for emergency stop
  if(ps2.isRedButtonPressed())  //Triangle pressed
  {
    MotorStop(UPDOWN_DEVICEID);
    MotorStop(FORWARDBACK_DEVICEID);
    Serial.println("!Emergency Stop!!");
    DisplayErrorCode(3,5);  // stop all processing
  }
  
  // left stick is up/down
#ifdef NO_PS2
  udspeed = PS@_NEUTRAL;
#else
  udspeed = ps2.readLeftJoystickY();
#endif  

  /*if(udspeed != PS2_NEUTRAL && udspeed != 127)
  {
    Serial.print("Raw UDSpeed ");
    Serial.println(udspeed);
  }*/
  
  // take an average of the last 4 reading to smooth out the speed values
  udboxcarSum -= udboxcar[udboxcarPos];
  udboxcar[udboxcarPos] = udspeed;
  udboxcarPos++;
  if(udboxcarPos >= 4)
    udboxcarPos = 0;
  udboxcarSum += udspeed;

  udspeed = udboxcarSum / 4;   // divide the sum by 4

  //Serial.print("Sum " );
  //Serial.print(udboxcarSum);
  //Serial.print("  Average UDSpeed ");
  //Serial.println(udspeed);

  // right stick is forward/back
#ifdef NO_PS2
  fbspeed = PS2_NEUTRAL;
#else
  fbspeed = ps2.readRightJoystickY();
#endif

  /*if(fbspeed != PS2_NEUTRAL)
  {
    Serial.print("Raw FBSpeed ");
    Serial.println(fbspeed);
  }*/
  
  fbboxcarSum -= fbboxcar[fbboxcarPos];
  fbboxcar[fbboxcarPos] = fbspeed;
  fbboxcarPos++;
  if(fbboxcarPos >= 4)
    fbboxcarPos = 0;
  fbboxcarSum += fbspeed;

  fbspeed = fbboxcarSum / 4 ;
  
  if(udspeed != PS2_NEUTRAL && udspeed != 127) // 127 is neutral
  {
    if(foreback_active != 0)
    {
      MotorForward(FORWARDBACK_DEVICEID, 0);
      foreback_active = 0;
    }

    // make sure button L1 or L2 is pressed,
    // L1 gives 1/2 speed
    // L2 gives full speed
    // no movement without a button pressed
    if(ps2.readLeftButton1())
      multiplier = updown_range[0];
    else if(ps2.readLeftButton2())
      multiplier = updown_range[1];

    // speed is a value between 0-255, need to convert to a percentage
    //Serial.print("speed value:");
    //Serial.println(udspeed);

    // just in case we stopped to a safe start error (Serial moise)
    // if we are moving, and this is the first move command
    if(udspeed != PS2_NEUTRAL && updown_active == 0)
      exitSafeStart(UPDOWN_DEVICEID);
      
    if(udspeed < PS2_NEUTRAL)
    {
      // up 126 = slowest to 0 = fastest
      udspeed = PS2_NEUTRAL - udspeed;  // covert to a 0 to 126 range

      // our forumla is joystock value * multiplier / 127 == motor speed percentage
      udspeed = (udspeed * multiplier) / PS2_NEUTRAL;  
      if(udspeed > 100)
        udspeed = 100;

      //Serial.print("multiplied speed value:");
      //Serial.println(udspeed);
    
      if(updown_active != udspeed)
      {
        MotorForward(UPDOWN_DEVICEID, udspeed);
        updown_active = udspeed;
      }      
    }
    else if(udspeed > PS2_NEUTRAL)
    {
      // down = 128 = slowes to 255 = fastest
      udspeed -= PS2_NEUTRAL;   // convert to 0 to 126 range
      // our forumla is joystock value * multiplier / 127 == motor speed percentage
      udspeed = (udspeed * multiplier) / PS2_NEUTRAL;  
      if(udspeed > 100)
        udspeed = 100;
      if(updown_active != udspeed)
      {
        MotorReverse(UPDOWN_DEVICEID, udspeed);
        updown_active = udspeed;
      }    
    }
  }
  else
  {
    // all stop
    if(updown_active != 0)
    {
      MotorBrake(UPDOWN_DEVICEID);
      updown_active = 0;
    }

    /*Serial.print("fbspeed: ");
    Serial.println(fbspeed);
    */
    
    // if this is the first move command, send an exit safe start command
    // incase we stopped due to a serial fault (noise)
    if(fbspeed != PS2_NEUTRAL && foreback_active == 0)
      exitSafeStart(FORWARDBACK_DEVICEID);
      
    // forward backward, 127 is neutral
    if(fbspeed != PS2_NEUTRAL && fbspeed != 127)
    {
      // see if button R1 or R2 is pressed for speed multiplication
      if(ps2.readRightButton1() && ps2.readRightButton2())
        multiplier = foreback_range[2];
      else if(ps2.readRightButton1())
        multiplier = foreback_range[0];
      else if(ps2.readRightButton2())
        multiplier = foreback_range[1];

      /*Serial.print("speed value: ");
      Serial.println(fbspeed);
      Serial.print("multiplier: ");
      Serial.println(multiplier);
      */
            
      // speed is a value between 0-255, 127 is neutral
      if(fbspeed < PS2_NEUTRAL)
      {
        // up 126 = slowest to 0 = fastest
        fbspeed = PS2_NEUTRAL - fbspeed;  // covert to a 0 to 126 range
  
        // our forumla is joystock value * multiplier / 127 == motor speed percentage
        fbspeed = (fbspeed * multiplier) / PS2_NEUTRAL;  
        if(fbspeed > 100)
          fbspeed = 100;
    
        if(foreback_active != fbspeed)
          MotorForward(FORWARDBACK_DEVICEID, fbspeed);
      }
      else if(fbspeed > PS2_NEUTRAL)
      {
        // down = 128 = slowes to 255 = fastest
        fbspeed -= PS2_NEUTRAL;   // convert to 0 to 126 range
   
        // our forumla is joystock value * multiplier / 127 == motor speed percentage
        fbspeed = (fbspeed * multiplier) / PS2_NEUTRAL;  
        if(fbspeed > 100)
          fbspeed = 100;
  
        if(foreback_active != fbspeed)
          MotorReverse(FORWARDBACK_DEVICEID, fbspeed);
      }
      foreback_active = fbspeed;
    }
    else
    {
      if(updown_active != 0)
      {
        MotorBrake(UPDOWN_DEVICEID);
        updown_active = 0;
      }
      else if(foreback_active != 0)
      {
        MotorBrake(FORWARDBACK_DEVICEID);
        foreback_active = 0;
      }
    }
  }

  return updown_active > 0 || foreback_active > 0;
}

