#include <U8g2lib.h>
#include <U8x8lib.h>

#include <EEPROM.h>

#include <Cytron_PS2Shield.h>

// Serial 1 is for Cytron PS/2 
// Serial 2 is for Pololu SMC

//#define NO_PS2 1
#define NO_LCD 1
#define NO_ENCODER 1 

int ledState = LOW;
long ledStateMillis = 0;

// PS2 pins
//const int PS2_POWER_PIN = 8;
//const int PS2_CLOCK_PIN = 9; 
//const int PS2_ATTENTION_PIN = 10;
//const int PS2_COMMAND_PIN = 11;
//const int PS2_DATA_PIN = 12;

const int LED_PIN = 13;

// current state of our process
const int MODE_IDLE = 1;   // no activity, can go into saw or menu mode
const int MODE_MENU = 2;   // controls operate menu, no motor activity
const int MODE_SAW = 3;     // motor (up/down forward/back) activity only
const int MODE_ACCUSET = 4; // moving sawhead up/down to a set position

int operation_mode = MODE_IDLE;

extern volatile long encoderPos;

int error = 0; 
byte type = 0;
long lastgamepadread = 0; // milliseconds we have been up
long lastscreendraw = 0;  // milliseconds since main screen was drawn

Cytron_PS2Shield ps2;  // Using HW Serial, which was rerouted to serial 3

void setup()
{
	Serial.begin(57600);

	Serial.println("Arduino Controller Version 1.11");
  
	// initialize serial port we use for PS2 interface
  ps2.begin(9600); 

  ps2.reset(1);             //call to reset Shield-PS2
  delay(500);
  ps2.reset(0);
  delay(1000);
  
	pinMode(LED_PIN, OUTPUT); 
	
#ifdef NO_PS2
  Serial.println("Code not using PS2 controler");
#else
  Serial.println("PS2 Shield Initialized on Serial1.");
#endif
 
  lcdInit();
  initMotorControllers();
  initRotaryEncoder();
    
	// if the Simple Motor Controller has automatic baud detection
	// enabled, we first need to send it the byte 0xAA (170 in decimal)
	// so that it can learn the baud rate
  //Serial2.begin(9600);
	//Serial2.write(0xAA); // send baud-indicator byte

}


void loop()
{
	long m;
  int udspeed;
  byte bytes[6];
  int n;
  byte b;
  
  m = millis();

  // if we are running accuset, check encode position first
  if(operation_mode == MODE_ACCUSET)
  {
    // wait for rotary encode to tell us we are at position
    accuset_loop();
  }
  else
  {
  	// only read the controller 10 times a second
  	if(m - lastgamepadread > 100)
  	{
      boolean bb = ps2.readAllButtons();          //read controller
      if(bb == false)
        Serial.println("ReadAllButtons failed");
      lastgamepadread = m;
      
      if(operation_mode == MODE_IDLE)
      {
#ifndef NO_ENCODER
        // first look for a saw head up/down accuset
        if(ps2x.Button(PSB_PAD_UP))
        {
          accuset_up();  
        }
        else if(ps2x.Button(PSB_PAD_DOWN))
        {
          accuset_down();
        }
        else
#endif
          ControlMotors();  // handle any manual motor movement
      }
       
      // look to see if we are to go into menu mode
      if(operation_mode == MODE_IDLE && enterMenuMode())
      {
        Serial.println("Entering Menu Mode");
        // enter menu mode, we won't come back out until the menu is dismissed
        StartMenu();
      }
      else if(operation_mode == MODE_MENU)
      {
        Serial.println("Menu Loop");
        MenuLoop();
      }
  	}
  }    
  
  // draw our screen on a regular basis if we are not in a menu
#ifndef NO_LCD
  if(operation_mode != MODE_MENU)
  {
    Serial.println("Drawing main screen");
    // draw the main screen 3 time a second
    if(m - lastscreendraw > 333)
    {
      drawMainScreen();
      //Serial.print("Encoder Pos ");
      //long l = encoderPos;
      //Serial.println(l);
    }
  }
#endif
 
NextCycle:  
	// toggle the LED on 1 second interval 
	if(m - ledStateMillis > 1000)
	{
		ledStateMillis = m;
		if(ledState == LOW)
			ledState = HIGH;
		else
			ledState = LOW;
		digitalWrite(LED_PIN, ledState);
		//Serial.println("Toggle LED");
	}
}

void FlashLED()
{
	digitalWrite(LED_PIN, HIGH);
	delay(500);
	digitalWrite(LED_PIN, LOW);
	delay(500);   
}


void DisplayErrorCode(int code1, int code2)
{
	// flash a 2 digit code, with 2 sec betwen numbers and 5 between the repeating code
	digitalWrite(LED_PIN, LOW);

	while(1)
	{
		for(int i = 0; i < code1; i++)
		{
			FlashLED();
		}
		delay(1500);

		for(int i = 0; i < code2; i++)
		{
			FlashLED();
		}
		delay(4500);
	}
}

