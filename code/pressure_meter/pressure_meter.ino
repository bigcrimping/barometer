#include <Wire.h>
#include <Arduino.h>
#include <BMx280I2C.h>

#define I2C_ADDRESS 0x76
#define PRESSURE_NORMAL  2
#define PRESSURE_HIGH  3
#define ARROW_UP  4
#define ARROW_DOWN  5
#define HISTORY_SIZE 72  // Assuming one sample every 10 minutes for 12 hours

float pressure_sample;
float pressure_history[HISTORY_SIZE];
unsigned long sample_timestamps[HISTORY_SIZE];
int history_index = 0;
float pressure;

BMx280I2C bmx280(I2C_ADDRESS);

void setup() {

  // Configure 4 GPIO as outputs for the 4 relays controlling the   
  pinMode(PRESSURE_NORMAL, OUTPUT); 
  pinMode(PRESSURE_HIGH, OUTPUT); 
  pinMode(ARROW_DOWN, OUTPUT); 
  pinMode(ARROW_UP, OUTPUT);
  digitalWrite(PRESSURE_NORMAL, HIGH);
  digitalWrite(PRESSURE_HIGH, HIGH);
  digitalWrite(ARROW_DOWN, HIGH);
  digitalWrite(ARROW_UP, HIGH); 
  
  Serial.begin(9600);
  Wire.begin();
	
  if (!bmx280.begin())
	{
		Serial.println("begin() failed. check your BMx280 Interface and I2C Address.");
		while (1);
	}
  
  bmx280.resetToDefaults();
	bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
	bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);
  bmx280.writeOversamplingHumidity(BMx280MI::OSRS_H_x16);
		
  display_check();
}

void loop() {
    if (!bmx280.measure()) {
        Serial.println("could not start measurement, is a measurement already running?");
        return;
    }
    do {
        delay(100);
    } while (!bmx280.hasValue());

    pressure_sample = grab_pressure();
    pressure_history[history_index] = pressure_sample;
    sample_timestamps[history_index] = millis(); // Store the timestamp

    history_index = (history_index + 1) % HISTORY_SIZE; // Circular buffer index

    analyze_pressure_trend();  // Call the trend analysis function
    set_pressure_type(pressure_sample);
    
    delay(600000);

}

float grab_pressure(){
  float result;
  result = bmx280.getPressure();
  return result;
}

float grab_temperature(){
  float result;
  result = bmx280.getTemperature();
  return result;
}

/* high pressure is defined as over 30.20 inHg (102268.9 Pa or 1022.689 mb) */
/* normal pressure is defined as between 29.80 and 30.20 inHg (100914.4–102268.9 Pa or 1022.689–1009.144 mb):*/
/* low pressure is defined as under 29.80 inHg (100914.4 Pa or 1009.144 mb): */

float set_pressure_type(float pressure){
  if (pressure >= 102268.9){               //Define High pressure state
    digitalWrite(PRESSURE_HIGH, LOW);
    digitalWrite(PRESSURE_NORMAL, HIGH);
    Serial.println("High Pressure");
  }
  else if (pressure <= 100914.4){          //Define Low pressure state
    digitalWrite(PRESSURE_HIGH, HIGH);
    digitalWrite(PRESSURE_NORMAL, HIGH);
    Serial.println("Low Pressure");
  }
  else{
    digitalWrite(PRESSURE_HIGH, HIGH);     //Define Normal pressure state
    digitalWrite(PRESSURE_NORMAL, LOW); 
    Serial.println("Normal Pressure");   
  }
}

void analyze_pressure_trend() {
    float start_pressure = pressure_history[history_index];
    float end_pressure = pressure_history[(history_index + HISTORY_SIZE - 1) % HISTORY_SIZE];

    if (start_pressure < end_pressure) {
        Serial.println("Pressure is increasing.");
        digitalWrite(ARROW_DOWN,HIGH); //Set down arrow off 
        digitalWrite(ARROW_UP,LOW); //Turn up arrow on
    } else if (start_pressure > end_pressure) {
        Serial.println("Pressure is decreasing.");
        digitalWrite(ARROW_UP,HIGH); // Set up arrow off
        digitalWrite(ARROW_DOWN,LOW);      //Set down arrow on 
    } else {
        Serial.println("Pressure is stable.");
    }
}

void display_check(){
    
    digitalWrite(ARROW_DOWN, HIGH);
    digitalWrite(ARROW_UP,HIGH);
    digitalWrite(PRESSURE_NORMAL,HIGH);
    digitalWrite(PRESSURE_HIGH,HIGH); 

    delay(750);
    digitalWrite(PRESSURE_NORMAL,LOW);
    delay(750);
    digitalWrite(PRESSURE_NORMAL,HIGH);

    delay(750);
    digitalWrite(PRESSURE_HIGH,LOW);
    delay(750);
    digitalWrite(PRESSURE_HIGH,HIGH);
    delay(750);

    delay(750);
    digitalWrite(ARROW_DOWN,LOW);
    delay(750);
    digitalWrite(ARROW_DOWN,HIGH);

    delay(750);
    digitalWrite(ARROW_UP,LOW);
    delay(750);
    digitalWrite(ARROW_UP,HIGH);
    delay(750);

}
