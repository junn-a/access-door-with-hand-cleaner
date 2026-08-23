#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- KONFIGURASI OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- PIN DEFINITION ---
#define SENSOR_PIN        4   // Sensor alkohol / proximity
#define LIMIT_SWITCH_PIN 23   // Limit switch pintu
#define RELAY_1          27   // Motor Alkohol
#define RELAY_2          14   // Solenoid / Door
#define LED_GREEN        18   // LED Standby
#define LED_ORANGE       19   // LED Ada orang / Pintu Terbuka

// --- SENSOR AJ-SR04M (SERIAL2) ---
#define RX2_PIN 16 // Connect to TX AJ-SR04M
#define TX2_PIN 17 // Connect to RX AJ-SR04M
HardwareSerial SensorSerial(2);

// --- TANGKI CONFIG ---
const float TANK_HEIGHT_CM = 50.0;  // Tinggi total tangki dari sensor ke dasar (cm)
const float MIN_DISTANCE_CM = 5.0;  // Jarak sensor ke cairan saat 100% penuh (cm)
int tankLevelPercent = 0;           // Persentase isi tangki (0 - 100%)
float distanceCm = 0;               // Jarak mentah dari sensor

// Timer untuk pembacaan sensor non-blocking
unsigned long lastSensorReadMillis = 0;
const unsigned long sensorReadInterval = 1000; // Baca sensor tiap 1 detik

// --- VARIABEL COUNTER & STATE EXISTING ---
int motorTriggerCount = 0;   // Jumlah sensor terhalang (Motor Aktif)
int doorClosedCount = 0;     // Jumlah limit switch tertekan

int lastLimitState = HIGH;   // State awal limit switch
int lastSensorState = HIGH;  // State awal sensor

// Variabel Debounce Limit Switch
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200; // 200ms

// Durasi (ms)
const unsigned long motorTime = 300;   // Waktu Relay 1 ON
const unsigned long doorTime  = 3000;  // Waktu Relay 2 ON

// Timer independen
unsigned long motorStartMillis = 0;
unsigned long doorStartMillis  = 0;

// State status
bool motorActive = false;
bool doorActive  = false;

// --- FUNGSI BACA SENSOR ULTRASONIK AJ-SR04M ---
void readAlcoholLevel() {
  // Kirim trigger byte (0xA0) ke AJ-SR04M Mode Serial
  SensorSerial.write(0xA0); 
  delay(50);

  if (SensorSerial.available() >= 3) {
    byte header = SensorSerial.read();
    byte highByte = SensorSerial.read();
    byte lowByte = SensorSerial.read();

    if (header == 0xFF) {
      int distanceMM = (highByte << 8) + lowByte;
      distanceCm = distanceMM / 10.0;

      // Hitung persentase kapasitas tangki
      if (distanceCm >= TANK_HEIGHT_CM) {
        tankLevelPercent = 0;
      } else if (distanceCm <= MIN_DISTANCE_CM) {
        tankLevelPercent = 100;
      } else {
        tankLevelPercent = map(distanceCm, TANK_HEIGHT_CM, MIN_DISTANCE_CM, 0, 100);
        tankLevelPercent = constrain(tankLevelPercent, 0, 100);
      }
    }
  }
}

// --- FUNGSI UPDATE OLED (VERSI SIMPEL & INFORMATIF) ---
void updateOLED() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // 1. TANGKI ALKOHOL (Atas: Progress Bar Besar + Status Peringatan)
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("TANGKI: ");
  display.print(tankLevelPercent);
  display.print("%");

  // Peringatan otomatis jika isi tangki kritis (<= 20%)
  if (tankLevelPercent <= 20) {
    display.setCursor(68, 0);
    display.print("!ISI ULANG");
  }

  // Progress Bar Ukuran Besar (Kapasitas Tangki)
  display.drawRect(0, 10, 128, 14, SSD1306_WHITE); // Garis Luar
  int barWidth = map(tankLevelPercent, 0, 100, 0, 124);
  if (barWidth > 0) {
    display.fillRect(2, 12, barWidth, 10, SSD1306_WHITE); // Isi Bar
  }

  // Garis Pemisah Tengah
  display.drawLine(0, 28, 128, 28, SSD1306_WHITE);

  // 2. COUNTER MOTOR vs LIMIT SWITCH (Bawah: Angka Besar)
  display.setTextSize(1);
  display.setCursor(0, 33);
  display.print("MOTOR");
  display.setCursor(72, 33);
  display.print("LIMIT SW");

  // Display Angka Counter (Size 2)
  display.setTextSize(2);
  display.setCursor(0, 46);
  display.print(motorTriggerCount);

  display.setCursor(52, 46);
  display.print("VS");

  display.setCursor(88, 46);
  display.print(doorClosedCount);

  display.display(); // Refresh layar OLED
}

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi Serial Hardware 2 untuk AJ-SR04M
  SensorSerial.begin(9600, SERIAL_8N1, RX2_PIN, TX2_PIN);

  // Inisialisasi OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED SSD1306 gagal diinisialisasi!"));
    for (;;);
  }

  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(LIMIT_SWITCH_PIN, INPUT_PULLUP);

  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_ORANGE, OUTPUT);

  // Relay Active LOW (HIGH = OFF)
  digitalWrite(RELAY_1, HIGH);
  digitalWrite(RELAY_2, HIGH);

  // Status Awal Indikator LED
  digitalWrite(LED_GREEN, HIGH);  // Hijau Nyala (Standby)
  digitalWrite(LED_ORANGE, LOW);   // Orange Mati

  // Pembacaan awal level tangki & Render OLED
  readAlcoholLevel();
  updateOLED();
}

void loop() {
  unsigned long currentMillis = millis();
  int currentSensorState = digitalRead(SENSOR_PIN);
  bool stateChanged = false; // Flag untuk render ulang OLED

  // 0. Periodic Level Check (Setiap 1 detik)
  if (currentMillis - lastSensorReadMillis >= sensorReadInterval) {
    lastSensorReadMillis = currentMillis;
    readAlcoholLevel();
    stateChanged = true; // Update tampilan jika ada siklus interval pembacaan
  }

  // 1. Deteksi Sensor Terhalang (Trigger HIGH ke LOW)
  if (currentSensorState == LOW && lastSensorState == HIGH) {
    motorTriggerCount++; // Tambah hitungan motor aktif

    digitalWrite(RELAY_1, LOW);
    motorStartMillis = currentMillis;
    motorActive = true;

    if (!doorActive) {
      digitalWrite(RELAY_2, LOW);
      doorStartMillis = currentMillis;
      doorActive = true;

      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_ORANGE, HIGH);
    }
    stateChanged = true;
  }
  lastSensorState = currentSensorState;

  // 2. Timer Relay 1 (Motor)
  if (motorActive && (currentMillis - motorStartMillis >= motorTime)) {
    digitalWrite(RELAY_1, HIGH); // Motor OFF
    motorActive = false;
    stateChanged = true;
  }

  // 3. Timer Relay 2 (Pintu & LED)
  if (doorActive && (currentMillis - doorStartMillis >= doorTime)) {
    digitalWrite(RELAY_2, HIGH); // Pintu OFF
    doorActive = false;

    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_ORANGE, LOW);
    stateChanged = true;
  }

  // 4. Logika Counting Limit Switch + Debounce
  int currentLimitState = digitalRead(LIMIT_SWITCH_PIN);

  if (currentLimitState == LOW && lastLimitState == HIGH) {
    if (currentMillis - lastDebounceTime > debounceDelay) {
      doorClosedCount++; // Tambah hitungan limit switch
      lastDebounceTime = currentMillis;
      stateChanged = true;
    }
  }
  lastLimitState = currentLimitState;

  // 5. Render Ulang OLED Hanya Saat Ada Perubahan State/Count/Interval
  if (stateChanged) {
    updateOLED();
  }
}
