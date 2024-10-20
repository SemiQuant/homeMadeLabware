#include <Wire.h>
#include <math.h>
#include <EEPROM.h>
// #include <RTClib.h>
#include <U8g2lib.h>

U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R2, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // Adafruit Feather ESP8266/32u4 Boards + FeatherWing OLED

// const int ph_meter = A0;
// const int temp_meter = A1;
const int buttonStart = 2; 
const int buttonSelect = 3;
// RTC_DS3231 rtc;

bool readSTD = true;

const int line1 = 10;
const int line2 = 21;
const int line3 = 32;

struct stds_obj {
  int ph4;
  int ph7;
  int ph10;
};

void processStandards() {
    u8g2.setFont(u8g2_font_6x13B_tf);
    int tmp_ph4;
    int tmp_ph7;
    int tmp_ph10;

    if (readSTD) {
        // EEPROM.put(0, rtc.now());
        // EEPROM.put(0, 1);
        u8g2.clearBuffer();
        u8g2.drawStr(5, line1, "Insert standard ");
        u8g2.drawStr(35, line2, "pH=4");
        u8g2.drawStr(5, line3, "click start.");
        u8g2.sendBuffer();
        while (digitalRead(buttonStart) == HIGH) {
            // wait
        }
        if (digitalRead(buttonStart) == LOW) {
            // EEPROM.put(1, measureIntensity());
            tmp_ph4 = measureIntensity();
            delay(200);
        }
        u8g2.clearBuffer();
        u8g2.drawStr(5, line1, "Insert standard ");
        u8g2.drawStr(35, line2, "pH=7");
        u8g2.drawStr(5, line3, "click start.");
        u8g2.sendBuffer();
        while (digitalRead(buttonStart) == HIGH) {
            // wait
        }
        if (digitalRead(buttonStart) == LOW) {
            // EEPROM.put(2, measureIntensity());
            tmp_ph7 = measureIntensity();
            delay(200);
        }
        u8g2.clearBuffer();
        u8g2.drawStr(5, line1, "Insert standard ");
        u8g2.drawStr(35, line2, "pH=10");
        u8g2.drawStr(5, line3, "click start.");
        u8g2.sendBuffer();
        while (digitalRead(buttonStart) == HIGH) {
            // wait
        }
        if (digitalRead(buttonStart) == LOW) {
            // EEPROM.put(3, measureIntensity());
            tmp_ph10 = measureIntensity();
            delay(200);
        }
    }

    stds_obj stds = {
      tmp_ph4,
      tmp_ph7,
      tmp_ph10
    };

    EEPROM.put(0, stds);

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_9x15B_tf);
  	u8g2.drawStr(18, line2, "STD read!");
    delay(2000);
  	u8g2.sendBuffer();
}

// float measureIntensity(int pin_in) {
float measureIntensity() {
    float sumReadings = 0;
    int numReadings = 12;

    for (int i = 0; i < numReadings; i++) {
        int rawValue = analogRead(A0); 
        sumReadings += rawValue;
        delay(200);
    }

    float average = sumReadings / numReadings;

    return average;
}

class LinearRegression {
private:
    double a, b;

public:
    LinearRegression() : a(0.0), b(0.0) {}

    // Fit function to perform linear regression
    void fit(double x[], double y[], int n) {
        // Calculate mean of x and y
        double mean_x = 0.0, mean_y = 0.0;
        for (int i = 0; i < n; ++i) {
            mean_x += x[i];
            mean_y += y[i];
        }
        mean_x /= n;
        mean_y /= n;

        // Calculate coefficients a and b of the linear regression equation
        double numerator = 0.0, denominator = 0.0;
        for (int i = 0; i < n; ++i) {
            numerator += (x[i] - mean_x) * (y[i] - mean_y);
            denominator += (x[i] - mean_x) * (x[i] - mean_x); // Square the difference
        }
        a = numerator / denominator;
        b = mean_y - a * mean_x;
    }

    // Predict function to predict y value given an x value
    double predict(double x) {
        return a * x + b;
    }

    // Getters for coefficients
    double getA() const { return a; }
    double getB() const { return b; }
};

LinearRegression lr; 

void setup() {
    Serial.begin(9600);
    Serial.println("Starting");

    pinMode(buttonStart, INPUT_PULLUP);
    pinMode(buttonSelect, INPUT_PULLUP);

    // Set welcome screen
    u8g2.begin();
    u8g2.setFont(u8g2_font_9x15B_tf);
  	u8g2.drawStr(0, line1, "SemiQuant.com");
  	u8g2.drawStr(0, line3, "phat Head");
  	u8g2.sendBuffer();
    u8g2.setFont(u8g2_font_squeezed_r7_tr);

    delay(2000);

    // if (!rtc.begin()) {
    //     Serial.println("Couldn't find RTC");
    //     Serial.flush();
    //     while (1) delay(10);
    // }

    // if (rtc.lostPower()) {
    //     Serial.println("RTC lost power, let's set the time!");
    //     rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // }

    // Serial.println("cust started");
    // DateTime sdt_date;
    // EEPROM.get(0, sdt_date);
    // bool std_stored = sdt_date > 255; // != 0 or error 225
    // DateTime currentTime = rtc.now();
    // bool std_stored = true;
    // int daysPassed = 20;
    
    // u8g2.clearBuffer();
    // if (std_stored) {
    //   // int daysPassed = currentTime.day() - sdt_date.day();
    //   u8g2.drawStr(0, line1, (String(F("Days since cal: ")) + String(daysPassed)).c_str());
    // 	u8g2.drawStr(0, line2, "select=reread ");
    //   u8g2.drawStr(0, line3, "start=use");
    // } else {
    // 	u8g2.drawStr(0, line1, "STD not read!");
    // 	u8g2.drawStr(0, line2, "select=read stds");
    //   u8g2.drawStr(0, line3, "start=use defaults");
    // }
    // u8g2.sendBuffer();

    u8g2.clearBuffer();
    u8g2.drawStr(0, line1, "Standards");
    u8g2.drawStr(0, line2, "select = reread");
    u8g2.drawStr(0, line3, "start = use");
    u8g2.sendBuffer();

    // int std_store;
    // EEPROM.get(0, std_store);
    // bool std_stored = std_store == 1;

  while (digitalRead(buttonSelect) == HIGH && digitalRead(buttonStart) == HIGH) {
    // Do nothing, just wait
    delay(100);
  }

    if (digitalRead(buttonSelect) == LOW) {
        processStandards();
    }
    // else if (digitalRead(buttonStart) == LOW && !std_stored) {
    //       // EEPROM.put(1, 633);
    //       // EEPROM.put(2, 529);
    //       // EEPROM.put(3, 424);
    //       stds_obj stds = {
    //         633,
    //         529,
    //         424
    //       };
    //       EEPROM.put(0, stds);
    // }else{
    //     // started = true;
    // }
        // get curve
        // int pH4;
        // int pH7;
        // int pH10;

        stds_obj read_stds;
        EEPROM.get(0, read_stds);

        // EEPROM.get(1, pH4); 
        // EEPROM.get(2, pH7); 
        // EEPROM.get(3, pH10);

        // u8g2.refreshDisplay();
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_9x15B_tf);
    	  u8g2.drawStr(0, line1, "pH:");
    	  u8g2.sendBuffer();

        // Sample data
        double x[] = { read_stds.ph4, read_stds.ph7, read_stds.ph10 };
        double y[] = { 4, 7, 10 };
        int n = sizeof(x) / sizeof(x[0]);

        // Fit the data
        lr.fit(x, y, n);

        delay(500);
}

void loop() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_9x15B_tf);
    u8g2.drawStr(0, line1, "pH:");

    // int int_reading;
    // int pH4;
    // int pH7;
    // int pH10;

    int pH_read = measureIntensity();
    Serial.println(pH_read);
    double predicted_pH = lr.predict(pH_read);

    // bool paused = false;
    // while (!paused) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_9x15B_tf);
        u8g2.drawStr(0, line1, "pH:");

        // if (digitalRead(buttonStart) == LOW) {
        //     paused = true;
        // }

        if (predicted_pH < 3 || predicted_pH > 12) {
              u8g2.drawStr(0, line2, "Caution");
              u8g2.drawStr(0, line3, String(predicted_pH).c_str());
        } else {
            // lcd.print(String(predicted_pH));
            u8g2.drawStr(4, line3, String(predicted_pH).c_str());
        }
        // u8g2.drawStr(0, line3, ("Ambient temp: " + String(rtc.getTemperature())).c_str());
        u8g2.sendBuffer();
        delay(500);
    // }

    // while (digitalRead(buttonStart) == LOW) {
    //     paused = false;
    // }
}



