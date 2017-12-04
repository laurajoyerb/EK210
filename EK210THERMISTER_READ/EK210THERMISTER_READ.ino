#include <math.h>

// PIN VALUES
const int RED = 2;    // RED LED
const int GREEN = 3;  // GREEN LED
const int TPOWER = 8; // THERMISTOR POWER
const int TREAD = A0; // THERMISTOR READ
const int relay1 = 7; // RELAY PIN
const int BUTTON = 9; // BUTTON PIN

double temperature = 0;
double oldTemp;

// Heating State
bool STATE = false;
bool switched = false; //For button edge detection

// Regime 0 is not running
// Regime 1 is essentially a dummy stage too go to regime 99 because it wasn't working
// Regime 99 is heating to 30ºish degrees
// Regime 2 is coasting to 60ºC
// Regime 3 is holding temp for 60 seconds
// Regime 4 is cooling back down
int regime = 0;

// Timer
double startTime = millis();
double previousTime = millis();
double currentTime = 0;
double steadyState;

long interval = 500.;

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
  Serial.print("              Running the setup function!                  ");
  Serial.println("");
}

// the loop function runs over and over again forever
void loop() {
  
  Serial.print(STATE);
  Serial.print(" ");
  Serial.print(regime);
  Serial.println("");
  
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
  
  // Printing Stuff
  printData(temperature);

  LEDControl(temperature);

  if (STATE) // only executes if button has been pushed and kill command has not been given
  {
    if (regime == 0)
    {
      regime = 1;
      digitalWrite(relay1, HIGH); // starts heating
    }
    else if (regime == 1)
    {
      if (temperature > 20)
      {
        regime = 99;
        Serial.print("              CHANGE TO REGIME 10               ");
        digitalWrite(relay1, HIGH); // keeps power on
      }
    }
    else if (regime == 99)
    {
      if (temperature > 30)
      {
        regime = 2;
        Serial.print("              CHANGE TO REGIME 2               ");
        digitalWrite(relay1, LOW); // turns off power to begin coasting
      }
    }
    else if (regime == 2)
    {
//      if ( (temperature - oldTemp) < 0  && temperature < 60) // executes if coasting doesn't work; should only execute if something went wrong
//      {
//        double diff = 60 - temperature;
//        digitalWrite(relay1, HIGH);
//        delay(10000 * diff/5); // heats for 10 seconds for every 5ºC increase that is needed
//        digitalWrite(relay1, LOW);
//      }
      if (abs(temperature - 60) <= 3)
      {
        regime = 3;
        steadyState = millis(); // starts 60 second timer
      }
    }
    else if (regime == 3)
    {
      if ( (steadyState - millis()) >= 60000 )
      {
        regime = 4;
        digitalWrite(relay1, LOW);
      }
    }
    else if (regime == 4)
    {
      if (temperature < 40)
      {
        regime = 0;
      }
    }
  } 
}

// Function to read temperature
float readTemperature() {
  digitalWrite(TPOWER, HIGH); //powers the thermister so that it can be read
  int tValue = analogRead(TREAD); //reads the voltage in the middle of the voltage devider
  currentTime = millis();
  digitalWrite(TPOWER, LOW);  //turns off power to the thermister so that it doesn't heat up
  double tResistance = SERIESRESISTOR * (1023./tValue - 1); //converts the ADC value from between 0-1023 to the value of the resistance of the thermister

  float steinhart;
  steinhart = tResistance / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15; 

  return steinhart;
}

// Function to control LEDs
void LEDControl(double temperature) {
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
  Serial.print(temperature); //Sends temp to computer
  Serial.print(" ");
  Serial.print((currentTime - startTime)/1000);
  Serial.println("");
}

