#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// Arduino pins
// const int output_photodiode = A1;
// const int button_input_blank = A2;
// const int button_input_read = A3;

// ESP32 pins
const int output_photodiode = 27;
const int button_input_blank = 16;
const int button_input_read = 4;

// uncomment this (and below) and connect led to pin 15 if you want it to not stay on continuously, but how else will you know which way to put the cuvette in? lol
// const int ledPin = 15;

float blankReading = 0.0;
float value = 0.0;
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16charsand2line display

void setup() {
  Serial.begin(9600);
  Serial.println("Starting");
  //analogReference(EXTERNAL);
  analogReadResolution(10);  
  analogSetWidth(10);              
  analogSetAttenuation(ADC_0db);
  pinMode(output_photodiode, INPUT);
  pinMode(button_input_blank, INPUT_PULLUP);
  pinMode(button_input_read, INPUT_PULLUP);

  // Set welcome screen
  lcd.init(); // initialize the lcd
  lcd.backlight(); // open the backlight
  lcd.setCursor(2, 0); 
  lcd.print("SemiQuant.com"); // Print a message to the LCD
  lcd.setCursor(2, 1); // set the cursor to column 2, line 1
  lcd.print("count4you"); // Print a message to the LCD.

  delay(2000);

  // Show Blank message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Blank reading:");
  lcd.setCursor(0, 1);
  lcd.print("Click Blank");
  
  // Wait for button_input_blank to be pressed
  while (digitalRead(button_input_blank) == HIGH) {
    // Wait for the button to be clicked
  }

  lcd.clear();
  lcd.print("Blank reading");
  // Get and display Blank reading
  blankReading = measureIntensity();
  Serial.print(blankReading);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Blank reading");
  lcd.setCursor(0, 1);
  lcd.print("Blank read");
  delay(2000);

  // Ask for Read
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("OD reading: ");
  lcd.setCursor(0, 1);
  lcd.print("Click Read");
}

void loop() {
  int buttonState = digitalRead(button_input_blank);
  int buttonState2 = digitalRead(button_input_read);

  // Serial.println(measureIntensity());

  // Serial.println("blankReading1");
  // Serial.println(blankReading);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == LOW) {
    lcd.clear();
    lcd.print("Blank reading");
    blankReading = measureIntensity();  // Corrected line
    Serial.print(blankReading);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Blank reading");
    lcd.setCursor(0, 1);
    lcd.print("Blank read");
    delay(1000);

    if (blankReading > 850){
      lcd.setCursor(0, 0);
      lcd.print("Underestimates as");
      lcd.setCursor(0, 1);
      lcd.print("blank 0 impedance");
    }
  };

  // Serial.println("blankReading2");
  // Serial.println(blankReading);

  if (buttonState2 == LOW) {
    lcd.clear();
    lcd.print("OD reading:");
    value = measureIntensity();
    
    // Serial.println(value);
    // float od = log10(1.0 / (value / blankReading));
    float od = -log(value / blankReading);

    lcd.setCursor(0, 1);
    if (value > 850 || value < 20 || value >= (blankReading - 4)){
      lcd.print("out of range");
      // lcd.clear();
      // lcd.setCursor(0, 0);
      // lcd.print(blankReading);
      // lcd.setCursor(0, 1);
      // lcd.print(value);
    }else{
      lcd.print(od);
    }
    
    Serial.println("value");
    Serial.println(value);
    Serial.println("blankReading3");
    Serial.println(blankReading);
  };

  delay(500);
}


// this esp is super noisy, can figure out why, so trying to filter that out
// float measureIntensity() {
//     int sumValues = 0;
//     int reads = 0;
//     // digitalWrite(ledPin, HIGH);
//     // delay(500);
//     while (reads < 20) {
//         sumValues += analogRead(output_photodiode);
//         reads++;
//         delay(200);
//     }
//     // digitalWrite(ledPin, LOW);
//     float averageIntensity = sumValues / reads;
//     Serial.println(averageIntensity);  // Print average intensity
//     // averageIntensity = map(averageIntensity, 20, 850, 0, 1000);
//     return averageIntensity;
// }

float measureIntensity() {
    const int maxReads = 10;
    const float tolerance = 0.05; // 5% tolerance
    const unsigned long timeout = 5000; // 5 seconds timeout

    int sumValues = 0;
    int reads = 0;
    float lastIntensity = 0;

    while (true) {
        unsigned long startTime = millis();

        while (reads < maxReads && (millis() - startTime) < timeout) {
            int currentValue = analogRead(output_photodiode);

            if (reads > 0 && abs(currentValue - lastIntensity) / lastIntensity > tolerance) {
                // Skip if the current value differs by more than 5% from the last one
                continue;
            }

             Serial.print(";" + String(currentValue));

            sumValues += currentValue;
            lastIntensity = currentValue;
            reads++;
            delay(200);
        }
        

        if (reads == maxReads) {
            // Successfully obtained 10 consecutive readings within tolerance
            break; // Exit the outer loop
        }

        // Reset variables for a new attempt
        sumValues = 0;
        reads = 0;
    }

    float averageIntensity = sumValues / reads;
    Serial.println(averageIntensity);  // Print average intensity

    return averageIntensity;
}


