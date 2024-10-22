#include <GyverDS18.h>
#define SENSOR_PIN 3
#define VALVE_OPEN_PIN 14
#define VALVE_CLOSE_PIN 15
GyverDS18Single sensor_coolant_temp(SENSOR_PIN);

int coolant_temp_now = 0;
int coolant_temp_difference = 0;
int coolant_temp_difference_max = 0;
int coolant_temp_required = 0;
bool temp_sensor_failure = false;

bool valve_is_opening = false;
bool valve_is_closing = false;
unsigned long valve_acted_last_time = 0;
unsigned int valve_action_time_1000 = 1000;

unsigned int current_time = 0;

bool test_mode = false;

// ---------- TEMPERATURE MEASUREMENT SCRIPTS ----------

// UPDATE TEMP VALUE FROM THE SENSOR, IF FAIL RISE FLAG FOR SENSOL FAILURE BOOLEAN
void coolant_get_temp() {
  if (sensor_coolant_temp.ready()) {
    if (sensor_coolant_temp.readTemp()) {
      if (temp_sensor_failure == true) {
        temp_sensor_failure = false;
      }
      coolant_temp_now = sensor_coolant_temp.getTempInt();
    }
  } else {
    temp_sensor_failure = true;
  }
}

// ---------- VALVE CONTROL SCRIPTS ---------- 

// SEND SIGNAL TO START VALVE OPENING
void valve_opening_start_send_signal() {
  digitalWrite(VALVE_OPEN_PIN, 1);
}

// SEND SIGNAL TO STOP VALVE OPENING
void valve_opening_finish_send_signal() {
  digitalWrite(VALVE_OPEN_PIN, 0);
}

// SEND SIGNAL TO START VALVE CLOSING
void valve_closing_start_send_signal() {
  digitalWrite(VALVE_CLOSE_PIN, 1);
}

// SEND SIGNAL TO STOP VALVE CLOSING
void valve_closing_finish_send_signal() {
  digitalWrite(VALVE_CLOSE_PIN, 1);
}

// SET OF ACTIONS FOR VAVLE OPENING START
void valve_opening_start() {
  valve_is_opening = true;
  valve_acted_last_time = current_time;
  valve_opening_start_send_signal();
  if (test_mode == true) {
    Serial.print("Valve_opening started at: ");
    Serial.println(current_time);
  }
}

// SET OF ACTIONS FOR VAVLE OPENING FINISH
void valve_opening_finish() {
  valve_is_opening = false;
  valve_opening_finish_send_signal();
  if (test_mode == true) {
    Serial.print("Valve_opening finished at: ");
    Serial.println(current_time);
  }
}

// SET OF ACTIONS FOR VAVLE CLOSING START
void valve_closing_start() {
  valve_is_closing = true;
  valve_acted_last_time = current_time;
  valve_closing_start_send_signal();
  if (test_mode == true) {
    Serial.print("Valve_closing started at: ");
    Serial.println(current_time);
  }
}

// SET OF ACTIONS FOR VAVLE CLOSING FINISH
void valve_closing_finish() {
  valve_is_closing = false;
  valve_closing_finish_send_signal();
  if (test_mode == true) {
    Serial.print("Valve_closing finished at: ");
    Serial.println(current_time);
  }
}

// RETURN OPERATION TIME, MS 
int valve_action_time() {
  return current_time - valve_acted_last_time;
}

// RETURN TRUE IF VALVE IN OPERATION MORE THAN PLANNED
bool valve_is_operation_time_passed() {
  if (valve_action_time() >= valve_action_time_1000) {
    return true;
  } else {
    return false;
  }
}

// VALVE STOP ALL KING OF OPERATION
void valve_stop_operation() {
  digitalWrite(VALVE_OPEN_PIN, 0);
  digitalWrite(VALVE_CLOSE_PIN, 0);
}

// ---------- TEMP ANALYSIS SCRIPTS ----------

// UPDATE IN MEMORY DIFFERENCE BETWEEN REQUIRED TEMP AND CURRENT TEMP
void coolant_get_temp_difference() {
  coolant_temp_difference = coolant_temp_now - coolant_temp_required;
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

// ---------- VALVE OPERATION SCRIPTS ----------

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
    valve_opening_start();
  } else {
    if (valve_is_operation_time_passed() == true) {
      valve_opening_finish();
    }
  }
}

// DECISION IF START OR FINISH CLOSING WILL BE IMPLEMENTED
void valve_operation_closing() {
  if (valve_is_closing == false) {
    valve_closing_start();
  } else {
    if (valve_is_operation_time_passed() == true) {
      valve_closing_finish();
    }
  }
}



void setup() {
  Serial.begin(9600);

  pinMode(SENSOR_PIN, INPUT);
  pinMode(VALVE_OPEN_PIN, OUTPUT);
  pinMode(VALVE_CLOSE_PIN, OUTPUT);

  sensor_coolant_temp.requestTemp();
}

void loop() {
  // put your main code here, to run repeatedly:
  coolant_get_temp();
  valve_general_control();
}
