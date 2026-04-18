#define SENSOR_PIN 2
#define RELAY_1 8   // motor
#define RELAY_2 9   // door

// durasi (ms)
unsigned long motorTime   = 300; // relay 1 ON
unsigned long delayTime   = 1000; // jeda antar relay
unsigned long doorTime    = 3000; // relay 2 ON
unsigned long cooldownTime = 2000;

// timer
unsigned long previousMillis = 0;

// state
int stepState = 0;
bool cooldown = false;

void setup() {
  pinMode(SENSOR_PIN, INPUT_PULLUP);

  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);

  // relay OFF (active LOW)
  digitalWrite(RELAY_1, HIGH);
  digitalWrite(RELAY_2, HIGH);
}

void loop() {
  unsigned long currentMillis = millis();
  int sensorState = digitalRead(SENSOR_PIN);

  switch (stepState) {

    // 🔹 0 = standby
    case 0:
      if (sensorState == LOW && !cooldown) {
        digitalWrite(RELAY_1, LOW); // relay 1 ON
        previousMillis = currentMillis;
        stepState = 1;
      }
      break;

    // 🔹 1 = relay 1 ON
    case 1:
      if (currentMillis - previousMillis >= motorTime) {
        digitalWrite(RELAY_1, HIGH); // relay 1 OFF
        previousMillis = currentMillis;
        stepState = 2;
      }
      break;

    // 🔹 2 = jeda 3 detik
    case 2:
      if (currentMillis - previousMillis >= delayTime) {
        digitalWrite(RELAY_2, LOW); // relay 2 ON
        previousMillis = currentMillis;
        stepState = 3;
      }
      break;

    // 🔹 3 = relay 2 ON
    case 3:
      if (currentMillis - previousMillis >= doorTime) {
        digitalWrite(RELAY_2, HIGH); // relay 2 OFF
        previousMillis = currentMillis;
        cooldown = true;
        stepState = 4;
      }
      break;

    // 🔹 4 = cooldown
    case 4:
      if (currentMillis - previousMillis >= cooldownTime) {
        cooldown = false;
        stepState = 0;
      }
      break;
  }
}
