#include <math.h>

//PIN VALUES
const int RED = 2;    //RED LED
const int GREEN = 3;  //GREEN LED
const int TPOWER = 8; //THERMISTER POWER
const int TREAD = A0; //THERMISTER READ
const int relay1 = 4; //RELAY PIN
const int BUTTON = 9; //BUTTON PIN

//Temp Target
const double TARGETTEMP = 50.;

//Heating State
bool STATE = false;
bool switched = false; //For button edge detection

//Timer
double startTime = millis();
double previousTime = millis();
double currentTime = 0;

double interval = 500.;

//
#define THERMISTORNOMINAL 10000
#define TEMPERATURENOMINAL 25
#define BCOEFFICIENT 3950 // The beta coefficient of the thermistor (usually 3000-4000)
#define SERIESRESISTOR 10000    


// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(TPOWER, OUTPUT);
  pinMode(relay1, OUTPUT);

  pinMode(BUTTON, INPUT);

  digitalWrite(relay1, LOW);
  
  Serial.begin(9600); //Starts serial connection so temperature can be sent to computer
}

// the loop function runs over and over again forever
void loop() {
  double temperature = readTemperature(); //Reads thermister
  //Printing Stuff
    previousTime = millis();
    printData(temperature);
    LEDControl(temperature);


  //BUTTON CONTROL
  /* if (digitalRead(BUTTON) == HIGH) {
    startTime = millis();
    if (!switched) {
      switched = true;
      STATE = !STATE;
    }
  }
  if (digitalRead(BUTTON) == LOW) {
    switched = false;
  } 

  //Simple Temperature Control
  if (STATE) {
    if (temperature > TARGETTEMP) {
      digitalWrite(relay1, HIGH);
    } else if (temperature <= TARGETTEMP) {
      digitalWrite(relay1, LOW);
    }
  } else {
    digitalWrite(relay1, LOW);
  }
   */
  delay(500);
}

//Function to read temperature
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
  steinhart -= 273.15;                         // convert to C

  return steinhart;
}

//Function to control LEDs
void LEDControl(double temperature) {
//  if (temperature < (TARGETTEMP-3))
//  {
//    digitalWrite(RED, LOW);
//    digitalWrite(GREEN, HIGH);
//  }
//  else if (abs(temperature - TARGETTEMP) <= 3)
//  {
//    digitalWrite(RED, HIGH);
//    digitalWrite(GREEN, HIGH);
//  }
//  else if (
  
  if (temperature > TARGETTEMP) {
    digitalWrite(RED, HIGH);
    digitalWrite(GREEN, LOW);
  } else if (temperature < TARGETTEMP) {
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, HIGH);    
  } else if (temperature == TARGETTEMP) {
    digitalWrite(RED, HIGH);
    digitalWrite(GREEN, HIGH);
  }
}

void printData(double temperature) {
  Serial.print(temperature); //Sends temp to computer
  Serial.print(" ");
  Serial.print((currentTime - startTime)/1000);
  Serial.println("");
}

