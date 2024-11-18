#include <GyverDS18.h>
#include <GyverTM1637.h>
#define SENSOR_PIN 3
#define POTENT_PIN A7
#define VALVE_OPEN_PIN 9
#define VALVE_CLOSE_PIN 10
#define DISPLAY_TEMP_CLK 5
#define DISPLAY_TEMP_NOW_DIO 6
#define DISPLAY_TEMP_REQ_DIO 7
#define coolant_temp_min 30
#define coolant_temp_max 65
#define close_relay 1
#define open_relay 0
GyverDS18Single sensor_coolant_temp(SENSOR_PIN);
GyverTM1637 display_temp_now(DISPLAY_TEMP_CLK, DISPLAY_TEMP_NOW_DIO);
GyverTM1637 display_temp_req(DISPLAY_TEMP_CLK, DISPLAY_TEMP_REQ_DIO);

int coolant_temp_now = 0;
int coolant_temp_difference = 0;
int coolant_temp_difference_max = 0;
int coolant_temp_required = 0;
int coolant_temp_required_range = 0;

bool temp_sensor_failure = false;

bool valve_is_opening = false;
bool valve_is_closing = false;
unsigned long valve_started_action_last_time = 0;
unsigned int valve_action_time_1000 = 1000;
unsigned long valve_finished_action_last_time = 0;
unsigned int valve_delay_time_1000 = 1000;

unsigned int current_time = 0;

bool test_mode = false;

// -------------------- COOLANT TEMPERATURE FUNCTIONS --------------------

// GETS TEMP VALUE FROM THE SENSOR AND UPDATES TEMP IN MEMORY, IF FAIL RISES FLAG FOR SENSOL FAILURE BOOLEAN
void coolant_temp_now_update() {
  if (sensor_coolant_temp.ready()) {
    if (sensor_coolant_temp.readTemp()) {
      if (temp_sensor_failure == true) {
        temp_sensor_failure = false;
      }
      coolant_temp_now = sensor_coolant_temp.getTempInt();
    } else {
      temp_sensor_failure = true;
    }
    sensor_coolant_temp.requestTemp();
  }
}

// UPDATES IN MEMORY COOLANT TEMP REQUIRED RANGE
int coolant_temp_return_required_range() {
  return (((coolant_temp_max - coolant_temp_min) / 5) + 1) ;
}

// UPDATES IN MEMORY COOLANT TEMP REQUIRED VALUE
void coolant_temp_required_update() {
  coolant_temp_required = coolant_temp_min + potentiometer_return_current_value() * 5;
  coolant_temp_required = constrain(coolant_temp_required, coolant_temp_min, coolant_temp_max);
}

// -------------------- POTENTIOMETER FUNCTIONS --------------------

// RETURNS STEP FOR POTENTIOMETER 
int potentiometer_return_step_for_temp() {
  return 1023 / coolant_temp_return_required_range();
}

// RETURNS VALUE ON POTENTIOMETER TAKING INTO ACCOUNT PRESELECTED STEP
int potentiometer_return_current_value() {
  return analogRead(POTENT_PIN) / potentiometer_return_step_for_temp();
}

// -------------------- VALVE CONTROL SCRIPTS -------------------- 

// SENDS SIGNAL TO START VALVE OPENING
void valve_opening_start_send_signal() {
  digitalWrite(VALVE_OPEN_PIN, open_relay);
}

// SENDS SIGNAL TO STOP VALVE OPENING
void valve_opening_finish_send_signal() {
  digitalWrite(VALVE_OPEN_PIN, close_relay);
}

// SENDS SIGNAL TO START VALVE CLOSING
void valve_closing_start_send_signal() {
  digitalWrite(VALVE_CLOSE_PIN, open_relay);
}

// SENDS SIGNAL TO STOP VALVE CLOSING
void valve_closing_finish_send_signal() {
  digitalWrite(VALVE_CLOSE_PIN, close_relay);
}

// SET OF ACTIONS FOR VAVLE OPENING START
void valve_opening_start() {
  Serial.println("Valve_opening_start");
  valve_is_opening = true;
  valve_started_action_last_time = current_time;
  valve_opening_start_send_signal();
}

// SET OF ACTIONS FOR VAVLE OPENING FINISH
void valve_opening_finish() {
  Serial.println("Valve_opening_finish");
  valve_is_opening = false;
  valve_finished_action_last_time = current_time;
  valve_opening_finish_send_signal();
}

// SET OF ACTIONS FOR VAVLE CLOSING START
void valve_closing_start() {
  Serial.println("Valve_closing_start");
  valve_is_closing = true;
  valve_started_action_last_time = current_time;
  valve_closing_start_send_signal();
}

// SET OF ACTIONS FOR VAVLE CLOSING FINISH
void valve_closing_finish() {
  Serial.println("Valve_closing_finish");
  valve_is_closing = false;
  valve_finished_action_last_time = current_time;
  valve_closing_finish_send_signal();
}

// RETURN OPERATION TIME, MS 
unsigned int valve_action_time() {
  return current_time - valve_started_action_last_time;
}

// RETURNS TRUE IF VALVE IN OPERATION MORE THAN PLANNED
bool valve_is_operation_time_passed() {
  if (valve_action_time() >= valve_action_time_1000) {
    return true;
  } else {
    return false;
  }
}

// RETURNS DELAY TIME, MS
unsigned int vavle_delay_time() {
  return current_time - valve_finished_action_last_time;
}

// RETURNS TRUE IF VALVE IN DELAY MORE THAN PLANNED
bool valve_is_delay_time_passed() {
  if (vavle_delay_time() >= valve_delay_time_1000) {
    return true;
  } else {
    return false;
  }
}

// VALVE STOPS ALL KING OF OPERATION
void valve_stop_operation() {
  if (valve_is_opening == true) {
    valve_opening_finish();
  } 
  if (valve_is_closing == true) {
    valve_closing_finish();
  }
}

// -------------------- TEMP ANALYSIS SCRIPTS --------------------

// UPDATES IN MEMORY DIFFERENCE BETWEEN REQUIRED TEMP AND CURRENT TEMP
void coolant_get_temp_difference() {
  coolant_temp_difference = coolant_temp_required - coolant_temp_now;
}

// COMPARE CURRENT TEMP DIFF WITH DIFF MAX, MAKE DECISION IS TEMP CORRECTION NECESSITY, RETURN TRUE OF FALSE
bool coolant_is_temp_diff_more_than_max() {
  if (abs(coolant_temp_difference) > coolant_temp_difference_max) {
    return true;
  } else {
    return false;
  }
}

//  RETURN TRUE IF OPENING REQUIRED OR FALSE IF CLOSING IS REQUIRED
bool valve_is_opening_required () {
  if (coolant_temp_difference >= 0) {
    return true;
  } else {
    return false;
  } 
}

// -------------------- VALVE OPERATION SCRIPTS --------------------

// GENERAL DECISION FOR VALVE OPERATION
void valve_general_control() {
  if (coolant_is_temp_diff_more_than_max() == true) {
    valve_start_operation();
  } else {
    valve_stop_operation();
  }
}

// DESICION IF OPENING OR CLOSING SCRYPT WILL BE IMPLEMENTED
void valve_start_operation() {
  if (valve_is_opening_required() == true) {
    valve_operation_opening();
  } else {
    valve_operation_closing();
  }
}

// DECISION IF START OR FINISH FOR OPENING WILL BE IMPLEMENTED
void valve_operation_opening() {
  if (valve_is_opening == false) {
    if (valve_is_delay_time_passed() == true) {
      valve_opening_start();
    }
  } else {
    if (valve_is_operation_time_passed() == true) {
      valve_opening_finish();
    }
  }
}

// DECISION IF START OR FINISH FOR CLOSING WILL BE IMPLEMENTED
void valve_operation_closing() {
  if (valve_is_closing == false) {
    if (valve_is_delay_time_passed() == true) {
      valve_closing_start();
    }
  } else {
    if (valve_is_operation_time_passed() == true) {
      valve_closing_finish();
    }
  }
}

// -------------------- DISPLAY FUNCTIONS --------------------

// DYSPLAYS TEMP IN POS 0, 1
void display_temperature(GyverTM1637 display, int temp) {
  if (temp >= 10) {
    display.display(0, temp / 10);
    display.display(1, temp % 10);
  }
}

// DISPLAYS _DEG _C IN POS 2, 3
void display_celc_deg_(GyverTM1637 display) {
  display.displayByte(2, _degree);
  display.displayByte(3, _C);
}


// --- TEST FUNCTIOINS ---
void test_relay_open() {
  Serial.println("Opened");
  digitalWrite(VALVE_OPEN_PIN, open_relay);
  digitalWrite(VALVE_CLOSE_PIN, open_relay);  
}

void test_relay_close() {
  Serial.println("Closed");
  digitalWrite(VALVE_OPEN_PIN, close_relay);
  digitalWrite(VALVE_CLOSE_PIN, close_relay);
}

void test_relay() {
  test_relay_open();
  delay(1000);
  test_relay_close();
  delay(1000);
}


void setup() {
  // START SERIAL.
  Serial.begin(9600);

  // TEMP SENSOR SETUP
  pinMode(SENSOR_PIN, INPUT);
  sensor_coolant_temp.requestTemp();

  // RELAY SETUP
  pinMode(VALVE_OPEN_PIN, OUTPUT);
  pinMode(VALVE_CLOSE_PIN, OUTPUT);
  valve_stop_operation();

  // DISPLAY SETUP
  display_temp_now.clear();
  display_temp_now.brightness(7);
  display_celc_deg_(display_temp_now);
  display_temp_req.clear();
  display_temp_req.brightness(7);
  display_celc_deg_(display_temp_req);
}

void loop() {
  current_time = millis();

  // --- UPDATE TEMP VALUES ---
  coolant_temp_now_update();
  coolant_get_temp_difference();
  coolant_temp_required_update();

  // --- VALVE CONTROL ---
  valve_general_control();

  // --- DISPLAY INFO ON DISPLAYS ---
  display_temperature(display_temp_now, coolant_temp_now);
  display_temperature(display_temp_req, coolant_temp_required);

}
