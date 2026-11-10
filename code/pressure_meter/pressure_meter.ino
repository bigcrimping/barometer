#include <Wire.h>
#include <Arduino.h>
#include <BMx280I2C.h>

#define I2C_ADDRESS 0x76

// Relay output pins. The relays are active-LOW: see relay() below, which is
// the only place that polarity is handled.
#define PRESSURE_NORMAL  2
#define PRESSURE_HIGH    3
#define ARROW_UP         4
#define ARROW_DOWN       5

#define HISTORY_SIZE 72  // One sample every 10 minutes for 12 hours

// Sampling interval (10 minutes) handled by a non-blocking millis() timer.
#define SAMPLE_INTERVAL 600000UL

// If a measurement fails, times out or reads as garbage, try again after
// this long rather than waiting a whole SAMPLE_INTERVAL.
#define RETRY_INTERVAL 60000UL

// How long to wait for the sensor to produce a reading before giving up.
#define MEASURE_TIMEOUT 2000UL

// Plausible range for a barometer reading near sea level. Anything outside
// this is a bus glitch or a disabled channel, not weather.
#define PRESSURE_MIN_PA  85000.0
#define PRESSURE_MAX_PA  110000.0

// Pressure state thresholds in Pa (see comment block above set_pressure_type).
#define PRESSURE_HIGH_PA  102268.9   // >= this -> High pressure (30.20 inHg)
#define PRESSURE_LOW_PA   100914.4   // <= this -> Low pressure  (29.80 inHg)
#define PRESSURE_HYSTERESIS  50.0    // Pa dead-band to stop relay chatter on a boundary

// Trend ("pressure tendency") is judged over the last 3 hours, the standard
// meteorological window: 18 samples at 10 minutes. The 12-hour buffer is kept
// so the window can be widened without changing the storage.
#define TREND_WINDOW_SAMPLES 18

// Minimum change (Pa) across the trend window to count as rising/falling.
// 100 Pa = 1 hPa in 3 hours, the usual threshold for "steady".
#define TREND_DEADBAND  100.0

enum PressureState { PRESSURE_STATE_LOW, PRESSURE_STATE_NORMAL, PRESSURE_STATE_HIGH };

float pressure_history[HISTORY_SIZE];
int history_index = 0;
int sample_count = 0;            // number of valid samples collected (caps at HISTORY_SIZE)
PressureState pressure_state = PRESSURE_STATE_NORMAL;

const uint8_t RELAY_PINS[] = { PRESSURE_NORMAL, PRESSURE_HIGH, ARROW_DOWN, ARROW_UP };
const uint8_t RELAY_COUNT = sizeof(RELAY_PINS) / sizeof(RELAY_PINS[0]);

BMx280I2C bmx280(I2C_ADDRESS);

// Energise (on = true) or release (on = false) a relay. The boards are
// active-LOW, so this is the one place that inversion lives.
void relay(uint8_t pin, bool on) {
  digitalWrite(pin, on ? LOW : HIGH);
}

void setup() {
  // Configure the 4 relay outputs and make sure every relay starts released.
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    relay(RELAY_PINS[i], false);
  }

  Serial.begin(9600);
  Wire.begin();

  if (!bmx280.begin()) {
    Serial.println("begin() failed. check your BMx280 Interface and I2C Address.");
    sensor_fault();
  }

  bmx280.resetToDefaults();
  delay(10);  // datasheet: 2 ms start-up after a soft reset before registers are writable
  bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
  bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);
  if (bmx280.isBME280()) {
    // Only a BME280 has a humidity channel; the library ignores this on a BMP280.
    bmx280.writeOversamplingHumidity(BMx280MI::OSRS_H_x16);
  }

  display_check();
}

// Sensor not found: flash the two arrows alternately forever so the fault
// looks different from any real weather state.
void sensor_fault() {
  while (1) {
    relay(ARROW_UP, true);
    relay(ARROW_DOWN, false);
    delay(500);
    relay(ARROW_UP, false);
    relay(ARROW_DOWN, true);
    delay(500);
  }
}

void loop() {
  static unsigned long lastSample = 0;
  static unsigned long interval = 0;   // 0 = sample immediately on first pass

  // Non-blocking sample timer: keeps the loop free to do other work between samples.
  if ((millis() - lastSample) < interval) {
    return;
  }
  lastSample = millis();
  interval = SAMPLE_INTERVAL;

  // Library 1.2.3 always returns true here; kept for forward compatibility.
  if (!bmx280.measure()) {
    Serial.println("could not start measurement, is a measurement already running?");
    interval = RETRY_INTERVAL;
    return;
  }

  // Wait for the reading with a deadline: an I2C glitch used to leave this
  // loop spinning forever with the display frozen.
  unsigned long waitStart = millis();
  while (!bmx280.hasValue()) {
    if (millis() - waitStart > MEASURE_TIMEOUT) {
      Serial.println("timed out waiting for the sensor, skipping this sample");
      interval = RETRY_INTERVAL;
      return;
    }
    delay(100);
  }

  float pressure_sample = bmx280.getPressure();

  // getPressure() returns NAN if the channel is disabled, and a glitched bus
  // read compensates to nonsense. Neither belongs in the history: a NaN would
  // fail every comparison and leave the display stuck for 3 hours.
  if (isnan(pressure_sample) ||
      pressure_sample < PRESSURE_MIN_PA || pressure_sample > PRESSURE_MAX_PA) {
    Serial.print("implausible reading (");
    Serial.print(pressure_sample);
    Serial.println(" Pa), skipping this sample");
    interval = RETRY_INTERVAL;
    return;
  }

  Serial.print("Pressure: ");
  Serial.print(pressure_sample / 100.0);
  Serial.println(" hPa");

  pressure_history[history_index] = pressure_sample;
  history_index = (history_index + 1) % HISTORY_SIZE; // Circular buffer index
  if (sample_count < HISTORY_SIZE) {
    sample_count++;
  }

  analyze_pressure_trend();
  set_pressure_type(pressure_sample);
}

/* low pressure is defined as under 29.80 inHg (100914.4 Pa or 1009.144 mb) */
/* normal pressure is defined as between 29.80 and 30.20 inHg (100914.4-102268.9 Pa or 1009.144-1022.689 mb) */
/* high pressure is defined as over 30.20 inHg (102268.9 Pa or 1022.689 mb) */

void set_pressure_type(float pressure) {
  // Hysteresis: only switch into a band when clearly past the threshold, and only
  // switch back out once clearly past it in the other direction. Prevents relay
  // chatter when the pressure hovers on a boundary.
  if (pressure >= PRESSURE_HIGH_PA + PRESSURE_HYSTERESIS) {
    pressure_state = PRESSURE_STATE_HIGH;
  }
  else if (pressure <= PRESSURE_LOW_PA - PRESSURE_HYSTERESIS) {
    pressure_state = PRESSURE_STATE_LOW;
  }
  else if (pressure < PRESSURE_HIGH_PA - PRESSURE_HYSTERESIS &&
           pressure > PRESSURE_LOW_PA + PRESSURE_HYSTERESIS) {
    pressure_state = PRESSURE_STATE_NORMAL;
  }
  // Otherwise we are within the dead-band of a threshold: hold the previous state.

  switch (pressure_state) {
    case PRESSURE_STATE_HIGH:
      relay(PRESSURE_HIGH, true);
      relay(PRESSURE_NORMAL, false);
      Serial.println("High Pressure");
      break;
    case PRESSURE_STATE_LOW:
      // Neither flap energised. The indicator's de-energised legend shows
      // LOW, so this is intentional.
      relay(PRESSURE_HIGH, false);
      relay(PRESSURE_NORMAL, false);
      Serial.println("Low Pressure");
      break;
    case PRESSURE_STATE_NORMAL:
      relay(PRESSURE_HIGH, false);
      relay(PRESSURE_NORMAL, true);
      Serial.println("Normal Pressure");
      break;
  }
}

void analyze_pressure_trend() {
  // Compare the newest sample with the one TREND_WINDOW_SAMPLES back (3 h).
  // Until that many samples exist, compare with the oldest we have.
  // history_index has already been advanced in loop().
  if (sample_count < 2) {
    return;  // nothing to compare against yet; arrows stay off
  }

  int span = sample_count - 1;
  if (span > TREND_WINDOW_SAMPLES) {
    span = TREND_WINDOW_SAMPLES;
  }
  int newest = (history_index + HISTORY_SIZE - 1) % HISTORY_SIZE;
  int oldest = (newest + HISTORY_SIZE - span) % HISTORY_SIZE;

  float delta = pressure_history[newest] - pressure_history[oldest];

  Serial.print("Change over last ");
  Serial.print(span * 10);
  Serial.print(" min: ");
  Serial.print(delta / 100.0);
  Serial.println(" hPa");

  if (delta > TREND_DEADBAND) {
    Serial.println("Pressure is increasing.");
    relay(ARROW_DOWN, false);
    relay(ARROW_UP, true);
  } else if (delta < -TREND_DEADBAND) {
    Serial.println("Pressure is decreasing.");
    relay(ARROW_UP, false);
    relay(ARROW_DOWN, true);
  } else {
    Serial.println("Pressure is stable.");
    relay(ARROW_UP, false);
    relay(ARROW_DOWN, false);
  }
}

// Power-on self test: pulse each relay in turn so every flap is seen to move.
void display_check() {
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    relay(RELAY_PINS[i], false);
  }

  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    delay(750);
    relay(RELAY_PINS[i], true);
    delay(750);
    relay(RELAY_PINS[i], false);
  }
  delay(750);
}
