#include <LiquidCrystal.h>

class LEDDisplay {
  private:
    // initialize the library with the numbers of the interface pins
    LiquidCrystal _lcd;
    String _line1 = "";
    String _line2 = "";
    bool _toUpdate = false;
  public:
    LEDDisplay(int pin1, int pin2, int pin3, int pin4, int pin5, int pin6) : _lcd(pin1, pin2, pin3, pin4, pin5, pin6)
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

class Button {
  private:
    bool _pressed;
    int _pin;
  
  public:
    Button(int pin){
      _pressed = false;
      _pin = pin;
    }

    bool getStatus(){
      return digitalRead(_pin);
    }
};

class LED {
  private:
    int _pin;
    bool _state;
    unsigned long _stateChangedTime;
    int _interval;

  public:
    LED(int pin, int interval){
      _pin = pin;
      _state = false;
      _stateChangedTime = millis();
      _interval = interval;
    }

    void flash() {
      unsigned long now = millis();
      if (now > (_stateChangedTime + _interval))
      {
        //change led state
        if (_state == false) {
          _state = true;
        }
        else {
          _state = false;
        }
        _stateChangedTime = now;
      }
      digitalWrite(_pin, _state);
    }

    int getPin(){
      return _pin;
    }
};

class Motor {
  private:
    // initialize the library with the numbers of the interface pins
    int _pin;
    bool _primingRequired;
    bool _running;
    int _doseLength; // dosing rate = 1ml / 1000ms
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

    int getPin() {
      return _pin;
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
        _dosingRequired = true; 
        _lastDoseStart = millis();
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

LEDDisplay disp(A0, A1, 5, 4, 3, 2);

Button redButton(9); // motor2 
Button yellowButton(8); //motor1
LED boardLED(13,1000);

// Motor dosing rate = 1ml/1000ms

// Clear liquid (yellow motor) = easycarbo
// https://www.easylifeaquarium.co.uk/products/freshwater/plant-food/easycarbo
// Aquarium with only a few plants : 1 ml per 100 litres daily
// Aquarium moderately planted : 1 ml per 50 litres daily
// Aquarium heavily planted : 1 ml per 25 litres daily

// Brown liquid (red motor) = profito plat food
// https://www.easylifeaquarium.co.uk/products/freshwater/plant-food/profito
// The dosage is very economical: 10 ml per 100 litres aquarium water every week.
// It's also possible to add 1/7 of the total dosage on a daily basis.
// The colour of the product can vary from dark green to dark brown.
// TIP:
// Use the first couple of weeks about 1/3 of the dosage. 
// That can later be adjusted to attain the best possible growth rate 
// (weekly per 100 litres 10 ml +/- 40%). The best dosage can be determined by 
// observing the youngest leaves of the plant. 
// If they are light green, then the dosage can be increased. 
// When algae should appear and/or no further growth enhancement of 
// the plants can be observed, then the dosage may be reduced. 
// The introduction of some fast-growing plants can indeed be very helpful 
// to prevent algae.
long dailyms = 3600000 * 24; //1 min = 60000ms, 1 hour = 3600000ms, 2 hour = 7200000ms, 3 hours = 10800000ms, 6 hours = 21600000ms

long dailyDoses = 6;

long dosingInterval = dailyms/dailyDoses; 

long easyCarboDaily = 7200; //7.2ml
long profitoDaily = 3000; //3ml

Motor yellowMotor(11, easyCarboDaily/dailyDoses);//(clear liquid carbon)
Motor redMotor(12, profitoDaily/dailyDoses);//(Black plant ferts)


long msUntilDosing = dosingInterval;
unsigned long currentTime = 0;
long totalDoses = 0;

void setup() {
  pinMode(boardLED.getPin(), OUTPUT);
  pinMode(yellowMotor.getPin(), OUTPUT);
  pinMode(redMotor.getPin(), OUTPUT);
  currentTime = millis();
}

void loop() {
  unsigned long oldTime = currentTime;
  currentTime = millis();
  if (currentTime < oldTime) {
    return; //overflowed; panic
  }
  msUntilDosing = msUntilDosing - (currentTime - oldTime);

  //read priming buttons
  yellowMotor.prime(yellowButton.getStatus());
  redMotor.prime(redButton.getStatus());

  //blinky
  boardLED.flash();

  //print countdown
  String dispTime = "";
  if (msUntilDosing >= 60000) {
    dispTime = String(round((msUntilDosing / 1000)/60)) + "m";
  }
  else {
    dispTime = String(round(msUntilDosing / 1000)) + "s";
  }
  disp.printLine("Ferts in: " + dispTime, 0);

  if (msUntilDosing <= 0)
  {
    yellowMotor.dose();
    redMotor.dose();
    msUntilDosing = dosingInterval;
    totalDoses = totalDoses + 1;
  }

   yellowMotor.tick();
   redMotor.tick();

  //print motor status
  disp.printLine("Y:" + yellowMotor.getState() + " R:" + redMotor.getState() + " T: " + (String) totalDoses,1);
  delay(10);
}
