#include <Servo.h>

#define SENSOR_1_PIN 2  // Sensor Otomatis (Relay 1 -> Jeda -> Relay 2)
#define SENSOR_2_PIN 3  // Sensor Khusus Relay 2
#define RELAY_1 8       // Motor
#define RELAY_2 9       // Door
#define SERVO_PIN 10    // Pin Sinyal Servo TD-8120MG (Orange)

// Posisi Derajat Servo
#define SERVO_CLOSE_POS 0   // Posisi saat Relay 2 OFF (Standby)
#define SERVO_OPEN_POS  90  // Posisi saat Relay 2 ON

Servo doorServo;

// Durasi (ms)
unsigned long motorTime    = 300;  // Relay 1 ON
unsigned long delayTime    = 1000; // Jeda antar relay
unsigned long doorTime     = 3000; // Relay 2 ON
unsigned long cooldownTime = 2000;

// Timer Alur Utama (Sensor 1)
unsigned long previousMillis = 0;

// Timer Mandiri Relay 2 & Servo
unsigned long relay2StartMillis = 0;
bool relay2Active = false;

// State Alur Utama
int stepState = 0;
bool cooldown = false;

// Fungsi helper untuk mengontrol Relay 2 dan Servo bersamaan
void setDoorState(bool active) {
  if (active) {
    digitalWrite(RELAY_2, LOW);     // Relay 2 ON (Active LOW)
    doorServo.write(SERVO_OPEN_POS); // Servo bergerak ke posisi buka
  } else {
    digitalWrite(RELAY_2, HIGH);    // Relay 2 OFF
    doorServo.write(SERVO_CLOSE_POS); // Servo kembali ke posisi awal
  }
}

void setup() {
  pinMode(SENSOR_1_PIN, INPUT_PULLUP);
  pinMode(SENSOR_2_PIN, INPUT_PULLUP);

  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);

  // Relay OFF (Active LOW)
  digitalWrite(RELAY_1, HIGH);
  digitalWrite(RELAY_2, HIGH);

  // Inisialisasi Servo
  doorServo.attach(SERVO_PIN);
  doorServo.write(SERVO_CLOSE_POS); // Set posisi awal servo saat standby
}

void loop() {
  unsigned long currentMillis = millis();

  int sensor1State = digitalRead(SENSOR_1_PIN);
  int sensor2State = digitalRead(SENSOR_2_PIN);

  // ==========================================
  // 1. KONTROL KHUSUS SENSOR 2 (RELAY 2 & SERVO DIRECT)
  // ==========================================
  if (sensor2State == LOW && !relay2Active) {
    relay2Active = true;
    relay2StartMillis = currentMillis;
    setDoorState(true); // Nyalakan Relay 2 & Gerakkan Servo
  }

  // Timer pemadam Relay 2 & Servo
  if (relay2Active) {
    if (currentMillis - relay2StartMillis >= doorTime) {
      setDoorState(false); // Matikan Relay 2 & Kembalikan Servo
      relay2Active = false;
    }
  }

  // ==========================================
  // 2. ALUR UTAMA / SENSOR 1 (MOTOR -> DOOR + SERVO)
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

    // 🔹 2 = Jeda sebelum Relay 2 & Servo Aktif
    case 2:
      if (currentMillis - previousMillis >= delayTime) {
        setDoorState(true); // Nyalakan Relay 2 & Gerakkan Servo
        relay2Active = true;
        relay2StartMillis = currentMillis;

        previousMillis = currentMillis;
        stepState = 3;
      }
      break;

    // 🔹 3 = Menunggu Relay 2 & Servo Selesai Durasi
    case 3:
      if (!relay2Active) { // Menunggu timer selesai di bagian kontrol atas
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
