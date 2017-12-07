#include <math.h>

// PIN VALUES
const int RED = 2;      // RED LED
const int GREEN = 3;    // GREEN LED
const int TPOWER = 8;   // THERMISTOR POWER
const int TREAD = A0;   // THERMISTOR READ
const int relay1 = 7;   // RELAY PIN
const int BUTTON = 9;   // BUTTON PIN

double temperature = 0; // Setup variable to store current temperature
double startTemp;       // Setup variable for finding temp to change between regime 1 and 2.
double stopTemp;        // Cut off temperature for initial heating

// Heating State
bool STATE = false;     // For turning on and off heating. Backup for safety.
bool switched = false;  // For button edge detection

// Regime 0 is not running
// Regime 1 is heating to 30ºish degrees
// Regime 2 is coasting to 60ºC
// Regime 3 is holding temp at 60ºC
int regime = 0;

// Timer
double startTime = millis();
double intervalTime = millis();
double currentTime = 0;
double steadyState;
bool time = false;

#define THERMISTORPIN A0         
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000       

// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(TPOWER, OUTPUT);
  pinMode(relay1, OUTPUT);

  pinMode(BUTTON, INPUT_PULLUP);

  digitalWrite(relay1, LOW);
 
  Serial.begin(9600); //Starts serial connection so temperature can be sent to computer
}

// the loop function runs over and over again forever
void loop() {
  
  // BUTTON CONTROL
  if (digitalRead(BUTTON) == LOW) 
  {
    startTime = millis();
    startTemp = readTemperature();
    if (!switched) 
    {
      switched = true;
      STATE = !STATE; // if the button is pushed when the STATE is already true (the device is running), it will be set to false and all heating will stop
      if (!STATE)
      {
        regime = 0;
        digitalWrite(relay1, LOW);
      }
    }
  }
  
  if (digitalRead(BUTTON) == HIGH) // switched is only true while the user is actively pushing the button
  {
    switched = false;
  }

  temperature = readTemperature(); //Reads thermistor

  if (startTemp < 57 && startTemp > 38)
  {
    regime = 3;
  }

  if(temperature < 0) // executes if there is an error in temperature reading or in circuitry
  {
    STATE = false;
    regime = 0;
    digitalWrite(relay1, LOW);
  }
  
  // Printing Stuff
  if (millis() - intervalTime >= 500.)
  {
    intervalTime = millis();
    printData(temperature);
  }
  
  LEDControl();

  if (STATE) // only executes if button has been pushed and kill command has not been given
  {
    if (abs(startTemp - 60) <= 3 || startTemp > (60+3)) // If start temp is already hot, program skips to steady state regime
    {
      regime = 3;
      stopTemp = startTemp;
      Serial.print("Stop temperature: ");
      Serial.print(stopTemp);
      Serial.println("");
      digitalWrite(relay1, LOW);
      if (!time)
      {
        steadyState = millis(); // starts 60 second timer
        time = true;
      }
    }
    if (regime == 0)
    {
      startTemp = temperature;
      regime = 1;
      Serial.print("Stop temperature: ");
      stopTemp = 0.713*startTemp + 12;
      Serial.print(stopTemp);
      Serial.println("");
      digitalWrite(relay1, HIGH); // starts heating
    }
    else if (regime == 1)
    {
      if (temperature > stopTemp)
      {
        regime = 2;
        digitalWrite(relay1, LOW);
      }
    }
    else if (regime == 2)
    {
      if (abs(temperature - 60) <= 5)
      {
        regime = 3;
        steadyState = millis(); // starts 60 second timer
      }
    }
    else if (regime == 3)
    {
      // Duty cycle (10 seconds)
      digitalWrite(relay1, HIGH);
      Serial.print("    On!     ");
      Serial.println("");
      delay(70);
      digitalWrite(relay1, LOW);
      Serial.print("    Off!     ");
      Serial.println("");
      delay(9930);
      
      if ( (millis() - steadyState) >= 60000 )
      {
        steadyState = millis(); // resets timer
        if(digitalRead(RED) == HIGH)
        {
          digitalWrite(RED, LOW);
        }
        else
        {
          digitalWrite(RED, HIGH);
        }
      }
    }
  } 
}

// Function to read temperature
float readTemperature() {
  digitalWrite(TPOWER, HIGH);       // Powers the thermister so that it can be read
  int tValue = analogRead(TREAD);   // Reads the voltage in the middle of the voltage devider
  currentTime = millis();
  digitalWrite(TPOWER, LOW);        // Turns off power to the thermister so that it doesn't heat up
  double tResistance = SERIESRESISTOR * (1023./tValue - 1); // Converts the ADC value from between 0-1023 to the value of the resistance of the thermister

  float steinhart;                                  //ALL MATH EXPLAINED BELOW
  steinhart = tResistance / THERMISTORNOMINAL;      // (R/Ro)
  steinhart = log(steinhart);                       // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                        // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                      // Invert
  steinhart -= 273.15;                              // Convert from K to C

  return steinhart;
}

// Function to control LEDs
void LEDControl() {
  if (regime != 0)
  {
    digitalWrite(RED, HIGH);
  }
  else
  {
    digitalWrite(RED, LOW);
  }

  if (regime == 3)
  {
    digitalWrite(GREEN, HIGH);
  }
  else 
  {
    digitalWrite(GREEN, LOW);
  }
}

void printData(double temperature) {
  // Printing State and Regime for troubleshooting
  Serial.print(STATE);
  Serial.print(" ");
  Serial.print(regime);
  Serial.print(" ");

  // Printing Temperature and Time for data
  Serial.print((currentTime - startTime)/1000);
  Serial.print(" ");
  Serial.print(temperature); //Sends temp to computer
  Serial.println("");
}

