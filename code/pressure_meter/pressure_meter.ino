#include <Wire.h>
#include <Arduino.h>
#include <BMx280I2C.h>

#define I2C_ADDRESS 0x76

// Relay output pins. NOTE: the relays are active-LOW - driven HIGH = off,
// pulled LOW = activated (see setup() and display_check()).
#define PRESSURE_NORMAL  2
#define PRESSURE_HIGH  3
#define ARROW_UP  4
#define ARROW_DOWN  5

#define HISTORY_SIZE 72  // Assuming one sample every 10 minutes for 12 hours

// Sampling interval (10 minutes) handled by a non-blocking millis() timer.
#define SAMPLE_INTERVAL 600000UL

// Pressure state thresholds in Pa (see comment block above set_pressure_type).
#define PRESSURE_HIGH_PA  102268.9   // >= this -> High pressure (30.20 inHg)
#define PRESSURE_LOW_PA   100914.4   // <= this -> Low pressure  (29.80 inHg)
#define PRESSURE_HYSTERESIS  50.0    // Pa dead-band to stop relay chatter on a boundary

// Minimum change (Pa) between oldest and newest sample to count as a trend.
#define TREND_DEADBAND  50.0

float pressure_sample;
float pressure_history[HISTORY_SIZE];
unsigned long sample_timestamps[HISTORY_SIZE];
int history_index = 0;
int sample_count = 0;            // number of valid samples collected (caps at HISTORY_SIZE)
int pressure_state = 0;          // current pressure state: 1 = high, 0 = normal, -1 = low
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
    static unsigned long lastSample = 0;

    // Non-blocking sample timer: keeps the loop free to do other work between samples.
    // Take a sample immediately on the first pass (lastSample == 0) and every SAMPLE_INTERVAL after.
    if (lastSample != 0 && (millis() - lastSample) < SAMPLE_INTERVAL) {
        return;
    }
    lastSample = millis();

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
    if (sample_count < HISTORY_SIZE) {
        sample_count++;
    }

    analyze_pressure_trend();  // Call the trend analysis function
    set_pressure_type(pressure_sample);
}

float grab_pressure(){
  float result;
  result = bmx280.getPressure();
  return result;
}

/* low pressure is defined as under 29.80 inHg (100914.4 Pa or 1009.144 mb) */
/* normal pressure is defined as between 29.80 and 30.20 inHg (100914.4–102268.9 Pa or 1009.144–1022.689 mb) */
/* high pressure is defined as over 30.20 inHg (102268.9 Pa or 1022.689 mb) */

void set_pressure_type(float pressure){
  // Hysteresis: only switch into a band when clearly past the threshold, and only
  // switch back out once clearly past it in the other direction. Prevents relay
  // chatter when the pressure hovers on a boundary.
  if (pressure >= PRESSURE_HIGH_PA + PRESSURE_HYSTERESIS){      //Enter High pressure state
    pressure_state = 1;
  }
  else if (pressure <= PRESSURE_LOW_PA - PRESSURE_HYSTERESIS){  //Enter Low pressure state
    pressure_state = -1;
  }
  else if (pressure < PRESSURE_HIGH_PA - PRESSURE_HYSTERESIS &&
           pressure > PRESSURE_LOW_PA + PRESSURE_HYSTERESIS){   //Clearly back in the Normal band
    pressure_state = 0;
  }
  // Otherwise we are within the dead-band of a threshold: hold the previous state.

  if (pressure_state == 1){
    digitalWrite(PRESSURE_HIGH, LOW);    // active-LOW: high pressure flap on
    digitalWrite(PRESSURE_NORMAL, HIGH);
    Serial.println("High Pressure");
  }
  else if (pressure_state == -1){
    digitalWrite(PRESSURE_HIGH, HIGH);
    digitalWrite(PRESSURE_NORMAL, HIGH);
    Serial.println("Low Pressure");
  }
  else{
    digitalWrite(PRESSURE_HIGH, HIGH);
    digitalWrite(PRESSURE_NORMAL, LOW);  // active-LOW: normal pressure flap on
    Serial.println("Normal Pressure");
  }
}

void analyze_pressure_trend() {
    // Compare the oldest valid sample with the newest. Once the buffer has wrapped,
    // the oldest valid cell is history_index (the next cell to be overwritten);
    // before that it is index 0. history_index has already been advanced in loop().
    int newest = (history_index + HISTORY_SIZE - 1) % HISTORY_SIZE;
    int oldest = (sample_count >= HISTORY_SIZE) ? history_index : 0;

    float delta = pressure_history[newest] - pressure_history[oldest];

    if (delta > TREND_DEADBAND) {
        Serial.println("Pressure is increasing.");
        digitalWrite(ARROW_DOWN,HIGH); //Set down arrow off
        digitalWrite(ARROW_UP,LOW); //Turn up arrow on
    } else if (delta < -TREND_DEADBAND) {
        Serial.println("Pressure is decreasing.");
        digitalWrite(ARROW_UP,HIGH); // Set up arrow off
        digitalWrite(ARROW_DOWN,LOW);      //Set down arrow on
    } else {
        Serial.println("Pressure is stable.");
        digitalWrite(ARROW_UP,HIGH);   // both arrows off
        digitalWrite(ARROW_DOWN,HIGH);
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
