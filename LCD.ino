#include <stdlib.h>

const int HEIGHT_ROW = 15;
const int STEP_ROW = 23;
const int FWDSPEED_ROW = 31;
const int UDSPEED_ROW = 39;
const int BLANK1_ROW = 47;
const int BLANK2_ROW = 55;
const int STATUS_ROW = 63;

extern const int PULSES_PER_INCH;

extern volatile long encoderPos;

extern int currentHeadInches;
extern int currentHeadPartial;

extern int currentStepInches;
extern int currentStepPartial;

extern int updown_active;
extern int foreback_active;

extern int startEncoderInches;
extern int startEncoderPartial;      // 64ths of an inch
extern long startEncoderPos;

// 0V LCD1 VDD 
// 5V LCD2 VSS
// 53 LCD4 RS
// 51 LCD5 RW
// 52 LCD6 E
// 0V LCD15 PSB //mode select.  Low = SPI, High = Parallel
// 5V LCD19 BLA

U8G2_ST7920_128X64_F_HW_SPI u8g2(U8G2_R0, /* CS=*/ 53, /* reset=*/ U8X8_PIN_NONE);

void lcdInit()
{
#ifndef NO_LCD
  u8g2.begin();
  u8g2.setFont(u8g2_font_5x8_mr);
  u8g2.clearBuffer();

  Serial.print("Drawing the main screen\n");
  drawMainScreen();
#endif
}

void drawMainScreen()
{
  char buffer[16];
  long pos = encoderPos;
  
  u8g2.clearBuffer();
  u8g2.drawStr(0, 6,"  SAWMILL CONTROLLER 1.0");  // write something to the internal memory
  u8g2.drawLine(0, 7, 128, 7);

  // convert our encoder pos to a height
  long diff = startEncoderPos - encoderPos;
  int inches = diff / PULSES_PER_INCH;
  int partial = diff - (inches * PULSES_PER_INCH);
  partial = partial * 64 / 600;
  
  currentHeadInches = startEncoderInches + inches;
  currentHeadPartial = startEncoderPartial + partial;
  if(currentHeadPartial >= 64)
  {
    currentHeadPartial -= 64;
    currentHeadInches++;
  }
  else if(currentHeadPartial < 0)
  {
    currentHeadPartial += 64;
    currentHeadInches--;
  }
  
  u8g2.drawStr(0, 15, "Current Height:");
  sprintf(buffer, "%i-%i/64", currentHeadInches, currentHeadPartial);
  u8g2.drawStr(80, 15, buffer);
  
  u8g2.drawStr(0, 23, "     Step Size:");
  sprintf(buffer, "%i-%i/64", currentStepInches, currentStepPartial);
  u8g2.drawStr(80, 23, buffer);
  
  u8g2.drawStr(0, 31, " Forward Speed: ");
  u8g2.drawStr(80, 31, itoa(foreback_active, buffer, 10));
  u8g2.drawStr(0, 39, " Up/Down Speed: ");
  u8g2.drawStr(80, 39, itoa(updown_active, buffer, 10));
  u8g2.drawStr(0, 47, " Blank Line A");
  u8g2.drawStr(0, 55, " Blank Line B");
  u8g2.drawStr(0, 63, " Status:");

  u8g2.sendBuffer();
}

void updateCurrentHeight(float pos)
{
  char strbuffer[10];
  dtostrf(pos, 6, 3, strbuffer);
  
  u8g2.drawStr(17, HEIGHT_ROW, strbuffer);  
}

void updateStepSize(float pos)
{
  char strbuffer[10];
  dtostrf(pos, 6, 3, strbuffer);
  
  u8g2.drawStr(17, STEP_ROW, strbuffer);    
}

void updateForwardSpeed(int speed)
{
  char strbuffer[10];
  itoa(speed, strbuffer, 10);
  
  u8g2.drawStr(17, FWDSPEED_ROW, strbuffer);    
}

void updateUpDownSpeed(int speed)
{
  char strbuffer[10];
  itoa(speed, strbuffer, 10);
  
  u8g2.drawStr(17, UDSPEED_ROW, strbuffer);    
}

void updateStatusLine(char* status)
{
  u8g2.drawStr(9, STATUS_ROW, status);
}

