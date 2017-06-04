#include <Wire.h>
#include <LiquidCrystal.h>
#define DS1307_I2C_ADDRESS 0x68


///////////////
//LED DISPLAY//
///////////////
class LedDisplay {
private:
  // initialize the library with the numbers of the interface pins
  LiquidCrystal _lcd;
  String _line1 = "";
  String _line2 = ""; 
  bool _toUpdate = false;
public:
  LedDisplay(int pin1, int pin2, int pin3, int pin4, int pin5, int pin6) : _lcd(pin1, pin2, pin3, pin4, pin5, pin6)
  {
    // set up the LCD's number of columns and rows:
    _lcd.begin(16, 2);
  }

  void printLine(String str, int line)
  {
    // set the cursor to column 0, line 1
    // (note: line 1 is the second row, since counting begins with 0):
    if (line == 0 && _line1 != str){_line1 = str; _toUpdate = true; }
    else if (line == 1 && _line2 != str){_line2 = str; _toUpdate = true;}
    if (_toUpdate)
    {
      _lcd.clear();
      _lcd.setCursor(0, 0);
      _lcd.print(_line1);
      _lcd.setCursor(0, 1);
      _lcd.print(_line2);
      _toUpdate = false;
    }
  }
}; 

///////////////
//END LED DISPLAY
//////////////

///////////////
//MOTOR
//////////////
class Motor {
private:
  // initialize the library with the numbers of the interface pins
  int _pin;
  bool _primingRequired;
  bool _running;
  int _doseLength;
  unsigned long _lastDoseStart;
  bool _dosingRequired;
  bool _dosingNow;
   
public:
  Motor(int pin, int doseLength)
  {
    _pin = pin;
    _primingRequired = false;
    _running = false;
    _doseLength = doseLength; //milliseconds
    _lastDoseStart = 0;
    _dosingRequired = false;
    pinMode(_pin, OUTPUT);
  }

  String getState()
  {
    if (_primingRequired && _running){return "p";}
    else if (_dosingRequired && _running) {return "d";}
    else if (!_primingRequired && !_dosingRequired && !_running){return "i";}
    else if (_running){_running = false; return "e";}
    else {_running = false; return "ee";}
  }

  void prime(bool on){_primingRequired = on;}

  void dose(){
    if (!_dosingRequired)
    {
      _dosingRequired = true; _lastDoseStart = millis();
    }
  }

  void tick()
  {
    //UPDATE!
    //decide if dosing is required
    unsigned long now = millis();
    if (_dosingRequired && now >= _lastDoseStart + _doseLength) {_dosingRequired = false;}
    if (now < _lastDoseStart){_dosingRequired= false;}//looped around, panic!

    if (_dosingRequired || _primingRequired) {_running = true;}
    else {_running = false;}
    
    digitalWrite(_pin,_running);    
  }
  
}; 
///////////////
//END MOTOR
//////////////

///////////////
//RTC
//////////////
class RTClock {
private:
  // Convert normal decimal numbers to binary coded decimal
  byte decToBcd(byte val)
  {
    return ( (val/10*16) + (val%10) );
  }
   
  // Convert binary coded decimal to normal decimal numbers
  byte bcdToDec(byte val)
  {
    return ( (val/16*10) + (val%16) );
  }  
public:
  RTClock()
  {
  }

  void setDateDs1307(byte second,        // 0-59
  byte minute,        // 0-59
  byte hour,          // 1-23
  byte dayOfWeek,     // 1-7
  byte dayOfMonth,    // 1-28/29/30/31
  byte month,         // 1-12
  byte year)          // 0-99
  {
    // 1) Sets the date and time on the ds1307
    // 2) Starts the clock
    // 3) Sets hour mode to 24 hour clock
    // Assumes you're passing in valid numbers
    Wire.beginTransmission(DS1307_I2C_ADDRESS);
    Wire.write(0);
    Wire.write(decToBcd(second));    // 0 to bit 7 starts the clock
    Wire.write(decToBcd(minute));
    Wire.write(decToBcd(hour));      // If you want 12 hour am/pm you need to set
    // bit 6 (also need to change readDateDs1307)
    Wire.write(decToBcd(dayOfWeek));
    Wire.write(decToBcd(dayOfMonth));
    Wire.write(decToBcd(month));
    Wire.write(decToBcd(year));
    Wire.endTransmission();
  }
  
  // Gets the date and time from the ds1307
  void getDateDs1307(int *second,
  int *minute,
  int *hour,
  int *dayOfWeek,
  int *dayOfMonth,
  int *month,
  int *year)
  {
    // Reset the register pointer
    Wire.beginTransmission(DS1307_I2C_ADDRESS);
    Wire.write(0);
    Wire.endTransmission();
     
    Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
     
    // A few of these need masks because certain bits are control bits
    *second     = (int)bcdToDec(Wire.read() &0x7f);
    *minute     = (int)bcdToDec(Wire.read());
    *hour       = (int)bcdToDec(Wire.read() &0x3f);  // Need to change this if 12 hour am/pm
    *dayOfWeek  = (int)bcdToDec(Wire.read());
    *dayOfMonth = (int)bcdToDec(Wire.read());
    *month      = (int)bcdToDec(Wire.read());
    *year       = (int)bcdToDec(Wire.read());
  }

  String getDateString()
  {
    int second, minute, hour, dayOfWeek, dayOfMonth, month, year;
    getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
    return String(dayOfMonth) + "/" + String(month) + "/" + String(year);
  }

  // Stops the DS1307, but it has the side effect of setting seconds to 0
  // Probably only want to use this for testing
  /*void stopDs1307()
  {
    Wire.beginTransmission(DS1307_I2C_ADDRESS);
    Wire.write(0);
    Wire.writeWire.writeWire.write(0x80);
    Wire.endTransmission();
  }*/
}; 
///////////////
//END RTC
//////////////

LedDisplay disp(A0, A1, 5, 4, 3, 2);
int redButtonPin = 9;
int yellowButtonPin = 8;
//Button downButton(7, 200);
RTClock rtc;

int count = 0;

const int ledPin = 13;
int ledState = 0;
unsigned long ledStateChangeTime = 0;
int ledPulseInterval = 1000;

const int motor1pin = 11;
const int motor2pin = 12;
int motorState = 0;

Motor m1(motor1pin, 3250);//twice a day = 6.5ml
Motor m2(motor2pin, 1500);//twice a day = 3.0ml


void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(motor1pin, OUTPUT);
  pinMode(motor2pin, OUTPUT);
  
  // Print a message to the LCD.
  //lcd.print("hello, world!");
  ledStateChangeTime = millis(); 
  //byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  Wire.begin();
  Serial.begin(9600);

  
  // Change these values to what you want to set your clock to.
  // You only need to run this the first time you setup your RTC.
  // Set the correct value below and un comment it to run it.
  //rtc.setDateDs1307(00, 37, 17, 5, 30, 9, 16); //sec, min, hour, dayOfWeek, dayOfMonth, month, year
  
}

void loop() {
  //get time
  int second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  rtc.getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  if (second > 59 || minute > 59 || hour > 23)
  {
    //bad clock, abort loop!
    return;
  }
  
  //read priming buttons
  getPrimeStatus();
  
  flashLed();
  
  //print time
  disp.printLine(String(hour) + ":" + String(minute) + ":" + String(second),0);
  
  if (hour == 18 || hour == 20)
  { 
    if (minute == 0)
    {
      if (second == 0)
      {
        m1.dose();
      }
      
      if (second == 5)
      {
        m2.dose();
      }
    }
  }
  
  m1.tick();
  m2.tick();
  
  //print motor status
  disp.printLine("M1:" + m1.getState() + " M2:" + m2.getState(),1);
  delay(50);
}


void flashLed() {
  unsigned long now = millis();
  if (now > (ledStateChangeTime + ledPulseInterval))
  {
    //change led state
    if (ledState == 0) {ledState = 1;}
    else {ledState = 0;}
    ledStateChangeTime = now;
  }
  digitalWrite(ledPin, ledState);
}

void getPrimeStatus()
{
  //Serial.println(yellowButton.getState());
  bool yellowState = digitalRead(yellowButtonPin);
  bool redState = digitalRead(redButtonPin);
  //Serial.print(yellowState);
  //Serial.println(redState);
  m1.prime(yellowState);
  m2.prime(redState);
}
 
