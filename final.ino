/*
  ESP32 - Access Door Control + Alcohol Tank Level (v3.2)
  ----------------------------------------------------
  1. Sensor Proximity 1 -> Relay 1 (pulse 500ms) + Relay 2 (3 detik) -> Buzzer alarm jika pintu terlambat ditutup.
  2. Sensor Proximity 2 -> Relay 2 (3 detik) -> TIDAK memicu buzzer.
  3. Limit switch mematikan alarm pintu.
  4. SENSOR ALKOHOL (XKC-Y265S PNP):
     - Memantau level cairan alkohol.
     - Jika alkohol HABIS -> Buzzer berbunyi beep panjang berkala (800ms ON / 400ms OFF).
     - Bunyi alarm alkohol dapat berjalan bersamaan/memiliki pola beda dari alarm pintu.
*/

// ==================== PIN CONFIGURATION ====================
const int PIN_PROX1        = 4;   // Sensor proximity 1 (trigger Relay 1 + Relay 2)
const int PIN_PROX2        = 17;  // Sensor proximity 2 (trigger Relay 2 langsung)
const int PIN_LIMIT_SWITCH = 5;   // Limit switch
const int PIN_ALCOHOL_SENS = 16;  // Sensor XKC-Y265S (Gunakan Voltage Divider ke 3.3V!)
const int PIN_RELAY1       = 18;  // Relay 1 -> motor 5V
const int PIN_RELAY2       = 19;  // Relay 2 -> magnet pintu (door lock)
const int PIN_BUZZER       = 21;  // Buzzer

// ==================== LOGIC LEVELS ====================
const int RELAY_ACTIVE   = LOW;   // Active LOW
const int RELAY_INACTIVE = HIGH;

const int PROX_ACTIVE_LEVEL   = LOW;  // NPN Open-Collector / Active LOW
const int LIMIT_PRESSED_LEVEL = LOW;  // Ditekan = LOW (Internal Pull-up)

// XKC-Y265S Mode GND: Saat alkohol HABIS -> Output HIGH
const int ALCOHOL_EMPTY_LEVEL = HIGH; 

const int BUZZER_ON  = HIGH;
const int BUZZER_OFF = LOW;

// ==================== TIMING CONFIG ====================
const unsigned long RELAY1_PULSE_MS    = 500;   // Durasi Relay1 (motor)
const unsigned long RELAY2_DURATION_MS = 3000;  // Durasi Relay2 (magnet pintu)
const unsigned long DEBOUNCE_MS        = 50;    // Debounce sensor & limit switch

// Timing Buzzer Pintu (Kedip Cepat)
const unsigned long BUZZER_DOOR_BLINK_MS = 200; 

// Timing Buzzer Alkohol (Beep Panjang: 800ms ON, 400ms OFF)
const unsigned long BUZZER_ALC_ON_MS  = 800;
const unsigned long BUZZER_ALC_OFF_MS = 400;

// ==================== STATE VARIABLES ====================
// Sensor Prox 1
int prox1StableState = HIGH;
int prox1LastReading = HIGH;
unsigned long prox1LastChangeTime = 0;

// Sensor Prox 2
int prox2StableState = HIGH;
int prox2LastReading = HIGH;
unsigned long prox2LastChangeTime = 0;

// Limit Switch
int limitStableState = HIGH;
int limitLastReading = HIGH;
unsigned long limitLastChangeTime = 0;
bool doorClosed = false;

// Sensor Alkohol
int alcStableState = LOW;
int alcLastReading = LOW;
unsigned long alcLastChangeTime = 0;
bool alcoholEmpty = false;

// Relays & Alarm
bool relay1Active = false;
unsigned long relay1StartTime = 0;

bool relay2Active = false;
unsigned long relay2StartTime = 0;
bool relay2ExpiryChecked = false;
bool triggeredByProx1 = false; 

// Status Alarm
bool buzzerDoorAlarmActive = false;
unsigned long buzzerAlarmStartTime = 0;
unsigned long buzzerLastToggleTime = 0;
bool buzzerPinState = false;

unsigned int buzzerAlarmCount = 0;
unsigned long buzzerLastAlarmDuration = 0;
unsigned long buzzerTotalAlarmDuration = 0;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_PROX1, INPUT_PULLUP);
  pinMode(PIN_PROX2, INPUT_PULLUP);
  pinMode(PIN_LIMIT_SWITCH, INPUT_PULLUP);
  pinMode(PIN_ALCOHOL_SENS, INPUT_PULLDOWN); // Pull-down untuk kestabilan sinyal PNP

  pinMode(PIN_RELAY1, OUTPUT);
  pinMode(PIN_RELAY2, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_RELAY1, RELAY_INACTIVE);
  digitalWrite(PIN_RELAY2, RELAY_INACTIVE);
  digitalWrite(PIN_BUZZER, BUZZER_OFF);

  prox1StableState = digitalRead(PIN_PROX1);
  prox1LastReading = prox1StableState;

  prox2StableState = digitalRead(PIN_PROX2);
  prox2LastReading = prox2StableState;

  limitStableState = digitalRead(PIN_LIMIT_SWITCH);
  limitLastReading = limitStableState;
  doorClosed = (limitStableState == LIMIT_PRESSED_LEVEL);

  alcStableState = digitalRead(PIN_ALCOHOL_SENS);
  alcLastReading = alcStableState;
  alcoholEmpty = (alcStableState == ALCOHOL_EMPTY_LEVEL);

  Serial.println("System v3.2 (with Alcohol Sensor) Ready.");
}

void loop() {
  unsigned long now = millis();

  handleProx1(now);
  handleProx2(now);
  handleLimitSwitch(now);
  handleAlcoholSensor(now);
  handleRelay1(now);
  handleRelay2(now);
  handleBuzzer(now);
}

// ---------------------------------------------------------
// Handler Sensor Proximity 1
// ---------------------------------------------------------
void handleProx1(unsigned long now) {
  int reading = digitalRead(PIN_PROX1);

  if (reading != prox1LastReading) {
    prox1LastChangeTime = now;
    prox1LastReading = reading;
  }

  if ((now - prox1LastChangeTime) > DEBOUNCE_MS) {
    if (reading != prox1StableState) {
      int prevStable = prox1StableState;
      prox1StableState = reading;

      if (prevStable != PROX_ACTIVE_LEVEL && prox1StableState == PROX_ACTIVE_LEVEL) {
        onProx1Triggered(now);
      }
    }
  }
}

void onProx1Triggered(unsigned long now) {
  Serial.println("[PROX1] Sensor 1 kena -> trigger Relay1 + Relay2 (Alarm Enabled)");

  if (buzzerDoorAlarmActive) {
    buzzerDoorAlarmActive = false;
    Serial.println("[ALARM PINTU] Alarm di-reset oleh pemicu ulang Prox 1.");
  }

  triggeredByProx1 = true;

  relay1Active = true;
  relay1StartTime = now;
  digitalWrite(PIN_RELAY1, RELAY_ACTIVE);

  relay2Active = true;
  relay2StartTime = now;
  relay2ExpiryChecked = false;
  digitalWrite(PIN_RELAY2, RELAY_ACTIVE);
}

// ---------------------------------------------------------
// Handler Sensor Proximity 2
// ---------------------------------------------------------
void handleProx2(unsigned long now) {
  int reading = digitalRead(PIN_PROX2);

  if (reading != prox2LastReading) {
    prox2LastChangeTime = now;
    prox2LastReading = reading;
  }

  if ((now - prox2LastChangeTime) > DEBOUNCE_MS) {
    if (reading != prox2StableState) {
      int prevStable = prox2StableState;
      prox2StableState = reading;

      if (prevStable != PROX_ACTIVE_LEVEL && prox2StableState == PROX_ACTIVE_LEVEL) {
        onProx2Triggered(now);
      }
    }
  }
}

void onProx2Triggered(unsigned long now) {
  Serial.println("[PROX2] Sensor 2 kena -> trigger Relay2 LANGSUNG (No Alarm)");

  if (buzzerDoorAlarmActive) {
    buzzerDoorAlarmActive = false;
    Serial.println("[ALARM PINTU] Alarm di-reset oleh pemicu ulang Prox 2.");
  }

  triggeredByProx1 = false;

  relay2Active = true;
  relay2StartTime = now;
  relay2ExpiryChecked = false;
  digitalWrite(PIN_RELAY2, RELAY_ACTIVE);
}

// ---------------------------------------------------------
// Handler Limit Switch
// ---------------------------------------------------------
void handleLimitSwitch(unsigned long now) {
  int reading = digitalRead(PIN_LIMIT_SWITCH);

  if (reading != limitLastReading) {
    limitLastChangeTime = now;
    limitLastReading = reading;
  }

  if ((now - limitLastChangeTime) > DEBOUNCE_MS) {
    if (reading != limitStableState) {
      limitStableState = reading;
    }
  }

  doorClosed = (limitStableState == LIMIT_PRESSED_LEVEL);
}

void onDoorClosed(unsigned long now) {
  buzzerLastAlarmDuration = now - buzzerAlarmStartTime;
  buzzerTotalAlarmDuration += buzzerLastAlarmDuration;
  buzzerDoorAlarmActive = false;

  Serial.println("[LIMIT] Pintu tertutup -> Alarm Pintu dimatikan.");
}

// ---------------------------------------------------------
// Handler Sensor Alkohol (XKC-Y265S)
// ---------------------------------------------------------
void handleAlcoholSensor(unsigned long now) {
  int reading = digitalRead(PIN_ALCOHOL_SENS);

  if (reading != alcLastReading) {
    alcLastChangeTime = now;
    alcLastReading = reading;
  }

  if ((now - alcLastChangeTime) > DEBOUNCE_MS) {
    if (reading != alcStableState) {
      alcStableState = reading;
      alcoholEmpty = (alcStableState == ALCOHOL_EMPTY_LEVEL);

      if (alcoholEmpty) {
        Serial.println("[WARNING] Tangki Alkohol KOSONG!");
      } else {
        Serial.println("[INFO] Tangki Alkohol TERISI.");
      }
    }
  }
}

// ---------------------------------------------------------
// Handler Relays
// ---------------------------------------------------------
void handleRelay1(unsigned long now) {
  if (relay1Active && (now - relay1StartTime >= RELAY1_PULSE_MS)) {
    relay1Active = false;
    digitalWrite(PIN_RELAY1, RELAY_INACTIVE);
  }
}

void handleRelay2(unsigned long now) {
  if (relay2Active && (now - relay2StartTime >= RELAY2_DURATION_MS)) {
    relay2Active = false;
    digitalWrite(PIN_RELAY2, RELAY_INACTIVE);

    if (!relay2ExpiryChecked) {
      relay2ExpiryChecked = true;

      if (!doorClosed && triggeredByProx1) {
        triggerDoorAlarmIfNeeded(now);
      }
    }
  }
}

void triggerDoorAlarmIfNeeded(unsigned long now) {
  if (!buzzerDoorAlarmActive) {
    buzzerDoorAlarmActive = true;
    buzzerAlarmStartTime = now;
    buzzerLastToggleTime = now;
    buzzerAlarmCount++;

    Serial.print("[ALARM PINTU] Pintu BELUM tertutup! Kejadian ke-");
    Serial.println(buzzerAlarmCount);
  }
}

// ---------------------------------------------------------
// Handler Utama Buzzer (Prioritas & Variasi Nada/Pola)
// ---------------------------------------------------------
void handleBuzzer(unsigned long now) {
  // Matikan alarm pintu jika pintu sudah tertutup
  if (buzzerDoorAlarmActive && doorClosed) {
    onDoorClosed(now);
  }

  // PRIO 1: Alarm Pintu Terbuka (Beep Cepat: 200ms ON / 200ms OFF)
  if (buzzerDoorAlarmActive) {
    if (now - buzzerLastToggleTime >= BUZZER_DOOR_BLINK_MS) {
      buzzerLastToggleTime = now;
      buzzerPinState = !buzzerPinState;
      digitalWrite(PIN_BUZZER, buzzerPinState ? BUZZER_ON : BUZZER_OFF);
    }
  } 
  // PRIO 2: Alarm Alkohol Habis (Beep Panjang: 800ms ON / 400ms OFF)
  else if (alcoholEmpty) {
    unsigned long interval = buzzerPinState ? BUZZER_ALC_ON_MS : BUZZER_ALC_OFF_MS;
    
    if (now - buzzerLastToggleTime >= interval) {
      buzzerLastToggleTime = now;
      buzzerPinState = !buzzerPinState;
      digitalWrite(PIN_BUZZER, buzzerPinState ? BUZZER_ON : BUZZER_OFF);
    }
  } 
  // kondisi normal / tidak ada alarm
  else {
    if (buzzerPinState) {
      buzzerPinState = false;
      digitalWrite(PIN_BUZZER, BUZZER_OFF);
    }
  }
}
