int buzzer=8;//Set digital IO foot control buzzer
void setup()
{
pinMode(buzzer,OUTPUT);//Set up the digital IO foot mode, the OUTPUT for output
}
void loop()
{
  unsigned char i;//define variable

  for (i=0; i != 100; i++)
  {
    digitalWrite(buzzer,HIGH);//have voice 
    delayMicroseconds(1000);//delay 1ms 
    digitalWrite(buzzer,LOW);//do not have voice 
    delayMicroseconds(1000);//de;
  }
  while (1)
    continue;
}

