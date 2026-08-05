#define SENSOR_1_PIN 2  // Sensor Otomatis (Relay 1 -> Jeda -> Relay 2)
#define SENSOR_2_PIN 3  // Sensor Khusus Relay 2
#define RELAY_1 8       // Motor
#define RELAY_2 9       // Door

// Durasi (ms)
unsigned long motorTime    = 300;  // Relay 1 ON
unsigned long delayTime    = 1000; // Jeda antar relay
unsigned long doorTime     = 3000; // Relay 2 ON
unsigned long cooldownTime = 2000;

// Timer Alur Utama (Sensor 1)
unsigned long previousMillis = 0;

// Timer Mandiri Relay 2 (Sensor 2 / Otomatis)
unsigned long relay2StartMillis = 0;
bool relay2Active = false;

// State Alur Utama
int stepState = 0;
bool cooldown = false;

void setup() {
  pinMode(SENSOR_1_PIN, INPUT_PULLUP);
  pinMode(SENSOR_2_PIN, INPUT_PULLUP);

  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);

  // Relay OFF (Active LOW)
  digitalWrite(RELAY_1, HIGH);
  digitalWrite(RELAY_2, HIGH);
}

void loop() {
  unsigned long currentMillis = millis();

  int sensor1State = digitalRead(SENSOR_1_PIN);
  int sensor2State = digitalRead(SENSOR_2_PIN);

  // ==========================================
  // 1. KONTROL KHUSUS SENSOR 2 (RELAY 2 DIRECT)
  // ==========================================
  // Jika Sensor 2 terdeteksi aktif, langsung nyalakan Relay 2
  if (sensor2State == LOW && !relay2Active) {
    relay2Active = true;
    relay2StartMillis = currentMillis;
    digitalWrite(RELAY_2, LOW); // Relay 2 ON
  }

  // Timer pemadam Relay 2 (berlaku baik dari Sensor 1 maupun Sensor 2)
  if (relay2Active) {
    if (currentMillis - relay2StartMillis >= doorTime) {
      digitalWrite(RELAY_2, HIGH); // Relay 2 OFF
      relay2Active = false;
    }
  }

  // ==========================================
  // 2. ALUR UTAMA / SENSOR 1 (MOTOR -> DOOR)
  // ==========================================
  switch (stepState) {

    // 🔹 0 = Standby
    case 0:
      if (sensor1State == LOW && !cooldown) {
        digitalWrite(RELAY_1, LOW); // Relay 1 ON
        previousMillis = currentMillis;
        stepState = 1;
      }
      break;

    // 🔹 1 = Relay 1 ON
    case 1:
      if (currentMillis - previousMillis >= motorTime) {
        digitalWrite(RELAY_1, HIGH); // Relay 1 OFF
        previousMillis = currentMillis;
        stepState = 2;
      }
      break;

    // 🔹 2 = Jeda sebelum Relay 2 ON
    case 2:
      if (currentMillis - previousMillis >= delayTime) {
        // Picu Relay 2 menggunakan logika mandiri agar tidak terjadi konflik
        digitalWrite(RELAY_2, LOW); // Relay 2 ON
        relay2Active = true;
        relay2StartMillis = currentMillis;

        previousMillis = currentMillis;
        stepState = 3;
      }
      break;

    // 🔹 3 = Menunggu Relay 2 Selesai Durasi Door Time
    case 3:
      if (!relay2Active) { // Menunggu timer Relay 2 selesai di bagian atas
        previousMillis = currentMillis;
        cooldown = true;
        stepState = 4;
      }
      break;

    // 🔹 4 = Cooldown
    case 4:
      if (currentMillis - previousMillis >= cooldownTime) {
        cooldown = false;
        stepState = 0;
      }
      break;
  }
}
