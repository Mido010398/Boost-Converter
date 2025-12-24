/*
Copyright (c) 2025 AstraVolta Team

Name Project: BOOST Converter 
Team : Astra Volt
This Arduino sketch controls a DC–DC boost converter (team: Astra Volt).
Main features:
overview :  
- Generates a PWM signal (50 kHz) using TimerOne to drive the switching transistor (pin 9).
- Measures output current using an INA219 sensor.
- Measures the converter output voltage through an analog voltage divider on A1.
- Shows measured voltage and current on a 16×2 I²C LCD.
- Implements a very simple feedback loop that increments/decrements PWM duty to try to match a "theoretical" boost output 
calculated from the current duty value.
*/

#include <Wire.h>                // I2C library
#include <Adafruit_INA219.h>     // INA219 current sensor library
#include <TimerOne.h>            // Timer1 library for PWM at custom frequency
#include <LiquidCrystal_I2C.h>  // LCD I2C library

// ------------------ Pins ------------------
#define loadVoltagePin  A1   // Analog pin connected to voltage divider (Vout measurement)
#define potPin          A0   // Analog pin connected to potentiometer (reference for duty cycle)

// ------------------ Objects ------------------
Adafruit_INA219 ina219;            // INA219 current sensor object
LiquidCrystal_I2C lcd(0x27,16,2); // LCD I2C object (0x27 is common I2C address)

// ------------------ Variables ------------------
int duty = 0;                  // PWM duty value (0-1023)
float current_mA = 0;          // Measured current in mA from INA219
float V_actual = 0;            // Measured voltage on load via voltage divider
int adc;                        // Raw ADC reading for load voltage
float Vadc;                     // Converted voltage from ADC

// ------------------ Constants ------------------
float Vin = 12.5;              // Input voltage to the boost converter
float R1 = 100000.0;           // Voltage divider resistor R1 in Ohms
float R2 = 1000.0;            // Voltage divider resistor R2 in Ohms

void setup() {
  Serial.begin(115200);          // Start serial communication for debugging
  while (!Serial) { delay(1); }  // Wait for serial to be ready (needed on some boards)

  // ---------- INA219 Setup ----------
  if (!ina219.begin()) {          // Initialize INA219 sensor
    Serial.println("Failed to find INA219 chip"); 
    while(1){ delay(10); }       // Stop execution if INA219 is not detected
  } 


  // ---------- LCD Setup ----------
  lcd.init();                     // Initialize LCD
  lcd.backlight();                // Turn on backlight
  lcd.clear();                    // Clear LCD
  lcd.setCursor(0,0);
  lcd.print("Astra Volt");        // Initial message
  delay(800);
  // ---------- PWM Setup ----------
 pinMode(9, OUTPUT); Timer1.initialize(20);          // Set Timer1 period: 20us → 50kHz PWM
  Timer1.pwm(9, 0);               // Start PWM on pin 9 with 0 duty

  pinMode(loadVoltagePin, INPUT); // Configure load voltage pin as input
  pinMode(potPin, INPUT);         // Configure potentiometer pin as input
  lcd.clear();                    // Clear LCD for main loop
}

void loop() {
  // -------- Read potentiometer to control Duty --------
  int potRaw = analogRead(potPin);      // Read potentiometer (0-1023)
  float D =  potRaw / 1023.0;              // Convert current PWM duty to fraction (0-1)
  float V_theoretical = Vin / (1.0 - D);  // Calculate theoretical Vout from current duty (Boost formula)

  // -------- Read actual load voltage --------
  adc = analogRead(loadVoltagePin);     // Read ADC value from voltage divider
  Vadc = adc * (5.0 / 1023.0);          // Convert ADC to voltage (0-5V)
  V_actual = Vadc* ((R2+R1) /(R2));     // Convert to actual load voltage using voltage divider formula

  // -------- Read current from INA219 --------
  current_mA = -1*ina219.getCurrent_mA();  // Read current in mA

  // -------- Simple feedback control --------
  if(V_theoretical > V_actual) {          // If theoretical voltage higher than measured
    duty++;                               // Increase PWM duty
  } else if(V_theoretical < V_actual) {   // If theoretical voltage lower than measured
    duty--;                               // Decrease PWM duty
  }
  duty = constrain(duty,0,1023);          // Ensure duty stays within 0-1023
  Timer1.setPwmDuty(9,duty);              // Apply PWM duty to pin 9

  // -------- LCD Display --------
  lcd.setCursor(0,0);
  lcd.print("Vout:");                     
  lcd.print(V_actual,1);                  // Show measured voltage
  lcd.print("V   ");                      // Clear extra characters

  lcd.setCursor(0,1);
  lcd.print("I:");                       
  lcd.print(current_mA,1);                // Show measured current
  lcd.print("mA ");                     // Clear extra characters

  lcd.setCursor(11,1);
  lcd.print("D:");                       
  lcd.print(D*100,0);                // Show measured current
  lcd.print("% ");                     // Clear extra characters



  // -------- Serial Monitor --------
  Serial.print("Vout="); Serial.print(V_actual);
  Serial.print(" | I="); Serial.print(current_mA);
  Serial.print("mA | PWM="); Serial.println(potRaw);
delay(150);    // Small delay for stability
}