typedef void (* myFunctionPointer) ();

char* szMain [] = {"Up/Down Motor Speed", "Set Current Height", "Set Step Height", "Exit" };
myFunctionPointer menufuncs[] = { motorspeedinit, currentheightinit, stepheightinit, exitmenu };

myFunctionPointer currentfunc = menuMain;
int selectedMenu = 0;

const int sensitivity = 100;
const long autorepeattime = 400;  // # millis between autorepeat events
const int PS2_NEUTRAL = 128;

const int ACTION_NONE = 0;
const int ACTION_SELECT = 1;
const int ACTION_UP = 2;
const int ACTION_DOWN = 3;

extern const int PULSES_PER_INCH;

// used for autorepeat
long lastactiontime = 0;  
short lastaction = 0;
byte select_button_down = 0;

// motor speed variables
int working_updown_range[2] = { 50, 100 };
int working_foreback_range[3] = { 33, 50, 100 };
int motor_speed_item = 0;
extern int updown_range[2];
extern int foreback_range[3];

// current height variables
int current_height_item = 0;
int workingHeadInches;
int workingHeadPartial;

// our mark where we start from, with this we can calc the current position
extern int startEncoderInches;
extern int startEncoderPartial;      // 64ths of an inch

extern long lastscreendraw;

// our current head position, this is calc'd from the start info and the
// current encoder pos
extern int currentHeadInches;
extern int currentHeadPartial;

// current step variables
int step_height_item = 0;
int workingStepInches;
int workingStepPartial;

extern int currentStepInches;
extern int currentStepPartial;
extern long stepCount;

extern Cytron_PS2Shield ps2;

void donothing()
{
  u8g2.clearBuffer();
  u8g2.drawStr(0, 6, "SAWMILL DO NOTHING MENU");  // write something to the internal memory
  u8g2.drawLine(0, 7, 128, 7);
}

void exitmenu()
{
  operation_mode = MODE_IDLE;
  drawMainScreen();
  lastscreendraw = millis();
}


// --------------------------------------
// 
//  Motor speed for fore/back and up
//  and down motors
//
//---------------------------------------
void motorspeedinit()
{
  int i;
  int address = 100;

  // grab current settings from EEPROM
  for(i = 0; i < sizeof(working_updown_range) / sizeof(int); i++)
  {
    EEPROM.get(address, working_updown_range[i]);
    address += sizeof(int);
  }

  for(i = 0; i < sizeof(working_foreback_range) / sizeof(int); i++)
  {
    EEPROM.get(address, working_foreback_range[i]);
    address += sizeof(int);
  }

  drawmotorspeed();
  currentfunc = motorspeedloop;
}

void motorspeedloop()
{
  int action = getaction();
  if(action == ACTION_DOWN)
  {
    if(motor_speed_item < 7)
    { // selection
      motor_speed_item++;
      if(motor_speed_item > 6)
        motor_speed_item = 0;
    }
    else
    {
      // lower current speed value, get multipler values
      int step = 1;
      if(ps2.readLeftButton1())
        step = 5;
      else if(ps2.readLeftButton2())
        step = 10;

      if(motor_speed_item < 9)
      {
        // up/down speed
        int index = motor_speed_item - 7;
        working_updown_range[index] -= step;
        if(working_updown_range[index] < 0)
          working_updown_range[index] = 0;  
      }
      else
      {
        int index = motor_speed_item - 9;
        working_foreback_range[index] -= step;    
        if(working_foreback_range[index] < 0)
          working_foreback_range[index] = 0;
      }
    }
    drawmotorspeed();
    lastaction = -1;
  }
  else if(action == ACTION_UP)
  {
    if(motor_speed_item < 7)
    {
      if(motor_speed_item == 0)
        motor_speed_item = 6;
      else
        motor_speed_item--;
    }
    else
    {
      // raise the current speed value, get multipler values
      int step = 1;
      if(ps2.readLeftButton1())
        step = 5;
      else if(ps2.readLeftButton2())
        step = 10;

      if(motor_speed_item < 9)
      {
        // up/down speed
        int index = motor_speed_item - 7;
        working_updown_range[index] += step;
        if(working_updown_range[index] > 100)
          working_updown_range[index] = 100;  
      }
      else
      {
        int index = motor_speed_item - 9;
        working_foreback_range[index] += step;    
        if(working_foreback_range[index] > 100)
          working_foreback_range[index] = 100;
      }
    }
    drawmotorspeed();
    lastaction = 1;
  }
  else if(action == ACTION_SELECT)
  {
    if(motor_speed_item == 5) // back, exit w/o saving
    {
      StartMenu();
    }
    else if(motor_speed_item == 6)  // save
    {
      int i;
      int address = 100;

      // save current settings to EEPROM
      for(i = 0; i < sizeof(working_updown_range) / sizeof(int); i++)
      {
        EEPROM.put(address, working_updown_range[i]);
        address += sizeof(int);
      }
      
      for(i = 0; i < sizeof(working_foreback_range) / sizeof(int); i++)
      {
        EEPROM.put(address, working_foreback_range[i]);
        address += sizeof(int);
      }

      // and put values back into variable used during motor movement
      memcpy(updown_range, working_updown_range, sizeof(working_updown_range));
      memcpy(foreback_range, working_foreback_range, sizeof(working_foreback_range));

      StartMenu();
    }
    else
    {
      // jump to value
      if(motor_speed_item < 7)
        motor_speed_item += 7;
      else
        motor_speed_item -= 7;
      drawmotorspeed();
    }
  }
}

void drawmotorspeed()
{
  char buffer[10];
  
  u8g2.clearBuffer();
  
  u8g2.setDrawColor(1);
  u8g2.drawStr(0, 6, "SAWMILL MOTOR SPEED MENU");  // write something to the internal memory
  u8g2.drawLine(0, 7, 128, 7);

  // draw color = 0 for reverse text, 1 for regular text
  u8g2.setDrawColor(motor_speed_item != 0);
  u8g2.drawStr(5,15, "Up/Down Speed 1 :");

  u8g2.setDrawColor(motor_speed_item != 7);
  u8g2.drawStr(98, 15, itoa(working_updown_range[0], buffer, 10));
  
  u8g2.setDrawColor(motor_speed_item != 1);
  u8g2.drawStr(5,23, "Up/Down Speed 2 :");

  u8g2.setDrawColor(motor_speed_item != 8);
  u8g2.drawStr(98, 23, itoa(working_updown_range[1], buffer, 10));
  
  u8g2.setDrawColor(motor_speed_item != 2);
  u8g2.drawStr(5,33, "Fwd/Bck Speed 1 :");
  
  u8g2.setDrawColor(motor_speed_item != 9);
  u8g2.drawStr(98, 33, itoa(working_foreback_range[0], buffer, 10));

  u8g2.setDrawColor(motor_speed_item != 3);
  u8g2.drawStr(5,41, "Fws/Bck Speed 2 :");
  
  u8g2.setDrawColor(motor_speed_item != 10);
  u8g2.drawStr(98, 41, itoa(working_foreback_range[1], buffer, 10));

  u8g2.setDrawColor(motor_speed_item != 4);
  u8g2.drawStr(5,49, "Fwd/Bck Speed 3 :");

  u8g2.setDrawColor(motor_speed_item != 11);
  u8g2.drawStr(98, 49, itoa(working_foreback_range[2], buffer, 10));

  u8g2.setDrawColor(motor_speed_item != 5);
  u8g2.drawStr(30,60, "Back");

  u8g2.setDrawColor(motor_speed_item != 6);
  u8g2.drawStr(80,60, "Save");
  
  u8g2.sendBuffer();
}


//----------------------------------------
//
//  Current Height of blade off the bed
//  settings
//
//----------------------------------------
void currentheightloop()
{
  int action = getaction();
  if(action == ACTION_DOWN)
  {
    if(current_height_item < 4)
    { // selection
      current_height_item++;
      if(current_height_item > 3)
        current_height_item = 0;
    }
    else
    {
      // lower current height value, get multipler values
      int step = 1;
      if(ps2.readLeftButton1())
        step = 5;
      else if(ps2.readLeftButton1())
        step = 10;

      if(current_height_item ==  4)
      {
        // whole inches range of 1 to 36
        workingHeadInches -= step;
        if(workingHeadInches < 1)
          workingHeadInches = 1;
      }
      else
      {
        // 64th of inch selection
        workingHeadPartial -= step;    
        if(workingHeadPartial < 0)
          workingHeadPartial = 0;
      }
    }
    drawcurrentheight();
    lastaction = -1;
  }
  else if(action == ACTION_UP)
  {
    if(current_height_item < 4)
    {
      if(current_height_item == 0)
        current_height_item = 3;
      else
        current_height_item--;
    }
    else
    {
      // raise the current speed value, get multipler values
      int step = 1;
      if(ps2.readLeftButton1())
        step = 5;
      else if(ps2.readLeftButton1())
        step = 10;

      if(current_height_item == 4)
      {
        // inches
        workingHeadInches += step;
        if(workingHeadInches > 36)
          workingHeadInches = 36;  
      }
      else
      {
        workingHeadPartial += step;    
        if(workingHeadPartial > 63)
          workingHeadPartial -= 64;  // wrap partial values
      }
    }
    drawcurrentheight();
    lastaction = 1;
  }
  else if(action == ACTION_SELECT)
  {
    if(current_height_item == 2) // back, exit w/o saving
    {
      StartMenu();
    }
    else if(current_height_item == 3)  // save
    {
      startEncoderInches = workingHeadInches;
      startEncoderPartial = workingHeadPartial;

      currentHeadInches = workingHeadInches;
      currentHeadPartial = workingHeadPartial;

      StartMenu();
    }
    else
    {
      // jump to value for editing or back to selection
      if(current_height_item < 2)
        current_height_item += 4;
      else if(current_height_item > 3)
        current_height_item -= 4;
      drawcurrentheight();
    }
  }    
}

void currentheightinit()
{
  currentfunc = currentheightloop;  

  current_height_item = 0;
  
  workingHeadInches = currentHeadInches;
  workingHeadPartial = currentHeadPartial;
  
  drawcurrentheight();
}

void drawcurrentheight()
{
  char buffer[10];
  
  u8g2.clearBuffer();
  
  u8g2.setDrawColor(1);
  u8g2.drawStr(0, 6, "SAWMILL CURRENT HEIGHT MENU");  // write something to the internal memory
  u8g2.drawLine(0, 7, 128, 7);

  u8g2.setDrawColor(current_height_item != 0);
  u8g2.drawStr(5, 25, "       Inches :");

  u8g2.setDrawColor(current_height_item != 4);
  u8g2.drawStr(90, 25, itoa(workingHeadInches, buffer, 10));
  
  u8g2.setDrawColor(current_height_item != 1);
  u8g2.drawStr(5, 35, "64ths of Inch :");

  u8g2.setDrawColor(current_height_item != 5);
  u8g2.drawStr(90, 35, itoa(workingHeadPartial, buffer, 10));

  u8g2.setDrawColor(current_height_item != 2);
  u8g2.drawStr(30,60, "Back");

  u8g2.setDrawColor(current_height_item != 3);
  u8g2.drawStr(80,60, "Save");
  
  u8g2.sendBuffer();
  
}

//---------------------------------------
//
// Step height very similiar to current
// height settings
//
//---------------------------------------
void stepheightinit()
{
  currentfunc = stepheightloop;  

  current_height_item = 0;
  
  workingStepInches = currentStepInches;
  workingStepPartial = currentStepPartial;
  
  drawstepheight();
}

void drawstepheight()
{
  char buffer[10];
  
  u8g2.clearBuffer();
  
  u8g2.setDrawColor(1);
  u8g2.drawStr(0, 6, "SAWMILL STEP HEIGHT MENU");  // write something to the internal memory
  u8g2.drawLine(0, 7, 128, 7);

  u8g2.setDrawColor(step_height_item != 0);
  u8g2.drawStr(5, 25, "       Inches :");

  u8g2.setDrawColor(step_height_item != 4);
  u8g2.drawStr(90, 25, itoa(workingStepInches, buffer, 10));
  
  u8g2.setDrawColor(step_height_item != 1);
  u8g2.drawStr(5, 35, "64ths of Inch :");

  u8g2.setDrawColor(step_height_item != 5);
  u8g2.drawStr(90, 35, itoa(workingStepPartial, buffer, 10));

  u8g2.setDrawColor(step_height_item != 2);
  u8g2.drawStr(30,60, "Back");

  u8g2.setDrawColor(step_height_item != 3);
  u8g2.drawStr(80,60, "Save");
  
  u8g2.sendBuffer();
}

void stepheightloop()
{
  int action = getaction();
  if(action == ACTION_DOWN)
  {
    if(step_height_item < 4)
    { // selection
      step_height_item++;
      if(step_height_item > 3)
        step_height_item = 0;
    }
    else
    {
      // lower current height value, get multipler values
      int step = 1;
      if(ps2.readLeftButton1())
        step = 5;
      else if(ps2.readLeftButton1())
        step = 10;

      if(step_height_item ==  4)
      {
        // whole inches range of 1 to 36
        workingStepInches -= step;
        if(workingStepInches < 1)
          workingStepInches = 1;
      }
      else
      {
        // 64th of inch selection
        workingStepPartial -= step;    
        if(workingStepPartial < 0)
          workingStepPartial = 0;
      }
    }
    drawstepheight();
    lastaction = -1;
  }
  else if(action == ACTION_UP)
  {
    if(step_height_item < 4)
    {
      if(step_height_item == 0)
        step_height_item = 3;
      else
        step_height_item--;
    }
    else
    {
      // raise the current speed value, get multipler values
      int step = 1;
      if(ps2.readLeftButton1())
        step = 5;
      else if(ps2.readLeftButton1())
        step = 10;

      if(step_height_item == 4)
      {
        // inches
        workingStepInches += step;
        if(workingStepInches > 36)
          workingStepInches = 36;  
      }
      else
      {
        workingStepPartial += step;    
        if(workingStepPartial > 63)
          workingStepPartial -= 64;  // wrap partial values
      }
    }
    drawstepheight();
    lastaction = 1;
  }
  else if(action == ACTION_SELECT)
  {
    if(step_height_item == 2) // back, exit w/o saving
    {
      StartMenu();
    }
    else if(step_height_item == 3)  // save
    {
      long pulses;
      
      currentStepInches = workingStepInches;
      currentStepPartial = workingStepPartial;

      // convert inches to encoder pulse count
      pulses = currentStepInches * PULSES_PER_INCH;
      pulses += (currentStepPartial * PULSES_PER_INCH) / 64;
      EEPROM.put(200, pulses);

      stepCount = pulses;
      
      StartMenu();
    }
    else
    {
      // jump to value for editing or back to selection
      if(step_height_item < 2)
        step_height_item += 4;
      else if(step_height_item > 3)
        step_height_item -= 4;
      drawstepheight();
    }
  }      
}

//---------------------------------------
//
// check for Start button press to enter 
// menu mode
//
//---------------------------------------
bool enterMenuMode()
{
#ifdef NO_PS2 || NO_LCD
  if (Serial.available() > 0) 
    return (Serial.read() == 'S');
#else
  return ps2.isStartButtonPressed();
#endif
}


//------------------------------------------
//
// Main menu code 
void drawMenu()
{
  u8g2.clearBuffer();
  
  u8g2.drawStr(0, 6,"  SAWMILL CONTROLLER MENU");  // write something to the internal memory
  u8g2.drawLine(0, 7, 128, 7);

  int y = 15;
  for(int i = 0; i < sizeof(szMain)/sizeof(char*); i++)
  {
    int len = strlen(szMain[i]);
    int x = (25 - len) / 2;
    if(i == selectedMenu)
      u8g2.setDrawColor(0);
    else 
      u8g2.setDrawColor(1);
    u8g2.drawStr(x * 5, y, szMain[i]);
    y += 8;
  }
  u8g2.setDrawColor(1);
  u8g2.sendBuffer();
}

void StartMenu()
{
  Serial.print("Starting the menu\n");
  selectedMenu = 0;
  drawMenu();
  operation_mode = MODE_MENU;
  currentfunc = menuMain;
}

void MenuLoop()
{
  // call our current processor function
  currentfunc();
}

void menuMain()
{
  int action = getaction();
  if(action == ACTION_SELECT)
    currentfunc = menufuncs[selectedMenu];
  else if(action == ACTION_DOWN)
    menuDown();
  else if(action == ACTION_UP)
    menuUp();
}

void menuUp()
{
    lastaction = 1; // move up in menu 
  
    if(selectedMenu == 0)
      selectedMenu = (sizeof(szMain)/sizeof(char*)) - 1;
    else
      selectedMenu--;
    drawMenu();
}

void menuDown()
{
    lastaction = -1; // move down in menu 
    if(selectedMenu == (sizeof(szMain)/sizeof(char*)) - 1)
      selectedMenu = 0;
    else
      selectedMenu++;
    drawMenu();  
}

//----------------------------------------------
//
//  Get the action from the PS2 contoller, takes 
//  care or autorepeat and button events
//
//----------------------------------------------
int getaction()
{
    int action = ACTION_NONE;

    if(select_button_down == 1)
    {
      // look for start button release
      if(ps2.isSelectButtonPressed() == false)
      {
        select_button_down = 0;
        return ACTION_SELECT;
      }
      else
        return ACTION_NONE; // no other action allowed until button release
    }
    
    int pos = ps2.readLeftJoystickY();
    if(pos == PS2_NEUTRAL)
    {
      lastaction = 0;
      //Serial.println("Menu no input.");
      // no movement, but look for a select button press
      if(ps2.isLeftJoystickButtonPressed())
      {
        Serial.print("Called the selected menu funtion\n");
        // select the current menu item
        select_button_down = 1;
        return ACTION_NONE;
      }
    }
    else if(lastaction != 0)
    {
      // last time in we did something, have a couple options
      // movement is in the same direction as last time, then we need to look how long if has been since last menu movement
      // movement is in opposite direction, 
      // no movement, back to neutral
      if(pos < (PS2_NEUTRAL - sensitivity))
      {
        // make sure we are still moving in the same direction
        if(lastaction == 1)
        {
          //Serial.println("Pos < N Looking for autorepeat");
          // yup, see if autorepeat time has past
          long l = millis();
          if(l > (lastactiontime + autorepeattime))
          {
            //Serial.println("Autorepeat up");
            lastactiontime = l;
            return ACTION_UP;
          }
          else
          {
            //Serial.println("Autorepeat time not met");
          }
        }
        else
        {
          lastaction = 0;
          //Serial.println("Pos < N last action reset");
        }
      }
      else if(pos > (PS2_NEUTRAL + sensitivity))
      {
        // make sure we are still moving in the same direction
        if(lastaction == -1)
        {
          //Serial.println("Pos > N Looking for autorepeat");
          // moving in same direction, has autorepeat time elaspsed
          long l = millis();
          if(l > (lastactiontime + autorepeattime))
          {
            //Serial.println("Autorepeat down");
            lastactiontime = l;
            return ACTION_DOWN;
          }
          else
          {
            //Serial.println("Autorepeat time not met");            
          }
        }
        else
        {
          // changed direction, reset and wait for next action
          lastaction = 0;
          //Serial.println("Pos > N last action reset");
        } 
      }
      else
      {
        // back to no movement
        //Serial.println("Pos = N last action reset");
        lastaction = 0;
      }
    }
    else if(pos < (PS2_NEUTRAL - sensitivity))
    {
      lastactiontime = millis();
      return ACTION_UP;
    }
    else if(pos > (PS2_NEUTRAL + sensitivity))
    {
      lastactiontime = millis();
      return ACTION_DOWN;
    }

    return ACTION_NONE;
}


