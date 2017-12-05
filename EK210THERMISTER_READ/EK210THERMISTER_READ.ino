#include <math.h>

// PIN VALUES
const int RED = 2;      // RED LED
const int GREEN = 3;    // GREEN LED
const int TPOWER = 8;   // THERMISTOR POWER
const int TREAD = A0;   // THERMISTOR READ
const int relay1 = 7;   // RELAY PIN
const int BUTTON = 9;   // BUTTON PIN

double temperature = 0; // Setup variable to store current temperature
double oldTemp;         // Setup variable to store previous variable for temp change.
double startTemp;       // Setup variable for finding temp to change between regime 1 and 2.

// Heating State
bool STATE = false;     // For turning on and off heating. Backup for safety.
bool switched = false;  // For button edge detection

// Regime 0 is not running
// Regime 1 is heating to 30ºish degrees
// Regime 2 is coasting to 60ºC
// Regime 3 is holding temp for 60 seconds
int regime = 0;

int decreaseCount = 0;  // Counts number of cycles the temperature decreases

// Timer
double startTime = millis();
double previousTime = millis();
double intervalTime = millis();
double currentTime = 0;
double steadyState;

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
  digitalWrite(relay1, LOW);
 
  Serial.begin(9600); //Starts serial connection so temperature can be sent to computer
}

// the loop function runs over and over again forever
void loop() {
  // BUTTON CONTROL
  if (digitalRead(BUTTON) == LOW) 
  {
    startTime = millis();
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

  oldTemp = temperature;
  temperature = readTemperature(); //Reads thermistor

  if(temperature < 0)
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
    if (regime == 0)
    {
      startTemp = temperature;
      regime = 1;
      Serial.print("Stop temperature: ");
      Serial.print(0.507*startTemp + 18);
      Serial.println("");
      digitalWrite(relay1, HIGH); // starts heating
    }
    else if (regime == 1)
    {
      if(temperature > 36)
      {
        if (temperature > startTemp + 3)
        {
          regime = 2;
          digitalWrite(relay1, LOW);
        }
      }
      else if (temperature > (0.507*startTemp + 18)) // stop = 0.507 *start + 21.13 is fit found from current bad data, rounded down for insulation (foam)
      {
        regime = 2;
        digitalWrite(relay1, LOW);
      }
    }
    else if (regime == 2)
    {
      /* if ( (temperature - oldTemp) < 0  && temperature < 60) // executes if coasting doesn't work; should only execute if something went wrong
      {
        decreaseCount += 1;
        if (decreaseCount > 100) {
          decreaseCount = 0;
          double diff = 60 - temperature;
          digitalWrite(relay1, HIGH);
          delay(10000 * diff/5); // heats for 10 seconds for every 5ºC increase that is needed
          digitalWrite(relay1, LOW);
        }
      }
      else
      {
        decreaseCount = 0;
      } */
      if (abs(temperature - 60) <= 5)
      {
        regime = 3;
        steadyState = millis(); // starts 60 second timer
      }
    }
    else if (regime == 3)
    {
      if ( (millis() - steadyState) >= 60000 )
      {
        regime = 0;
        digitalWrite(relay1, LOW);
        STATE = false;
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
  Serial.print(temperature); //Sends temp to computer
  Serial.print(" ");
  Serial.print((currentTime - startTime)/1000);
  Serial.println("");
}

