/*
 * OD600 Spectrophotometer - Final
 * Hardware:
 *   - Arduino Nano
 *   - AS7341 breakout (I2C, addr 0x39)
 *   - LCD1602 with PCF8574 I2C backpack (addr 0x27)
 *   - 600nm LED via 2N2222 transistor, PWM on D6
 *   - Button LID   -> D2 (INPUT_PULLUP, LOW = lid closed)
 *   - Button BLANK -> D3 (INPUT_PULLUP)
 *   - Button READ  -> D4 (INPUT_PULLUP)
 */

#include <Wire.h>
#include <Adafruit_AS7341.h>
#include <LiquidCrystal_I2C.h>

#define PIN_LED_PWM   6
#define PIN_BTN_LID   2
#define PIN_BTN_BLANK 3
#define PIN_BTN_READ  4

#define LED_PWM_VALUE 180
#define NUM_AVERAGES  5
#define SETTLE_MS     50
#define DEBOUNCE_MS   300
#define LCD_I2C_ADDR  0x27

#define OD_MIN  0.05
#define OD_MAX  1.20

Adafruit_AS7341 as7341;
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 16, 2);

float g_I0 = 0;
bool  g_hasBlank = false;
unsigned long g_lastBlankPress = 0;
unsigned long g_lastReadPress  = 0;

// ── LCD helper — line 0 and 1 swapped for upside-down mount ─────
void lcdMsg(const char* top, const char* bot) {
  lcd.clear();
  lcd.setCursor(0, 1); lcd.print(top);  // physically top = logical row 1
  lcd.setCursor(0, 0); lcd.print(bot);  // physically bottom = logical row 0
}

// ════════════════════════════════════════════════════════════════
float readDarkSubtracted() {
  uint16_t readings[12];
  analogWrite(PIN_LED_PWM, 0);
  delay(SETTLE_MS);
  as7341.readAllChannels(readings);
  float dark = (float)readings[AS7341_CHANNEL_590nm_F6];
  analogWrite(PIN_LED_PWM, LED_PWM_VALUE);
  delay(SETTLE_MS);
  as7341.readAllChannels(readings);
  float lit = (float)readings[AS7341_CHANNEL_590nm_F6];
  analogWrite(PIN_LED_PWM, 0);
  return lit - dark;
}

float takeMeasurement() {
  float total = 0;
  for (int i = 0; i < NUM_AVERAGES; i++) total += readDarkSubtracted();
  return total / NUM_AVERAGES;
}

bool lidIsClosed() {
  return (digitalRead(PIN_BTN_LID) == LOW);
}

void showReady() {
  if (!g_hasBlank) {
    lcdMsg("Press BLANK", "to set blank");
  } else {
    lcdMsg("Ready BLANK/READ", "Blank: set");
  }
}

// ════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);

  pinMode(PIN_BTN_LID,   INPUT_PULLUP);
  pinMode(PIN_BTN_BLANK, INPUT_PULLUP);
  pinMode(PIN_BTN_READ,  INPUT_PULLUP);
  pinMode(PIN_LED_PWM,   OUTPUT);
  analogWrite(PIN_LED_PWM, 0);

  lcd.init();
  lcd.backlight();
  lcdMsg("www.DrDx.Me", "see4U2");
  delay(2500);

  lcdMsg("Initialising", "sensor...");
  if (!as7341.begin()) {
    lcdMsg("SENSOR ERROR", "Check wiring!");
    Serial.println("ERR: AS7341 not found.");
    while (true) { delay(1000); }
  }

  as7341.setATIME(99);
  as7341.setASTEP(999);
  as7341.setGain(AS7341_GAIN_128X);

  Serial.println("OD600 ready.");
  Serial.println("D2=Lid  D3=Blank  D4=Read");
  Serial.println("---");

  showReady();
}

// ════════════════════════════════════════════════════════════════
void loop() {
  unsigned long now = millis();

  // ── BLANK ─────────────────────────────────────────────────────
  if (digitalRead(PIN_BTN_BLANK) == LOW && (now - g_lastBlankPress) > DEBOUNCE_MS) {
    g_lastBlankPress = now;

    if (!lidIsClosed()) {
      lcdMsg("Close lid", "then blank!");
      Serial.println("WARN: Lid open — blank aborted.");
      delay(2000);
      showReady();
      return;
    }

    lcdMsg("Blanking...", "Keep lid closed");
    Serial.println("Blanking...");

    float sig = takeMeasurement();
    if (sig <= 0) {
      lcdMsg("Blank failed", "Check sensor");
      Serial.println("ERR: Blank signal <= 0.");
      delay(2000);
      showReady();
      return;
    }

    g_I0 = sig;
    g_hasBlank = true;

    char buf[17];
    dtostrf(sig, 8, 1, buf);
    lcdMsg("Blank set!", buf);
    Serial.print("Blank I0 = ");
    Serial.println(sig, 1);
    delay(2000);
    showReady();
  }

  // ── READ ──────────────────────────────────────────────────────
  if (digitalRead(PIN_BTN_READ) == LOW && (now - g_lastReadPress) > DEBOUNCE_MS) {
    g_lastReadPress = now;

    if (!lidIsClosed()) {
      lcdMsg("Close lid", "then read!");
      Serial.println("WARN: Lid open — read aborted.");
      delay(2000);
      showReady();
      return;
    }

    if (!g_hasBlank) {
      lcdMsg("No blank set!", "Press BLANK 1st");
      Serial.println("WARN: No blank set.");
      delay(2500);
      showReady();
      return;
    }

    lcdMsg("Reading...", "Keep lid closed");
    Serial.println("Reading...");

    float sig = takeMeasurement();
    if (sig <= 0) {
      lcdMsg("Read failed", "Check sensor");
      Serial.println("ERR: Read signal <= 0.");
      delay(2000);
      showReady();
      return;
    }

    float od = log10(g_I0 / sig);
    if (od < 0)   od = 0;
    if (od > 4.0) od = 4.0;

    // Linear calibration correction
    od = 1.055 * od - 0.006;
    if (od < 0) od = 0;

    // Range check
    bool inRange = (od >= OD_MIN && od <= OD_MAX);

    // Serial output
    Serial.print("OD600 = ");
    Serial.print(od, 3);
    Serial.print("  |  signal = ");
    Serial.print(sig, 1);
    Serial.print("  |  I0 = ");
    Serial.print(g_I0, 1);
    Serial.print("  |  ");
    Serial.println(inRange ? "OK" : "OUT OF RANGE");

    // LCD output
    char valBuf[8];
    dtostrf(od, 5, 3, valBuf);
    char line2[17];
    snprintf(line2, 17, "OD: %s", valBuf);
    if (!inRange) {
      if (od < OD_MIN) lcdMsg(line2, "Too low <0.05");
      else             lcdMsg(line2, "Too high >1.2");
    } else {
      lcdMsg(line2, "In range OK");
    }

    // Hold until next button press
    while (digitalRead(PIN_BTN_READ) == HIGH &&
           digitalRead(PIN_BTN_BLANK) == HIGH) {
      delay(50);
    }
    delay(DEBOUNCE_MS);
    showReady();
  }
}
