/*
 * This code is for Cranfield automatic salt spraying system
 * Software created for Arduino Mega 2560
 * Keypad pins: D2 to D9
 * Servo PWM: D10
 * Motor PWM: D11
 * Motor Direction pin: D12
 * First version created by Cranfield Manufacturing 2020 Group project No.16
 * Modified by Wei Luo
*/
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Servo.h>
#include <EEPROM.h>
#define pul 11
#define dir 12
#define ena 13
#define ROWS 4
#define COLS 4
#define addr 0
#define addrA 50
#define addrB 100
#define addrC 150
Servo myservo;
float time_val = -1, num_val = 1, deg_val = 0, temp;  //temp for recover value when invalid value input
bool time_first = true;
bool num_first = true;
bool deg_first = true;
bool rotate_direction;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'}, 
  {'4', '5', '6', 'B'}, 
  {'7', '8', '9', 'C'}, 
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8, 9};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

LiquidCrystal_I2C lcd(0x27, 20, 4);

int level = 0;
char key;
int ServoPos = 0;


float GetNumber(float val, int limit, bool &first, bool decimal_allow = true, bool return_or_not = true){
  int num = 0;
  float decnum = 0.00;
  int counter = 0;
  int digits = 0;
  bool decOffset = false;
  bool input_flag = false;

  Serial.println(time_val);
  Serial.println(num_val);
  Serial.println(deg_val);

  temp = val; //record previous value
  Serial.println("temp value:");
  Serial.println(temp);

  key = NO_KEY;
  while (key == 'A' && key =='B' && key == 'C' && key == 'D' && key == NO_KEY){
    key = keypad.getKey();
  }

  while(key != '#' && key != 'D'){
    switch(key){
      case '*':
      if (decimal_allow == true){
        if(!decOffset && digits != 0){
          decOffset = true;
          input_flag = true;
          digits++;
          lcd.setCursor(digits, 3);
          lcd.print(".");
        }
      }
      break;
      
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
      if(first == false){
        lcd.setCursor(0, 3);
        lcd.print("                    ");
        first = true;
      }
      if (key == '0' && digits != 0 && num == 0){
        break;
      }
      else {
        input_flag = true;
        num = num * 10 + (key - '0');
        digits++;
        lcd.setCursor(digits, 3);
        lcd.print(key);
        if(decOffset){
          counter++;
        }
      }
      break;
    }
    key = keypad.getKey();
  }
  if (key == '#'){
    decnum = num / pow(10, counter);  //Refresh value
    if (input_flag == false){
      decnum = temp;
    }
    if((limit > 0 && decnum > limit) || (limit == 0 && decnum == 0 && val == 0)){
      lcd.clear();
      lcd.print("Invalid!");
      delay(2000);
      key = 'A';
      level = 0;
      return decnum;
    }
    else{
      lcd.clear();
      lcd.print("Confirming...");
      if (return_or_not == true){
        key = 'A';
        level = 0;
      }
      first = false;
      delay(2000);
      return decnum;
    }
  }
  if (key == 'D'){
    key = 'A';  //back
    level = 0;
    decnum = val; //keep original value
    Serial.print(decnum);
    return decnum;
  }
}

void servo(float time_val, int num_val){
  Serial.println(time_val);
  Serial.println(num_val);
  for(int i = 0; i < num_val; i++){
    Serial.println("servo run");
    myservo.write(ServoPos - 40);  //press
    delay(time_val * 1000); //press delay
    myservo.write(ServoPos); //release
    delay(1000);  //release delay
  }
}

void motor(float deg_val){
  int pulse_number = round((deg_val/360)*6400);
  for(int i = 0; i < pulse_number; i++){
    digitalWrite(pul, HIGH);
    delay(2);
    digitalWrite(pul, LOW);
    delay(2);
  }
  delay(1000);
}

void print_number(float num, bool display_zero = false){
  char string[20];
  if(time_val > 0 || (time_val < 0 && display_zero == true)){
    if(num - floor(num) > 0){
      dtostrf(num, 1, 2, string);
      //Serial.print(string);
      lcd.print(string);
    }
    else{
      itoa(int(num), string, 10);
      //Serial.print(string);
      lcd.print(string);
    }
  }
}

void(* resetFunc) (void) = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // boot up animation
  lcd.init();
  lcd.backlight();
  lcd.setCursor(5, 0);
  lcd.print("Cranfield");
  lcd.setCursor(5, 3);
  lcd.print("Rolls-Royce");
  delay(3000);
  lcd.clear();

  lcd.setCursor(3, 0);
  lcd.print("Automatic Mode");
  lcd.setCursor(7, 1);
  lcd.print("Ver 2.0");
  delay(2000);

  //Initialize servo
  myservo.attach(10);
  //myservo.write(100);
  if (EEPROM.read(addr) == 255){
    myservo.write(100);
  }
  else{
    myservo.write((int)EEPROM.read(addr + 1));
  }

  //Initialize motor
  pinMode(pul, OUTPUT);
  pinMode(dir, OUTPUT);
  pinMode(ena, OUTPUT);
  digitalWrite(dir, LOW);

  //Initialize keypad
  key = NO_KEY;


  //Servo Calibration
  lcd.clear();
  lcd.print("Calibration?");
  lcd.setCursor(0, 1);
  lcd.print("[#]Calibrate Servo");
  lcd.setCursor(0, 2);
  lcd.print("[*]Skip");
  key = NO_KEY;
  while(key != '*' && key != '#'){
    key = keypad.getKey();
  }
  switch(key){
    case '#':
    lcd.clear();
    ServoPos = myservo.read();
    char posStr[20];
    lcd.print("Servo Calibration:");
    lcd.setCursor(0, 1);
    lcd.print("Press [A] or [B]");
    lcd.setCursor(0, 2);
    lcd.print("Current Position:");
    lcd.setCursor(0, 3);
    Serial.println(ServoPos);
    itoa(ServoPos, posStr, 10);
    lcd.print(posStr);
    key = NO_KEY;
    while(key != '#'){
      switch(key){
        case 'A':
        if (ServoPos < 180){
          ServoPos++;
          myservo.write(ServoPos);
          lcd.setCursor(0, 3);
          lcd.print("                    ");
          itoa(ServoPos, posStr, 10);
          lcd.setCursor(0, 3);
          lcd.print(posStr);
          delay(20);
        }
        key = NO_KEY;
        key = keypad.getKey();
        break;

        case 'B':
        if (ServoPos > 40){
          ServoPos--;
          myservo.write(ServoPos);
          lcd.setCursor(0, 3);
          lcd.print("                    ");
          itoa(ServoPos, posStr, 10);
          lcd.setCursor(0, 3);
          lcd.print(posStr);
          delay(20);
        }
        key = NO_KEY;
        key = keypad.getKey();
        break;

        case '0':
        ServoPos = 100;
        myservo.write(ServoPos);
        EEPROM.write(addr, 255);
        lcd.setCursor(0, 3);
        lcd.print("                    ");
        itoa(ServoPos, posStr, 10);
        lcd.setCursor(0, 3);
        lcd.print(posStr);
        delay(20);
        key = NO_KEY;
        key = keypad.getKey();
        break;

        default:
        key = NO_KEY;
        key = keypad.getKey();
        
      }
    }
    EEPROM.write(addr, 1);
    EEPROM.write(addr + 1, ServoPos);
    lcd.clear();
    lcd.print("Calibration");
    lcd.setCursor(0, 1);
    lcd.print("Completed!");
    delay(2000);
    key = NO_KEY;
    break;

    case '*':
    ServoPos = myservo.read();
    EEPROM.write(addr + 1, ServoPos);
    key = NO_KEY;
    break;

  }

  // Main Menu
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Main Page");
  lcd.setCursor(0, 1);
  lcd.print("[A]New Procedure");
  lcd.setCursor(0, 2);
  lcd.print("[B]Saved Procedure");
  lcd.setCursor(0, 3);
  lcd.print("[C]SampleTemperature");

  while(key != 'A' && key != 'B' && key != 'C'){
    key = keypad.getKey();
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  //input from keypad
  if (level == 0){
    switch(key){
      case 'A':
      // New Procedure
      lcd.clear();
      lcd.print("New Procedure");
      lcd.setCursor(0, 1);
      lcd.print("[A]Spray Time");
      lcd.setCursor(0, 2);
      lcd.print("[B]Number of Sprays");
      lcd.setCursor(0, 3);
      lcd.print("[C]Degree of Turn");
      key = NO_KEY;
      while(key != 'A' && key != 'B' && key != 'C' && key != 'D' && key != '#' && key != '*'){
        key = keypad.getKey();
      }
      if (key == 'A' || key == 'B' || key == 'C'){
        level = 1;
        break;
      }
      else if (key == 'D' || key == '*'){
        level = 0;
        break;
      }
      else {
        if (time_val <= 0){
          lcd.clear();
          lcd.print("ERROR!");
          delay(3000);
          key = 'D';
          level = 0;
          break;
        }
        else{
          while (key == '#'){
            lcd.clear();
            lcd.print("Processing...");
            //servo and motor control
            motor(deg_val);
            servo(time_val, num_val);
            delay(1000);
            //mission complete
            lcd.clear();
            lcd.print("Mission Complete");
            lcd.setCursor(0, 1);
            lcd.print("[#]REPEAT");
            lcd.setCursor(0, 2);
            lcd.print("[B]SAVE PROCEDURE");
            lcd.setCursor(0, 3);
            lcd.print("[D]MAIN PAGE");
            key = NO_KEY;
            while (key != '#' && key != 'B' && key != 'D'){
              key = keypad.getKey();
            }
            if (key == 'B'){
              lcd.clear();
              lcd.print("Save As...");
              //delay(2000);
              // put saving code here
              lcd.setCursor(0, 1);
              lcd.print("[A]C Ring");
              lcd.setCursor(0, 2);
              lcd.print("[B]Fatigue");
              lcd.setCursor(0, 3);
              lcd.print("[C]Bobbins");
              key = NO_KEY;
              while (key != 'A' && key != 'B' && key != 'C' && key != '*'){
                key = keypad.getKey();
              }
              switch(key){
                case 'A':
                lcd.clear();
                lcd.print("Saving to [A]...");
                EEPROM.write(addrA, 1);
                Serial.println(EEPROM.read(addrA));
                EEPROM.put(addrA + 1, deg_val);
                EEPROM.put(addrA + 5, time_val);
                EEPROM.put(addrA + 9, num_val);
                EEPROM.write(addrA + 13, rotate_direction);
                delay(1000);
                lcd.clear();
                lcd.print("PROCEDURE SAVED");
                delay(2000);
                break;

                case 'B':
                lcd.clear();
                lcd.print("Saving to [B]...");
                EEPROM.write(addrB, 1);
                EEPROM.put(addrB + 1, deg_val);
                EEPROM.put(addrB + 5, time_val);
                EEPROM.put(addrB + 9, num_val);
                EEPROM.write(addrB + 13, rotate_direction);
                delay(1000);
                lcd.clear();
                lcd.print("PROCECDURE SAVED");
                delay(2000);
                break;

                case 'C':
                lcd.clear();
                lcd.print("Saving to [C]...");
                EEPROM.write(addrC, 1);
                EEPROM.put(addrC + 1, deg_val);
                EEPROM.put(addrC + 5, time_val);
                EEPROM.put(addrC + 9, num_val);
                EEPROM.write(addrC + 13, rotate_direction);
                delay(1000);
                lcd.clear();
                lcd.print("PROCEDURE SAVED");
                delay(2000);
                break;

                case '*':
                lcd.clear();
                lcd.print("SAVE CANCELLED");
                delay(2000);
                break;
              }
              level = 0;
              key = 'D';
              
            }
            else{
              level = 0;
            }
          }
      }
      break;

      case 'B':
      // Saved Procedures
      lcd.clear();
      lcd.print("Saved Procedure");
      lcd.setCursor(0, 1);
      lcd.print("[A]C Ring");
      lcd.setCursor(0, 2);
      lcd.print("[B]Fatigue");
      lcd.setCursor(0, 3);
      lcd.print("[C]Bobbins");
      key = NO_KEY;
      while(key != 'A' && key != 'B' && key != 'C' && key != 'D' && key != '*'){
        key = keypad.getKey();
      }
      switch(key){
        case 'A':
        Serial.println((int)EEPROM.read(addrA));
        if((int)EEPROM.read(addrA) != 1){
          lcd.clear();
          lcd.print("ERROR!");
          lcd.setCursor(0, 1);
          lcd.print("No data found!");
          delay(2000);
          key = 'B';
          level = 0;
        }
        else{
          EEPROM.get(addrA + 1, deg_val);
          EEPROM.get(addrA + 5, time_val);
          EEPROM.get(addrA + 9, num_val);
          Serial.println(deg_val);
          Serial.println(time_val);
          Serial.println(num_val);
          rotate_direction = (bool)EEPROM.read(addrA + 13);
          if (rotate_direction == true){
            digitalWrite(dir, HIGH);
          }
          else{
            digitalWrite(dir, LOW);
          }
          char Str_deg[20];
          char Str_time[20];
          char Str_num[20];
          itoa((int)deg_val, Str_deg, 10);
          dtostrf((float)time_val, 1, 2, Str_time);
          itoa((int)num_val, Str_num, 10);
          lcd.clear();
          lcd.print("Run below?");
          lcd.setCursor(0, 1);
          lcd.print("Degree of Turn:");
          lcd.setCursor(15, 1);
          lcd.print(Str_deg);
          lcd.setCursor(0, 2);
          lcd.print("Spary time:");
          lcd.setCursor(11, 2);
          lcd.print(Str_time);
          lcd.setCursor(0, 3);
          lcd.print("Number of sprays:");
          lcd.setCursor(17, 3);
          lcd.print(Str_num);
          key = NO_KEY;
          while(key != '#' && key != '*' && key != 'D'){
            key = keypad.getKey();
          }
          switch(key){
            case '#':
            while (key == '#'){
              lcd.clear();
              lcd.print("Processing...");
              motor(deg_val);
              servo(time_val, num_val);
              delay(1000);
              lcd.clear();
              lcd.print("Mission Complete");
              lcd.setCursor(0, 1);
              lcd.print("[#]REPEAT");
              lcd.setCursor(0, 2);
              lcd.print("[D]BACK");
              key = NO_KEY;
              while (key != '#' && key != 'D'){
                key = keypad.getKey();
              }
            }
            if (key == 'D'){
              key = 'B'; //return to saved procedure
              level = 0;
            }
            break;

            case '*': case 'D':
            key = 'B'; //return to saved procedure
            level =0;
            break;
          }
        }
        break;

        case 'B':
        if(EEPROM.read(addrB) != 1){
          lcd.clear();
          lcd.print("ERROR!");
          lcd.setCursor(0, 1);
          lcd.print("No data found!");
          delay(2000);
          key = 'B';
          level = 0;
        }
        else{
          EEPROM.get(addrB + 1, deg_val);
          EEPROM.get(addrB + 5, time_val);
          EEPROM.get(addrB + 9, num_val);
          rotate_direction = EEPROM.read(addrB + 13);
          if (rotate_direction == true){
            digitalWrite(dir, HIGH);
          }
          else{
            digitalWrite(dir, LOW);
          }
          char Str_deg[20];
          char Str_time[20];
          char Str_num[20];
          itoa((int)deg_val, Str_deg, 10);
          dtostrf((float)time_val, 1, 2, Str_time);
          itoa((int)num_val, Str_num, 10);
          lcd.clear();
          lcd.print("Run below");
          lcd.setCursor(0, 1);
          lcd.print("Degree of Turn:");
          lcd.setCursor(15, 1);
          lcd.print(Str_deg);
          lcd.setCursor(0, 2);
          lcd.print("Spary time:");
          lcd.setCursor(11, 2);
          lcd.print(Str_time);
          lcd.setCursor(0, 3);
          lcd.print("Number of sprays:");
          lcd.setCursor(17, 3);
          lcd.print(Str_num);
          key = NO_KEY;
          while(key != '#' && key != '*' && key != 'D'){
            key = keypad.getKey();
          }
          switch(key){
            case '#':
            while (key == '#'){
              lcd.clear();
              lcd.print("Processing...");
              motor(deg_val);
              servo(time_val, num_val);
              delay(1000);
              lcd.clear();
              lcd.print("Mission Complete");
              lcd.setCursor(0, 1);
              lcd.print("[#]REPEAT");
              lcd.setCursor(0, 2);
              lcd.print("[D]BACK");
              key = NO_KEY;
              while (key != '#' && key != 'D'){
                key = keypad.getKey();
              }
            }
            if (key == 'D'){
              key = 'B'; //return to save procedure
              level = 0;
            }
            break;

            case '*': case 'D':
            key = 'B'; //return to saved procedure
            level = 0;
            break;
          }
        }
        break;

        case 'C':
        if(EEPROM.read(addrC) != 1){
          lcd.clear();
          lcd.print("ERROR!");
          lcd.setCursor(0, 1);
          lcd.print("No data found!");
          delay(2000);
          key = 'B'; //return to saved procedure
          level = 0;
        }
        else{
          EEPROM.get(addrC + 1, deg_val);
          EEPROM.get(addrC + 5, time_val);
          EEPROM.get(addrC + 9, num_val);
          rotate_direction = EEPROM.read(addr + 13);
          if (rotate_direction == true){
            digitalWrite(dir, HIGH);
          }
          else{
            digitalWrite(dir, LOW);
          }
          char Str_deg[20];
          char Str_time[20];
          char Str_num[20];
          itoa((int)deg_val, Str_deg, 10);
          dtostrf((float)time_val, 1, 2, Str_time);
          itoa((int)num_val, Str_num, 10);
          lcd.clear();
          lcd.print("Run below?");
          lcd.setCursor(0, 1);
          lcd.print("Degree of Turn:");
          lcd.setCursor(15, 1);
          lcd.print(Str_deg);
          lcd.setCursor(0, 2);
          lcd.print("Spary time:");
          lcd.setCursor(11, 2);
          lcd.print(Str_time);
          lcd.setCursor(0, 3);
          lcd.print("Number of sprays:");
          lcd.setCursor(17, 3);
          lcd.print(Str_num);
          key = NO_KEY;
          while(key != '#' && key != '*' && key != 'D'){
            key = keypad.getKey();
          }
          switch(key){
            case '#':
            while (key == '#'){
              lcd.clear();
              lcd.print("Processing...");
              motor(deg_val);
              servo(time_val, num_val);
              delay(1000);
              lcd.clear();
              lcd.print("Mission Complete");
              lcd.setCursor(0, 1);
              lcd.print("[#]REPEAT");
              lcd.setCursor(0, 2);
              lcd.print("[D]BACK");
              key = NO_KEY;
              while (key != '#' && key != 'D'){
                key = keypad.getKey();
              }
            }
            if (key == 'D'){ //return to saved procedure
              key = 'B';
              level = 0;
            }
            break;

            case '*': case 'D':
            key = 'B'; //return to main page
            level = 0;
            break;
          }
        }
        break;

        case 'D':
        lcd.setCursor(0, 0);
        lcd.print("                    ");
        lcd.setCursor(0, 0);
        lcd.print("Delete Procedure");
        key = NO_KEY;
        while(key != 'A' && key != 'B' && key != 'C' && key != '*' && key != 'D'){
        key = keypad.getKey();
        }
        switch(key){
          case 'A':
          lcd.clear();
          lcd.print("Delete setting [A]?");
          lcd.setCursor(0, 1);
          lcd.print("[#]Delete");
          lcd.setCursor(0, 2);
          lcd.print("[*]Cancel");
          while(key != '#' && key != '*'){
            key = keypad.getKey();
          }
          switch(key){
            case '#':
            EEPROM.write(addrA, 0);
            lcd.clear();
            lcd.print("Setting [A] Cleared");
            delay(2000);
            key = 'B';
            level = 0;
            break;

            case '*':
            key = 'B';
            level = 0;
            break;
          }
          break;
              
          case 'B':
          lcd.clear();
          lcd.print("Delete setting [B]?");
          lcd.setCursor(0, 1);
          lcd.print("[#]Delete");
          lcd.setCursor(0, 2);
          lcd.print("[*]Cancel");
          while(key != '#' && key != '*'){
            key = keypad.getKey();
          }
          switch(key){
            case '#':
            EEPROM.write(addrB, 0);
            lcd.clear();
            lcd.print("Setting [B] Cleared");
            delay(2000);
            key = 'B';
            level = 0;
            break;

            case '*':
            key = 'B';
            level = 0;
            break;
          }
          break;

          case 'C':
          lcd.clear();
          lcd.print("Delete setting [C]?");
          lcd.setCursor(0, 1);
          lcd.print("[#]Delete");
          lcd.setCursor(0, 2);
          lcd.print("[*]Cancel");
          while(key != '#' && key != '*'){
            key = keypad.getKey();
          }
          switch(key){
            case '#':
            EEPROM.write(addrC, 0);
            lcd.clear();
            lcd.print("Setting [C] Cleared");
            delay(2000);
            key = 'B';
            level = 0;
            break;

            case '*':
            key = 'B';
            level = 0;
            break;
          }
          break;

          case '*': case 'D':
          key = 'B';
          level = 0;
          break;
        }
        break;

        case '*':
        key = NO_KEY; //return to main page
        level =0;
        break;
      }
      break;

      case 'C':
      lcd.clear();
      lcd.print("Sample Temperature");
      lcd.setCursor(0, 2);
      lcd.print("198.2 Degrees");
      delay(1000);
      lcd.setCursor(0, 2);
      lcd.print("205.6 Degrees");
      delay(1000);
      lcd.setCursor(0, 2);
      lcd.print("208.4 Degrees");
      delay(1000);
      lcd.setCursor(0, 2);
      lcd.print("210.1 Degrees");
      delay(1000);
      key = NO_KEY;
      while(key == NO_KEY){
        key = keypad.getKey();
        if(key == 'D'){
          break;
        }
      }
      break;

      default:
      //back to main page
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Main Page");
      lcd.setCursor(0, 1);
      lcd.print("[A]New Procedure");
      lcd.setCursor(0, 2);
      lcd.print("[B]Saved Procedure");
      lcd.setCursor(0, 3);
      lcd.print("[C]SampleTemperature");
      key = NO_KEY;
      while(key == NO_KEY){
        key = keypad.getKey();
      }
      break;
      }
    }
  }
  else if (level == 1){
    switch(key){
      case 'A':
      // Spray Time
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Spray Time");
      lcd.setCursor(0, 2);
      lcd.print("INPUT SECOND:");
      lcd.setCursor(0, 3);
      lcd.print(" ");
      lcd.setCursor(1, 3);
      print_number(time_val);
      time_val = GetNumber(time_val, 0, time_first, true);
      break;

      case 'B':
      // Number of Sprays per
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Number of Sprays per");
      lcd.setCursor(0, 1);
      lcd.print("index");
      lcd.setCursor(0, 2);
      lcd.print("INPUT VALUE:");
      lcd.setCursor(0, 3);
      lcd.print(" ");
      lcd.setCursor(1, 3);
      print_number(num_val, true);
      num_val = GetNumber(num_val, 0, num_first, false);
      break;

      case 'C':
      // Degrees of turn
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Degrees of turn per");
      lcd.setCursor(0, 1);
      lcd.print("index");
      lcd.setCursor(0, 2);
      lcd.print("INPUT DEGREE(0-360):");
      lcd.setCursor(0, 3);
      lcd.print(" ");
      lcd.setCursor(1, 3);
      print_number(deg_val, true);
      deg_val = GetNumber(deg_val, 360, deg_first, true, false);
      if(level == 0 || deg_val == 0){
        break;
      }
      else if(deg_val > 360){
        deg_val = temp;
        break;
      }
      else{
        lcd.clear();
        lcd.setCursor( 0, 0);
        lcd.print("Direction");
        lcd.setCursor(0, 1);
        lcd.print("[A]Clockwise");
        lcd.setCursor(0, 2);
        lcd.print("[B]Anticlockwise");
        key = NO_KEY;
        while(key != 'A' && key != 'B'){
          key = keypad.getKey();
        }
        switch(key){
          case 'A':
          lcd.clear();
          lcd.print("Clockwise...");
          rotate_direction = true;
          digitalWrite(dir, HIGH);
          //return to new procedure
          key = 'A';
          level = 0;
          delay(1000);
          break;

          case 'B':
          lcd.clear();
          lcd.print("Anticlockwise...");
          rotate_direction = false;
          digitalWrite(dir, LOW);
          //return to new procedure
          key = 'A';
          level = 0;
          delay(1000);
          break;
        }
        break;
       }
    }
  }
}

void blink(){ //Emergency Stop
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("EMERGENCY STOP");
  lcd.setCursor(0, 1);
  lcd.print("Press [#] to restart");
  key = NO_KEY;
  while(key != '#'){
    key = keypad.getKey();
  }
  resetFunc();
}
