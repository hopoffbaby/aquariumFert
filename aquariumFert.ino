#include <LiquidCrystal.h>

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
      if (line == 0 && _line1 != str) {
        _line1 = str;
        _toUpdate = true;
      }
      else if (line == 1 && _line2 != str) {
        _line2 = str;
        _toUpdate = true;
      }
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
      if (_primingRequired && _running) {
        return "p";
      }
      else if (_dosingRequired && _running) {
        return "d";
      }
      else if (!_primingRequired && !_dosingRequired && !_running) {
        return "i";
      }
      else if (_running) {
        _running = false;
        return "e";
      }
      else {
        _running = false;
        return "ee";
      }
    }

    void prime(bool on) {
      _primingRequired = on;
    }

    void dose() {
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
      if (_dosingRequired && now >= _lastDoseStart + _doseLength) {
        _dosingRequired = false;
      }
      if (now < _lastDoseStart) {
        _dosingRequired = false; //looped around, panic!
      }

      if (_dosingRequired || _primingRequired) {
        _running = true;
      }
      else {
        _running = false;
      }

      digitalWrite(_pin, _running);
    }

};
///////////////
//END MOTOR
//////////////

LedDisplay disp(A0, A1, 5, 4, 3, 2);
int redButtonPin = 9;
int yellowButtonPin = 8;

int count = 0;

const int ledPin = 13;
int ledState = 0;
unsigned long ledStateChangeTime = 0;
int ledPulseInterval = 1000;

const int motor1pin = 11;
const int motor2pin = 12;
int motorState = 0;

// dosing rate = 1ml / 1000ms
Motor m1(motor1pin, 812);//twice a day = 6.5ml daily = 6500ms/day
Motor m2(motor2pin, 375);//twice a day = 3.0ml daily = 3000ms/day

long dosingInterval = 10800000; //1 min = 60000ms, 1 hour = 3600000ms, 3 hours = 10800000ms, 6 hours = 21600000ms
long millisUntilDosing = dosingInterval;
unsigned long timeNow = 0;

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(motor1pin, OUTPUT);
  pinMode(motor2pin, OUTPUT);

  ledStateChangeTime = millis();
  Serial.begin(9600);
  timeNow = millis();
}

void loop() {
  unsigned long oldTime = timeNow;
  timeNow = millis();
  if (timeNow < oldTime) {
    return; //overflowed; panic
  }
  millisUntilDosing = millisUntilDosing - (timeNow - oldTime);

  //read priming buttons
  getPrimeStatus();

  flashLed();

  //print countdown
  disp.printLine("Ferts in: " + String(round(millisUntilDosing / 1000)) + "s", 0);

  if (millisUntilDosing <= 0)
  {
    m1.dose();
    m2.dose();
    millisUntilDosing = dosingInterval;
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
    if (ledState == 0) {
      ledState = 1;
    }
    else {
      ledState = 0;
    }
    ledStateChangeTime = now;
  }
  digitalWrite(ledPin, ledState);
}

void getPrimeStatus()
{
  bool yellowState = digitalRead(yellowButtonPin);
  bool redState = digitalRead(redButtonPin);
  m1.prime(yellowState);
  m2.prime(redState);
}
