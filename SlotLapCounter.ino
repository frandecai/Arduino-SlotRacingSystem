#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "pitches.h"

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args) write(args);
#else
#define printByte(args) print(args,BYTE);
#endif

#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 20
#define LCD_ROWS 4
#define SELECT_BUTTON_PIN 2
#define CHOOSE_CANCEL_BUTTON_PIN 3
#define SENSOR_LANE_1_PIN 4
#define SENSOR_LANE_2_PIN 5
#define POWER_LANE_1_PIN 8
#define POWER_LANE_2_PIN 9
#define BUZZER_PIN 9

#define QUARTER  1000

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

boolean cleanScreen, inRace, pressedSelectButton,
        pressedChooseCancelButton, countDown, results,
        readSensor1, readSensor2 = false;

int option = 1;
int segs1, segs2, laps1, laps2 = 0;

int controller1_pin = A0;
int controller2_pin = A1;

float now1, now2, time1, time2, best1, best2 = 0;

// Custom chars
uint8_t chequeredFlag[8] = {
  0x15, 0x0A, 0x15, 0x0A, 0x15, 0x0A, 0x15, 0x0A
};
uint8_t clock[8] = {
    0x0,0xe,0x15,0x17,0x11,0xe,0x0
};
uint8_t heart[8] = {
    0x0,0xa,0x1f,0x1f,0xe,0x4,0x0
};

int melody[] = {
  NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5,
  NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_C5,
  NOTE_C6, NOTE_G6, NOTE_E6, NOTE_C6, NOTE_G6, NOTE_E6,
  
  NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_B5,
  NOTE_FS5, NOTE_DS5, NOTE_DS5, NOTE_E5, NOTE_F5,
  NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_B5
};

int durations[] = {
  16, 16, 16, 16,
  32, 16, 8, 16,
  16, 16, 16, 32, 16, 8,
  
  16, 16, 16, 16, 32,
  16, 8, 32, 32, 32,
  32, 32, 32, 32, 32, 16, 8
};


void setup()
{
    Serial.begin(9600);

    pinMode(SELECT_BUTTON_PIN, INPUT);
    pinMode(CHOOSE_CANCEL_BUTTON_PIN, INPUT);
    pinMode(SENSOR_LANE_1_PIN, INPUT);
    pinMode(SENSOR_LANE_2_PIN, INPUT);
    pinMode(POWER_LANE_1_PIN, OUTPUT);
    pinMode(POWER_LANE_2_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    lcd.init();
    lcd.backlight();

    lcd.createChar(0, chequeredFlag);
    lcd.createChar(1, clock);
    lcd.createChar(2, heart);

    // Displays presentation
    String initialMessage1 = "Frandecai's Slot";
    String initialMessage2 = " Racing System";

    for (int i = 0; i < LCD_COLUMNS; i++) {
        lcd.setCursor(i, 0);
        lcd.write(byte(0));
        lcd.setCursor(i, 3);
        lcd.write(byte(0));
    }

    for (int i = 0; i < LCD_ROWS; i++) {
        lcd.setCursor(0, i);
        lcd.write(byte(0));
        lcd.setCursor((LCD_COLUMNS - 1), i);
        lcd.write(byte(0));
    }

    delay(1000);

    lcd.setCursor(2, 1);
    lcd.print("Frandecai's SLOT");
    lcd.setCursor(3, 2);
    lcd.print("Racing System");

    digitalWrite(BUZZER_PIN, LOW);
    //playMelody();
    delay(4000);
    lcd.clear();
}


void loop()
{
    // Serial.print("Option ");
    // Serial.println(option);
    // Serial.print("Button ");
    // Serial.println(digitalRead(SELECTBUTTONPIN));
    // Serial.print("pressedSelectButton ");
    // Serial.println(pressedSelectButton);
    // Serial.print("inRace ");
    // Serial.println(inRace);
    // Serial.print("SENSOR_LANE_1 ");
    // Serial.println(digitalRead(SENSOR_LANE_1));
    // Serial.print("countDown ");
    // Serial.println(countDown);
    

    // MENU
    if (inRace == false) {
        readSensor1 = false;
        readSensor2 = false;
        countDown = false;
        results = false;
        now1 = millis();
        now2 = millis();
        time1 = 0;
        time2 = 0;
        best1 = 0;
        best2 = 0;
        laps1 = 0;
        laps2 = 0;

        if ((digitalRead(SELECT_BUTTON_PIN) == HIGH) && (pressedSelectButton == false)) {
            pressedSelectButton = true;

            if (option < 3) {
                option++;
            } else {
                option = 1;
            }
        } else if ((digitalRead(SELECT_BUTTON_PIN) == LOW) && (pressedSelectButton == true)) {
            pressedSelectButton = false;
        }

        // Press choose button in menu
        if ((digitalRead(CHOOSE_CANCEL_BUTTON_PIN) == HIGH) && (pressedChooseCancelButton == false)) {
            pressedChooseCancelButton = true;

            inRace = true;
            cleanScreen = true;
        } else if ((digitalRead(CHOOSE_CANCEL_BUTTON_PIN) == LOW) && (pressedChooseCancelButton == true)) {
            pressedChooseCancelButton = false;
        }

        // Show menu
        menu();

    // GO TO RACE MODE
    } else {
        if ((digitalRead(CHOOSE_CANCEL_BUTTON_PIN) == HIGH) && (pressedChooseCancelButton == false)) {
            pressedChooseCancelButton = true;

            inRace = false;
            cleanScreen = true;
            results = true;
        } else if ((digitalRead(CHOOSE_CANCEL_BUTTON_PIN) == LOW) && (pressedChooseCancelButton == true)) {
            pressedChooseCancelButton = false;
        }

        switch (option) {
            case 1:
                freeRacing();

                break;
            case 2:
                freeLapRacing();

                break;
            case 3:
                lapRacing();

                break;
        }
    }

    if (cleanScreen == true) {
        lcd.clear();
        cleanScreen = false;
    }
}


void playMelody() {
    // noTone(BUZZER_PIN);

    // int size = sizeof(durations) / sizeof(int);

    // for (int note = 0; note < size; note++) {
    //     //to calculate the note duration, take one second divided by the note type.
    //     //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    //     int duration = 1000 / durations[note];
    //     tone(BUZZER_PIN, melody[note], duration);

    //     //to distinguish the notes, set a minimum time between them.
    //     //the note's duration + 30% seems to work well:
    //     int pauseBetweenNotes = duration * 1.30;
    //     delay(pauseBetweenNotes);

    //     //stop the tone playing:
    //     noTone(BUZZER_PIN);
    // }

    tone(BUZZER_PIN, 1000, 128);
}


void menu()
{
    lcd.setCursor(6, 0);
    lcd.print("* MENU *");
    lcd.setCursor(0, 1);
    lcd.print("Best time racing ");
    lcd.setCursor(0, 2);
    lcd.print("Best lap racing  ");
    lcd.setCursor(0, 3);
    lcd.print("Lap racing       ");

    switch (option) {
        case 1:
            lcd.setCursor(17, 1);
            lcd.print("<--");
            lcd.setCursor(17, 2);
            lcd.print("   ");
            lcd.setCursor(17, 3);
            lcd.print("   ");
        
            break;
        case 2:
            lcd.setCursor(17, 1);
            lcd.print("   ");
            lcd.setCursor(17, 2);
            lcd.print("<--");
            lcd.setCursor(17, 3);
            lcd.print("  ");
        
            break;
        case 3:
            lcd.setCursor(17, 1);
            lcd.print("   ");
            lcd.setCursor(17, 2);
            lcd.print("   ");
            lcd.setCursor(17, 3);
            lcd.print("<--");
        
            break;
    }
}


void freeRacing()
{
    if (countDown == false) {
        showCountDown();

        countDown = true;
    } else {
        if (results == false) {
            showTitlesInRace();
            timeLapControl();
        } else {
            showResults();
        }
    }
}


void freeLapRacing()
{
    if (countDown == false) {
        showCountDown();

        countDown = true;
    } else {
        if (results == false) {
            showTitlesInRace();
            timeLapControl();
        } else {
            showResults();
        }
    }
}


void lapRacing()
{
    if (countDown == false) {
        showCountDown();

        countDown = true;
    } else {
        if (results == false) {
            showTitlesInRace();
            timeLapControl();
        } else {
            showResults();
        }
    }
}


void showCountDown()
{
    for (int countDown = 3; countDown > 0; countDown--) {
        String string = "";
        for (int cols = 1; cols <= 20; cols++) {
            string += String(countDown);
        }

        for (int i = 0; i <= 3; i++) {
            lcd.setCursor(0, i);
            lcd.print(string);
        }

        delay(1000);
    }

    cleanScreen = true;
}


void showTitlesInRace()
{
    lcd.setCursor(2, 0);
    lcd.print("PILOT 1  PILOT 2");
    lcd.setCursor(0, 1);
    lcd.printByte(1);
    lcd.setCursor(0, 2);
    lcd.printByte(2);
    lcd.setCursor(0, 3);
    lcd.printByte(0);
}


void timeLapControl()
{
    // Set the outputs values for voltage in both lanes
    int valueController1 = analogRead(controller1_pin);
    int valueController2 = analogRead(controller2_pin);
    int powerLane1 = map(valueController1, 0, 1023, 0, 255);
    int powerLane2 = map(valueController2, 0, 1023, 0, 255);
    analogWrite(POWER_LANE_1_PIN, powerLane1);
    analogWrite(POWER_LANE_2_PIN, powerLane2);

    // Calculations for times, bests and laps
    time1 = (millis() - now1) / 1000;
    lcd.setCursor(4, 1);
    lcd.print(time1, 1);

    time2 = (millis() - now2) / 1000;
    lcd.setCursor(13, 1);
    lcd.print(time2, 1);

    if ((digitalRead(SENSOR_LANE_1_PIN) == LOW) && (readSensor1 == false)) { 
        readSensor1 = true;

    } else if ((digitalRead(SENSOR_LANE_1_PIN) == HIGH) && (readSensor1 == true)) {
        readSensor1 = false;
        laps1++;
        now1 = millis();

        if (laps1 == 1) {
            best1 = time1;
        } else {
            if (time1 < best1) {
                best1 = time1;
            }
        }

        lcd.setCursor(4, 2);
        lcd.print(best1, 1);

        if ((option == 2) || (option == 3)) {
            lcd.setCursor(5, 3);
            lcd.print(laps1, 1);
        }
    }

    if ((digitalRead(SENSOR_LANE_2_PIN) == LOW) && (readSensor2 == false)) {
        readSensor2 = true;

    } else if ((digitalRead(SENSOR_LANE_2_PIN) == HIGH) && (readSensor2 == true)) {
        readSensor2 = false;
        laps2++;
        now2 = millis();

        if (laps2 == 1) {
            best2 = time2;
        } else {
            if (time2 < best2) {
                best2 = time2;
            }
        }

        lcd.setCursor(4, 2);
        lcd.print(best2, 1);

        if ((option == 2) || (option == 3)) {
            lcd.setCursor(15, 3);
            lcd.print(laps2, 1);
        }
    }
}


void showResults()
{
    String winner = "";
    String bestTime = "";
    String laps = "";

    if (best1 == 0) {
        winner = "Pilot 2";
        bestTime = String(best2);
        laps = String(laps2);
    } else if (best2 == 0) {
        winner = "Pilot 1";
        bestTime = String(best1);
        laps = String(laps1);
    } else {
        if (best1 < best2) {
            winner = "Pilot 1";
            bestTime = String(best1);
            laps = String(laps1);
        } else {
            winner = "Pilot 2";
            bestTime = String(best2);
            laps = String(laps2);
        }
    }

    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("FINAL RESULTS");
    lcd.setCursor(0, 1);
    lcd.print("Winner: " + winner);
    lcd.setCursor(0, 2);
    lcd.print("Best Time: " + bestTime);

    if ((option == 2) || (option == 3)) {
        lcd.setCursor(0, 3);
        lcd.print("Laps: " + laps);
    }
    
    delay(5000);
}
