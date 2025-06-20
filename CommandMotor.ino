void exitSafeStart(byte deviceId)
{
  Serial2.write(0xAA);
  Serial2.write(deviceId);
  Serial2.write(0x03);
}

// cause error condtion, only use in emergency
void MotorStop(byte deviceId)
{
  Serial2.write(0xAA);
  Serial2.write(deviceId);
  Serial2.write(0x60);
  Serial.println("Motor Stop");
}

// slow down noramally with the specified brake value 
// 0 no brake
//  to
// 32 maximum brakes (hard stop
void MotorBrake(byte deviceId)
{
  Serial2.write(0xAA);
  Serial2.write(deviceId);
  Serial2.write(0x12);
  Serial2.write(0x10);  // 16 brake value; middle of the road
  Serial.print("Motor Brake");
}

void MotorForward(byte deviceId, int percentage)
{
  Serial2.write(0xAA);
  Serial2.write(deviceId);
  Serial2.write(0x05);
  Serial2.write((byte)0x00);
  Serial2.write(percentage);  
  Serial.print("Motor Forward ");
  Serial.println(percentage);
}

void MotorReverse(byte deviceId, byte percentage)
{

  Serial2.write(0xAA);
  Serial2.write(deviceId);
  Serial2.write(0x06);
  Serial2.write((byte)0x00);
  Serial2.write(percentage);  
  Serial.print("Motor Reverse ");
  Serial.println(percentage);
}

