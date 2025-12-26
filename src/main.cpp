#include <Arduino.h>
#include <Keyboard.h>
#include <OneButton.h>

/*
To Do:

- Build Switch for Modes in main loop
  >Blink LED using cadance from POTS tone ocde (on and off time)
- Add CAlubration Mode to Serial print all ADC Values with titles
- Wire rest to board
- Enable / Diable all keyboard outlut. Move all keyboard controls to seperate function and call, this will
  allow use of enable / diable to globaly set state for all keyboard commands
- Work on CB interface.....
- Configure Encoder
  > Push and Hold to cycle thru diffrent train modes (DE2, DM3, DH4, DE6) blink LED to indicate new mode
  > 

Test:

- Test new Push and Hold Code for Switch B (Starter)
- Test CB Delay to correct out of sync

*/

#define DEBUG

#ifdef DEBUG
  #define DEBUG_PRINT(x)    Serial.print (x)
  #define DEBUG_PRINTLN(x)  Serial.println (x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x) 
#endif

//=====================================
//Definr GPIO Pins

#define pin_LED 4 //LED Pin
// #define pin_switchA 8 //multi button input A
// #define pin_switchB 9 //multi button input A

#define throttle_pin A3   //Potetiometer Pin - Throttle
#define reverser_pin A2   //Potetiometer Pin - Reverser
#define indBrake_pin A1   //Potetiometer Pin - Ind Brake
#define trnBrake_pin A0 //Potetiometer Pin - Train Brake

#define pin_btnArrayA A9 //multi button input A (HL Up, HL Dn, Cab Up, Cab Dn)
#define pin_btnArrayB A8 //multi button input B (Start, Off, Horn)

#define pin_CB1 16 //CB 1 Input
#define pin_CB2 10 //CB 2 Input
#define pin_CB3 7  //CB 3 Inpu9t

#define pin_EncoderBtn 6
#define pin_EncoderDt 14
#define pin_EncoderClk 15

//=====================================
//Encoder Setup

  // If you define ENCODER_DO_NOT_USE_INTERRUPTS *before* including
  // Encoder, the library will never use interrupts.  This is mainly
  // useful to reduce the size of the library when you are using it
  // with pins that do not support interrupts.  Without interrupts,
  // your program must call the read() function rapidly, or risk
  // missing changes in position.
#define ENCODER_DO_NOT_USE_INTERRUPTS
#include <Encoder.h>

Encoder myEnc(pin_EncoderClk, pin_EncoderDt); //CLK, DT

// The 2. parameter activeLOW is true, because external wiring sets the button to LOW when pressed.
OneButton EncoderBtn(pin_EncoderBtn, true);  //Setup Encoder push button
// In case the momentary button puts the input to HIGH when pressed:
// The 2. parameter activeLOW is false when the external wiring sets the button to HIGH when pressed.
// The 3. parameter can be used to disable the PullUp .
// OneButton button(PIN_INPUT, false, false);

//=====================================
//Lever Variables

enum LeverPosition_Reverser {REV, N, FWD};
LeverPosition_Reverser reverser_currentPOS;

int throttle_currentPOS = 0;
int indBrake_currentPOS = 0;
int trnBrake_currentPOS = 0;

int throttle_previousReading = 0; //soter last read infoemarion
int indBrake_previousReading = 0; //soter last read infoemarion
int trnBrake_previousReading = 0; //soter last read infoemarion

//=====================================
//Button Variables
const int buttonSpacing = 200;    // ms time to wait for next key press on levers
const int keypressPeriod = 1000;  // ms to hold down the keypress before release when using Keyboard.press()
const int threshold = 50;         // Threshold for button press to trigger to remove variation in input
const int debounceDelay = 50;     // Debounce delay in milliseconds

// const int btnArrayA_Pin = A0; // Single analog pin for buttons
const int btnArrayA_numButtons = 4;
const int btnArrayA_Values[btnArrayA_numButtons] = {100, 300, 500, 700}; // Expected analog values for each button

// const int btnArrayB_Pin = A0; // Single analog pin for buttons
const int btnArrayB_numButtons = 4;
const int btnArrayB_Values[btnArrayB_numButtons] = {100, 300, 500, 700}; // Expected analog values for each button

int btnArrayA_State[btnArrayA_numButtons];
int btnArrayA_lastButtonState[btnArrayA_numButtons];
unsigned long btnArrayA_lastDebounceTime[btnArrayA_numButtons];

int btnArrayB_State[btnArrayB_numButtons];
int btnArrayB_lastButtonState[btnArrayB_numButtons];
unsigned long btnArrayB_lastDebounceTime[btnArrayB_numButtons];

bool btnArrayB_flag = false;
int btnArrayB_lastReadingState = 0;
unsigned long btnArrayB_lastReadingDebounceTime;

OneButton SW_CB1(pin_CB1, true); //CB1 button
OneButton SW_CB2(pin_CB2, true); //CB2 button
OneButton SW_CB3(pin_CB3, true); //CB3 button

//=====================================
//Keyboard Variables

bool keypressed = 0;
unsigned long keypressedPm = 0;  //previous mills for timer

  /*
  Button A Connections
  0 = BLUE Toggle DN
  1 = BLUE Toggle UP
  2 = RED Toggle DN
  3 = RED Toggle UP

  Button B Connections
  0 = SPARE
  1 = Horn
  2 = Start
  3 = Stop  

  CB CIrcuits
  CB1 = Master          
  CB2 = Starter
  CB3 = Traction Motor

  EY_NUM_LOCK	    0xDB	219
  KEY_KP_SLASH	  0xDC	220
  KEY_KP_ASTERISK	0xDD	221
  KEY_KP_MINUS	  0xDE	222
  KEY_KP_PLUS	    0xDF	223
  KEY_KP_ENTER	  0xE0	224
  KEY_KP_1	  0xE1	225
  KEY_KP_2	  0xE2	226
  KEY_KP_3	  0xE3	227
  KEY_KP_4	  0xE4	228
  KEY_KP_5	  0xE5	229
  KEY_KP_6	  0xE6	230
  KEY_KP_7	  0xE7	231
  KEY_KP_8	  0xE8	232
  KEY_KP_9	  0xE9	233
  KEY_KP_0	  0xEA	234
  KEY_KP_DOT	0xEB	235

  */

//keybindings
int keybinding_btnA[] = {0xE1, 0xE4, 0xE3, 0xE6}; // defione keybord bindings
int keybinding_btnB[] = {'x', 0xDF, 0xEA, 0xEB}; // defione keybord bindings

char keybinding_Throttle[] = {'t', 'g'}; // (UP , DN)
char keybinding_Reverser[] = {'t', 'g'}; // (UP , DN)
char keybinding_IndBrake[] = {'t', 'g'}; // (UP , DN)
char keybinding_TrnBrake[] = {'t', 'g'}; // (UP , DN)

// const char keybinding_CB[] = {'KEY_KP_7', 0xE8, 'KEY_KP_9'}; // (CB1, CB2, CB3)

int keybinding_CB[] = {0xE7, 0xE8, 0xE9}; // (CB1, CB2, CB3)

//Global
enum OperationMode {DISABLE, MODE_1, MODE_2, MODE_3, MODE_4, };
OperationMode currentMode = DISABLE;

//=====================================
// put function declarations here:
void keypressHandler();
void sendKeypress(int key);
void sendKeypressChar(char key);

void reverser();
void throttle();
void indBrake();
void trnBrake();
void buttonHandler();

void encoderCheck();
void encoderPress();
void encoderHold();

void CB1_press();
void CB1_release();
void CB2_press();
void CB2_release();
void CB3_press();
void CB3_release();

//=============================================================================
//SETUP 
//=============================================================================

void setup() {
  Serial.begin(9600);   //USB Serial for PC Communication
  Serial1.begin(9600);  //HW Serial @ pins 0 and 1

  pinMode(throttle_pin, INPUT);
  pinMode(reverser_pin, INPUT);
  pinMode(indBrake_pin, INPUT);
  pinMode(trnBrake_pin, INPUT);

  pinMode(pin_LED, OUTPUT);

  pinMode(pin_btnArrayA, INPUT);
  pinMode(pin_btnArrayB, INPUT);  

  EncoderBtn.attachClick(encoderPress);
  EncoderBtn.attachLongPressStop(encoderHold);

  SW_CB1.setPressMs(1000);
  SW_CB2.setPressMs(1000);
  SW_CB3.setPressMs(1000);

  SW_CB1.attachLongPressStart(CB1_press);
  SW_CB2.attachLongPressStart(CB2_press);
  SW_CB3.attachLongPressStart(CB3_press);

  SW_CB1.attachLongPressStop(CB1_release);
  SW_CB2.attachLongPressStop(CB2_release);
  SW_CB3.attachLongPressStop(CB3_release);

  digitalWrite(pin_LED, HIGH);
  delay(100);
  digitalWrite(pin_LED, LOW);

  Keyboard.begin();

  Serial.println("Setup Complete");

}

//=============================================================================
//LOOP
//=============================================================================

void loop() {
  // put your main code here, to run repeatedly:
  EncoderBtn.tick();  
  SW_CB1.tick();
  SW_CB2.tick();
  SW_CB3.tick();

  throttle();
  indBrake();
  reverser();
  trnBrake();

  buttonHandler();
  encoderCheck(); //check for change in encoder value
  keypressHandler();  //checks and clears keypress after set delay

}

//=====================================
//MISC FUNCTIONS
//=====================================

// void modeSwitch() {
//   //store code for encoder mode seletcion, Train model + Disable

//   switch (currentMode)
//   {
//   case DISABLE:
//     DEBUG_PRINTLN("Input Bypassed");
//     break;
  
//   default:
//     break;
//   }

// }

void sendKeypress(int key) {

  unsigned long Cm = millis();
  
  if(!keypressed) {
    Serial.println("pressedBTN");
    Keyboard.press(key);    
    keypressedPm = Cm;
    keypressed = true; //set keypress as true
  }
}

void sendKeypressChar(char key) {

  unsigned long Cm = millis();
  
  if(!keypressed) {
    Serial.println("pressedBTN");
    Keyboard.press(key);    
    keypressedPm = Cm;
    keypressed = true; //set keypress as true
  }
}

void keypressHandler() {
  //check for active keybress and release all if flag triggerd
  unsigned long Cm = millis();

  if(keypressed) {
    if(Cm - keypressPeriod >= keypressedPm) {
    Serial.println("releasedBTN");
    Keyboard.releaseAll();    
    keypressed = false;
    }
  }
}

void encoderCheck() {
  static long position  = -999;

  long newPos = myEnc.read();
  if (newPos != position) {
    position = newPos;
    Serial.println(position);
  }
  // With any substantial delay added, Encoder can only track
  // very slow motion.  You may uncomment this line to see
  // how badly a delay affects your encoder.
  //delay(50);
}

void encoderPress() {
  DEBUG_PRINTLN("Encoder BTN Pressed");
}

void encoderHold() {
   DEBUG_PRINTLN("Encoder BTN Held");
}

void buttonHandler() {

//Switches A BLUE / RED
int readingA = analogRead(pin_btnArrayA);

  for (int i = 0; i < btnArrayA_numButtons; i++) {
    if (readingA - btnArrayA_Values[i] > threshold && readingA < (btnArrayA_Values[i] + buttonSpacing)) {
      if (millis() - btnArrayA_lastDebounceTime[i] > debounceDelay) {
        if (readingA > (btnArrayA_lastButtonState[i]+20) || readingA < (btnArrayA_lastButtonState[i]-20)) {
          Serial.print("Reading A:");
          Serial.println(readingA);
          btnArrayA_lastDebounceTime[i] = millis();
          btnArrayA_State[i] = HIGH;
          if (btnArrayA_State[i] == HIGH) {   // if current button is pressed process triger below

            // sendKeypress(keybinding_btnA[i]); // send key press for corrisponding buttion input
            Keyboard.write(keybinding_btnA[i]);
            if(i == 0) Keyboard.write(0xE2);
            if(i == 1) Keyboard.write(0xE5);

            Serial.print("SW A Button ");
            Serial.print(i);
            Serial.println(" pressed");
          }
        }
      }
    } else {
      btnArrayA_State[i] = LOW;
    }
    btnArrayA_lastButtonState[i] = readingA;
  }

  // //Switches B (Starter) - Timed Release
  // int readingB = analogRead(pin_btnArrayB);
  
  // for (int i = 0; i < btnArrayB_numButtons; i++) {
  //   if (readingB - btnArrayB_Values[i] > threshold && readingB < (btnArrayB_Values[i] + buttonSpacing)) {
  //     if (millis() - btnArrayB_lastDebounceTime[i] > debounceDelay) {
  //       if (readingB > (btnArrayB_lastButtonState[i]+50) || readingB < (btnArrayB_lastButtonState[i]-50)) {
  //         Serial.println(readingB);
  //         btnArrayB_lastDebounceTime[i] = millis();
  //         btnArrayB_State[i] = HIGH;
  //         if (btnArrayB_State[i] == HIGH) {   // if current button is pressed process triger below

  //           Serial.print("Button ");
  //           Serial.print(i);
  //           Serial.println(" pressed");

  //           sendKeypress(keybinding_btnB[i]); // send key press for corrisponding buttion input
            
  //         }
  //       }
  //     }
  //   } else {
  //     btnArrayB_State[i] = LOW;
  //   }
  //   btnArrayB_lastButtonState[i] = readingB;
  // }

// delay(50);

//Switches B (Starter) - Hold
int readingB = analogRead(pin_btnArrayB);

if (readingB > (btnArrayB_lastReadingState + threshold) || readingB < (btnArrayB_lastReadingState - threshold)) {
  Serial.print("Reading B:");
  DEBUG_PRINTLN(readingB);
    if ((millis() - btnArrayB_lastReadingDebounceTime) > debounceDelay) {
      if (readingB > btnArrayB_Values[0] && readingB < btnArrayB_Values[1]) {
        // btnArrayB_flag = true;
        // DEBUG_PRINTLN("SW B Button 0:");
        // Keyboard.press(keybinding_btnB[0]);
      } else if (readingB > btnArrayB_Values[1] && readingB < btnArrayB_Values[2]) {
        btnArrayB_flag = true;  
        DEBUG_PRINTLN("SW B Button 1:");
        Keyboard.press(keybinding_btnB[1]);
      } else if (readingB > btnArrayB_Values[2] && readingB < btnArrayB_Values[3]) {
        btnArrayB_flag = true;  
        DEBUG_PRINTLN("SW B Button 2:");
        Keyboard.press(keybinding_btnB[2]);
      } else if (readingB > btnArrayB_Values[3]) {
        btnArrayB_flag = true;  
        DEBUG_PRINTLN("SW B Button 3:");
        Keyboard.press(keybinding_btnB[3]);
      } else {       	
        if(btnArrayB_flag) {
          DEBUG_PRINTLN("SW B Button Release:");
          btnArrayB_flag = false; 
          Keyboard.releaseAll();         
        }

      }
      btnArrayB_lastReadingDebounceTime = millis();
    }
    btnArrayB_lastReadingState = readingB;
     
     if(btnArrayB_flag == true && readingB == 0) {
          DEBUG_PRINTLN("Button Release B");
          btnArrayB_flag = false; 
          Keyboard.releaseAll();         
        }
  }
}

// CB Keypresses
void CB1_press() {
  Keyboard.write(keybinding_CB[0]);
  Serial.print("CB1 Press");
}
void CB1_release() {
  Keyboard.write(keybinding_CB[0]);
  Serial.print("CB1 Release");
}
void CB2_press() {
  Keyboard.write(keybinding_CB[1]);
}
void CB2_release() {
  Keyboard.write(keybinding_CB[1]);
}
void CB3_press() {
  Keyboard.write(keybinding_CB[2]);
}
void CB3_release() {
  Keyboard.write(keybinding_CB[2]);
}

void reverser() {
  // static reverserPos_current
  static bool enteringNewState = false;
  static const int reverserDelay = 20;

  /*
  Positions:
    R = 396 [+500]
    N = 550 [+650] [-450]
    F = 710 [-610] 

    Study, About 200 wide for each position. Posibly use math to caculate base on START / STOP  
  */

  // threshold steps for eahc direction
  static const int R_up_Threshold = 500;
  static const int N_dn_Threshold = 450;
  static const int N_up_Threshold = 650;
  static const int F_dn_Threshold = 610;

  int currentReading = analogRead(reverser_pin);
  


  switch (reverser_currentPOS)
  {
  case REV:
    if(enteringNewState) {      
      delay(reverserDelay);
      Keyboard.press('h');
      delay(100);
      Keyboard.release('h');
      enteringNewState = false;
    }    
    if(currentReading > R_up_Threshold) {
      enteringNewState = true;
      // send keyboard command and debug code
      Keyboard.press('y');
      delay(100);
      Keyboard.release('y');
      DEBUG_PRINTLN("Moving Up: REV --> N");
      reverser_currentPOS = N;
    }
    break;
  case N:
    if(enteringNewState) {
      // add Keyboard and Debug Code here
      delay(reverserDelay);
      enteringNewState = false;
    } 
    //Moving Up   
    if(currentReading > N_up_Threshold) {
      enteringNewState = true;
      // send keyboard command and debug cod
      Keyboard.press('y');
      delay(100);
      Keyboard.release('y');
      DEBUG_PRINTLN("Moving Up: N --> FWD");
      reverser_currentPOS = FWD;
    }

    //Moving Dn   
    if(currentReading < N_dn_Threshold) {
      enteringNewState = true;
      // send keyboard command and debug cod
      Keyboard.press('h');
      delay(100);
      Keyboard.release('h');
      DEBUG_PRINTLN("Moving Dn: REV <-- N");
      reverser_currentPOS = REV;
    }
    break;
  case FWD:
    if(enteringNewState) {
      delay(reverserDelay);
      Keyboard.press('y');
      delay(100);
      Keyboard.release('y');
      enteringNewState = false;
    } 
    //Moving Dn   
    if(currentReading < F_dn_Threshold) {
      enteringNewState = true;
      // send keyboard command and debug cod
      Keyboard.press('h');
      delay(100);
      Keyboard.release('h');
      DEBUG_PRINTLN("Moving Dn: N <-- FWD");
      reverser_currentPOS = N;
    }
    break;
  
  default:
    break;
  }



  //   if(previousReading != currentReading) {
  //     DEBUG_PRINT("Reverser: ");
  //     DEBUG_PRINTLN(currentReading);   
  //   previousReading = currentReading;
  // }

}

void indBrake() {

  static bool enteringNewState_indBrake = false;

  static int ind_detentCt = 7;
  static int ind_offset = 5;

  static int ind_X_low  = 290;
  static int ind_X_Dead = 360;
  static int ind_X_high = 740;
  static int ind_Detent1  = ind_X_Dead - ind_X_low; //Deadspace for First detent (off)
  static int ind_Detent2  = ((ind_X_high - ind_X_low) - ind_Detent1) / ind_detentCt;

  static int ind_keypressDelay = 50; //delay in ms to wait before next keypress


  int currentReading = analogRead(indBrake_pin);

  // if(indBrake_previousReading != currentReading) {
  //   DEBUG_PRINT("IndBrake: ");
  //   DEBUG_PRINTLN(currentReading);
  //   indBrake_previousReading = currentReading;
  // }

switch (indBrake_currentPOS)  {
  
  case 0:
    if(enteringNewState_indBrake) {
      DEBUG_PRINTLN("indBrake: 0");
      delay(ind_keypressDelay);
      Keyboard.press('k');
      delay(100);
      Keyboard.release('k'); 
      delay(ind_keypressDelay);
      Keyboard.press('k');
      delay(100);
      Keyboard.release('k');  

      enteringNewState_indBrake = false;
    }  
    //Move Up  
    if(currentReading > (ind_X_low + ind_Detent1 + ((ind_Detent2*indBrake_currentPOS)+ind_offset))) {
      enteringNewState_indBrake = true;
      indBrake_currentPOS = 1;
      // send keyboard command and debug code
      Keyboard.press('i');
      delay(100);
      Keyboard.release('i');            
    }
    break;
  
  case 1:
    if(enteringNewState_indBrake) {
      DEBUG_PRINTLN("indBrake: 1");
      enteringNewState_indBrake = false;
      delay(ind_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (ind_X_low + ind_Detent1 + (((ind_Detent2*indBrake_currentPOS)-ind_Detent2)-ind_offset))) {      
      enteringNewState_indBrake = true;
      indBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('k');
      delay(100);
      Keyboard.release('k');            
    }    
    //Move Up  
    if(currentReading > (ind_X_low + ind_Detent1 + ((ind_Detent2*indBrake_currentPOS)+ind_offset))) {      
      enteringNewState_indBrake = true;
      indBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('i');
      delay(100);
      Keyboard.release('i');      

    }        
  break;

  case 2:
    if(enteringNewState_indBrake) {
      DEBUG_PRINTLN("indBrake: 2");
      enteringNewState_indBrake = false;
      delay(ind_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (ind_X_low + ind_Detent1 + (((ind_Detent2*indBrake_currentPOS)-ind_Detent2)-ind_offset))) {      
      enteringNewState_indBrake = true;
      indBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('k');
      delay(100);
      Keyboard.release('k');            
    }    
    //Move Up  
    if(currentReading > (ind_X_low + ind_Detent1 + ((ind_Detent2*indBrake_currentPOS)+ind_offset))) {      
      enteringNewState_indBrake = true;
      indBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('i');
      delay(100);
      Keyboard.release('i');      

    }        
  break;

  case 3:
    if(enteringNewState_indBrake) {
      DEBUG_PRINTLN("indBrake: 3");
      enteringNewState_indBrake = false;
      delay(ind_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (ind_X_low + ind_Detent1 + (((ind_Detent2*indBrake_currentPOS)-ind_Detent2)-ind_offset))) {      
      enteringNewState_indBrake = true;
      indBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('k');
      delay(100);
      Keyboard.release('k');            
    }    
    //Move Up  
    if(currentReading > (ind_X_low + ind_Detent1 + ((ind_Detent2*indBrake_currentPOS)+ind_offset))) {      
      enteringNewState_indBrake = true;
      indBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('i');
      delay(100);
      Keyboard.release('i');      

    }        
  break;

  case 4:
    if(enteringNewState_indBrake) {
      DEBUG_PRINTLN("indBrake: 4");
      enteringNewState_indBrake = false;
      delay(ind_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (ind_X_low + ind_Detent1 + (((ind_Detent2*indBrake_currentPOS)-ind_Detent2)-ind_offset))) {      
      enteringNewState_indBrake = true;
      indBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('k');
      delay(100);
      Keyboard.release('k');            
    }    
    //Move Up  
    if(currentReading > (ind_X_low + ind_Detent1 + ((ind_Detent2*indBrake_currentPOS)+ind_offset))) {      
      enteringNewState_indBrake = true;
      indBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('i');
      delay(100);
      Keyboard.release('i');      

    }        
  break;

  case 5:
    if(enteringNewState_indBrake) {
      DEBUG_PRINTLN("indBrake: 5");
      enteringNewState_indBrake = false;
      delay(ind_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (ind_X_low + ind_Detent1 + (((ind_Detent2*indBrake_currentPOS)-ind_Detent2)-ind_offset))) {      
      enteringNewState_indBrake = true;
      indBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('k');
      delay(100);
      Keyboard.release('k');            
    }    
    //Move Up  
    if(currentReading > (ind_X_low + ind_Detent1 + ((ind_Detent2*indBrake_currentPOS)+ind_offset))) {      
      enteringNewState_indBrake = true;
      indBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('i');
      delay(100);
      Keyboard.release('i');      

    }        
  break;

  case 6:
    if(enteringNewState_indBrake) {
      DEBUG_PRINTLN("indBrake: 6");
      enteringNewState_indBrake = false;
      delay(ind_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (ind_X_low + ind_Detent1 + (((ind_Detent2*indBrake_currentPOS)-ind_Detent2)-ind_offset))) {      
      enteringNewState_indBrake = true;
      indBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('k');
      delay(100);
      Keyboard.release('k');            
    }    
    //Move Up  
    if(currentReading > (ind_X_low + ind_Detent1 + ((ind_Detent2*indBrake_currentPOS)+ind_offset))) {      
      enteringNewState_indBrake = true;
      indBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('i');
      delay(100);
      Keyboard.release('i');      

    }        
  break;

  case 7:
    if(enteringNewState_indBrake) {
      DEBUG_PRINTLN("indBrake: 7");
      enteringNewState_indBrake = false;
      delay(ind_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (ind_X_low + ind_Detent1 + (((ind_Detent2*indBrake_currentPOS)-ind_Detent2)-ind_offset))) {      
      enteringNewState_indBrake = true;
      indBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('k');
      delay(100);
      Keyboard.release('k');            
    }            
  break;
  
  default:
  break;
  }
}

void throttle() {
  static bool enteringNewState_throttle = false;

  static int detentCt = 11;
  static int offset = 3;

  static int X_low  = 360;
  static int X_Dead = 400;
  static int X_high = 600;
  static int Detent1  = X_Dead - X_low; //Deadspace for First detent (off)
  static int Detent2  = ((X_high - X_low) - Detent1) / detentCt;

  static int keypressDelay = 20; //delay in ms to wait before next keypress

  // Use to caculate each step based on wht switch state. USe swotch sate for current POS
  // 
  // Up Shift = X_low + Detent1 + ((Detent2*throttle_currentPOS)-Detent2);

  int currentReading = analogRead(throttle_pin);

  // if(throttle_previousReading != currentReading) {
  //   DEBUG_PRINT("Throttle: ");
  //   DEBUG_PRINTLN(currentReading);
  //   throttle_previousReading = currentReading;
  // }

  // add delay after keypress to prevent quick stacking on inputs to PC

  switch (throttle_currentPOS)
  {
  
  case 0:
    if(enteringNewState_throttle) {
      DEBUG_PRINTLN("Throttle: 0");

      Keyboard.press('g');
      delay(100);
      Keyboard.release('g'); 
      delay(20);
      Keyboard.press('g');
      delay(100);
      Keyboard.release('g');  

      enteringNewState_throttle = false;
    }  
    //Move Up  
    if(currentReading > (X_low + Detent1 + ((Detent2*throttle_currentPOS)+offset))) {
      enteringNewState_throttle = true;
      throttle_currentPOS = 1;
      // send keyboard command and debug code
      Keyboard.press('t');
      delay(100);
      Keyboard.release('t');            
    }
    break;
  
  case 1:
    if(enteringNewState_throttle) {
      DEBUG_PRINTLN("Throttle: 1");
      enteringNewState_throttle = false;
      delay(keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (X_low + Detent1 + (((Detent2*throttle_currentPOS)-Detent2)-offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS = 0;
      // send keyboard command and debug code
      Keyboard.press('g');
      delay(100);
      Keyboard.release('g');            
    }    
    //Move Up  
    if(currentReading > (X_low + Detent1 + ((Detent2*throttle_currentPOS)+offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS = 2;
      // send keyboard command and debug code
      Keyboard.press('t');
      delay(100);
      Keyboard.release('t');      

    }        
  break;
  
  case 2:
    if(enteringNewState_throttle) {
      DEBUG_PRINTLN("Throttle: 2");
      enteringNewState_throttle = false;
      delay(keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (X_low + Detent1 + (((Detent2*throttle_currentPOS)-Detent2)-offset))) {     
      enteringNewState_throttle = true;
      throttle_currentPOS = 1;
      // send keyboard command and debug code
      Keyboard.press('g');
      delay(100);
      Keyboard.release('g');            
    }    
    //Move Up  
    if(currentReading > (X_low + Detent1 + ((Detent2*throttle_currentPOS)+offset))) {     
      enteringNewState_throttle = true;
      throttle_currentPOS = 3;
      // send keyboard command and debug code
      Keyboard.press('t');
      delay(100);
      Keyboard.release('t');            
    }    
  break;
    
  case 3:
    if(enteringNewState_throttle) {
      DEBUG_PRINTLN("Throttle: 3");
      enteringNewState_throttle = false;
      delay(keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (X_low + Detent1 + (((Detent2*throttle_currentPOS)-Detent2)-offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS--; //increment throttle position;
      // send keyboard command and debug code
      Keyboard.press('g');
      delay(100);
      Keyboard.release('g');            
    }    
    // Move Up  
    if(currentReading > (X_low + Detent1 + ((Detent2*throttle_currentPOS)+offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS++; //increment throttle position
      // send keyboard command and debug code
      Keyboard.press('t');
      delay(100);
      Keyboard.release('t');            
    }    
  break;
  case 4:
    if(enteringNewState_throttle) {
      DEBUG_PRINTLN("Throttle: 4");
      enteringNewState_throttle = false;
      delay(keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (X_low + Detent1 + (((Detent2*throttle_currentPOS)-Detent2)-offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS--; //increment throttle position;
      // send keyboard command and debug code
      Keyboard.press('g');
      delay(100);
      Keyboard.release('g');            
    }    
    // Move Up  
    if(currentReading > (X_low + Detent1 + ((Detent2*throttle_currentPOS)+offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS++; //increment throttle position
      // send keyboard command and debug code
      Keyboard.press('t');
      delay(100);
      Keyboard.release('t');            
    }    
  break;
  case 5:
    if(enteringNewState_throttle) {
      DEBUG_PRINTLN("Throttle: 5");
      enteringNewState_throttle = false;
      delay(keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (X_low + Detent1 + (((Detent2*throttle_currentPOS)-Detent2)-offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS--; //increment throttle position;
      // send keyboard command and debug code
      Keyboard.press('g');
      delay(100);
      Keyboard.release('g');            
    }    
    // Move Up  
    if(currentReading > (X_low + Detent1 + ((Detent2*throttle_currentPOS)+offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS++; //increment throttle position
      // send keyboard command and debug code
      Keyboard.press('t');
      delay(100);
      Keyboard.release('t');            
    }    
  break;
  case 6:
    if(enteringNewState_throttle) {
      DEBUG_PRINTLN("Throttle: 6");
      enteringNewState_throttle = false;
      delay(keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (X_low + Detent1 + (((Detent2*throttle_currentPOS)-Detent2)-offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS--; //increment throttle position;
      // send keyboard command and debug code
      Keyboard.press('g');
      delay(100);
      Keyboard.release('g');            
    }    
    // Move Up  
    if(currentReading > (X_low + Detent1 + ((Detent2*throttle_currentPOS)+offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS++; //increment throttle position
      // send keyboard command and debug code
      Keyboard.press('t');
      delay(100);
      Keyboard.release('t');            
    }    
  break;
  case 7:
    if(enteringNewState_throttle) {
      DEBUG_PRINTLN("Throttle: 7");
      enteringNewState_throttle = false;
      delay(keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (X_low + Detent1 + (((Detent2*throttle_currentPOS)-Detent2)-offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS--; //increment throttle position;
      // send keyboard command and debug code
      Keyboard.press('g');
      delay(100);
      Keyboard.release('g');            
    }    
    // Move Up  
    if(currentReading > (X_low + Detent1 + ((Detent2*throttle_currentPOS)+offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS++; //increment throttle position
      // send keyboard command and debug code
      Keyboard.press('t');
      delay(100);
      Keyboard.release('t');            
    }    
  break;
  case 8:
    if(enteringNewState_throttle) {
      DEBUG_PRINTLN("Throttle: 8");
      enteringNewState_throttle = false;
      delay(keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (X_low + Detent1 + (((Detent2*throttle_currentPOS)-Detent2)-offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS--; //increment throttle position;
      // send keyboard command and debug code
      Keyboard.press('g');
      delay(100);
      Keyboard.release('g');            
    }    
    // Move Up  
    if(currentReading > (X_low + Detent1 + ((Detent2*throttle_currentPOS)+offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS++; //increment throttle position
      // send keyboard command and debug code
      Keyboard.press('t');
      delay(100);
      Keyboard.release('t');            
    }    
  break;
  case 9:
    if(enteringNewState_throttle) {
      DEBUG_PRINTLN("Throttle: 9");
      enteringNewState_throttle = false;
      delay(keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (X_low + Detent1 + (((Detent2*throttle_currentPOS)-Detent2)-offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS--; //increment throttle position;
      // send keyboard command and debug code
      Keyboard.press('g');
      delay(100);
      Keyboard.release('g');            
    }    
    // Move Up  
    if(currentReading > (X_low + Detent1 + ((Detent2*throttle_currentPOS)+offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS++; //increment throttle position
      // send keyboard command and debug code
      Keyboard.press('t');
      delay(100);
      Keyboard.release('t');            
    }    
  break;
  case 10:
    if(enteringNewState_throttle) {
      DEBUG_PRINTLN("Throttle:10");
      enteringNewState_throttle = false;
      delay(keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (X_low + Detent1 + (((Detent2*throttle_currentPOS)-Detent2)-offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS--; //increment throttle position;
      // send keyboard command and debug code
      Keyboard.press('g');
      delay(100);
      Keyboard.release('g');            
    }    
    // Move Up  
    if(currentReading > (X_low + Detent1 + ((Detent2*throttle_currentPOS)+offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS++; //increment throttle position
      // send keyboard command and debug code
      Keyboard.press('t');
      delay(100);
      Keyboard.release('t');            
    }    
  break;
  case 11:
    if(enteringNewState_throttle) {
      DEBUG_PRINTLN("Throttle:11");
      enteringNewState_throttle = false;
      delay(keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (X_low + Detent1 + (((Detent2*throttle_currentPOS)-Detent2)-offset))) {      
      enteringNewState_throttle = true;
      throttle_currentPOS--; //decrement throttle position;
      // send keyboard command and debug code
      Keyboard.press('g');
      delay(100);
      Keyboard.release('g');            
    }    
  break;

  default:
    DEBUG_PRINTLN("Out Of Range:");
    break;
  }

}

void trnBrake() {
  static bool enteringNewState_trnBrake = false;

  static int trn_detentCt = 11; //detent ct for control
  static int trn_offset = 3;    //offset for dead zone overlap between detents. must be less than Detent2

  static int trn_X_low  = 114;  //CALIBRATION 
  static int trn_X_Dead = 320;
  static int trn_X_high = 837;
  static int trn_Detent1  = trn_X_Dead - trn_X_low; //Deadspace for First detent (off)
  static int trn_Detent2  = ((trn_X_high - trn_X_low) - trn_Detent1) / trn_detentCt;
  static int trn_keypressDelay = 100; //delay in ms to wait before next keypress
  
  int currentReading = analogRead(trnBrake_pin);

  // if(trnBrake_previousReading != currentReading) {
  //   DEBUG_PRINT("TrnBrake: ");
  //   DEBUG_PRINTLN(currentReading)uuuuu;
  //   trnBrake_previousReading = currentReading;
  // }

switch (trnBrake_currentPOS)  {

  case 99:
    if(enteringNewState_trnBrake) {
      DEBUG_PRINTLN("TrnBrake: Release");
      delay(trn_keypressDelay);
      Keyboard.press('j');
      // delay(100);
      // Keyboard.release('k'); 
      // delay(trn_keypressDelay);
      // Keyboard.press('k');
      // delay(100);
      // Keyboard.release('k');  

      enteringNewState_trnBrake = false;
    }  
    //Move Up  
    if(currentReading > 175) {
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS = 0;
      // send keyboard command and debug code
      // Keyboard.press('u');
      // delay(100);
      Keyboard.release('j');            
    }
  break;
  
  case 0:
    if(enteringNewState_trnBrake) {
      DEBUG_PRINTLN("TrnBrake: 0");
      // delay(trn_keypressDelay);
      // Keyboard.press('j');
      // delay(100);
      // Keyboard.release('j'); 
      // delay(trn_keypressDelay);
      // Keyboard.press('j');
      // delay(100);
      // Keyboard.release('j');  

      enteringNewState_trnBrake = false;
    }  
    //Move Up  
    if(currentReading > (trn_X_low + trn_Detent1 + ((trn_Detent2*trnBrake_currentPOS)+trn_offset))) {
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS = 1;
      // send keyboard command and debug code
      Keyboard.press('u');
      delay(100);
      Keyboard.release('u');            
    }
    //Release
    if(currentReading < 165) {
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS = 99;
    }

    break;
  
  case 1:
    if(enteringNewState_trnBrake) {
      DEBUG_PRINTLN("TrnBrake: 1");
      enteringNewState_trnBrake = false;
      delay(trn_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (trn_X_low + trn_Detent1 + (((trn_Detent2*trnBrake_currentPOS)-trn_Detent2)-trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('j');
      delay(100);
      Keyboard.release('j');            
    }    
    //Move Up  
    if(currentReading > (trn_X_low + trn_Detent1 + ((trn_Detent2*trnBrake_currentPOS)+trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('u');
      delay(100);
      Keyboard.release('u');      

    }        
  break;

  case 2:
    if(enteringNewState_trnBrake) {
      DEBUG_PRINTLN("TrnBrake: 2");
      enteringNewState_trnBrake = false;
      delay(trn_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (trn_X_low + trn_Detent1 + (((trn_Detent2*trnBrake_currentPOS)-trn_Detent2)-trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('j');
      delay(100);
      Keyboard.release('j');            
    }    
    //Move Up  
    if(currentReading > (trn_X_low + trn_Detent1 + ((trn_Detent2*trnBrake_currentPOS)+trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('u');
      delay(100);
      Keyboard.release('u');      

    }        
  break;

  case 3:
    if(enteringNewState_trnBrake) {
      DEBUG_PRINTLN("TrnBrake: 3");
      enteringNewState_trnBrake = false;
      delay(trn_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (trn_X_low + trn_Detent1 + (((trn_Detent2*trnBrake_currentPOS)-trn_Detent2)-trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('j');
      delay(100);
      Keyboard.release('j');            
    }    
    //Move Up  
    if(currentReading > (trn_X_low + trn_Detent1 + ((trn_Detent2*trnBrake_currentPOS)+trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('u');
      delay(100);
      Keyboard.release('u');      

    }        
  break;

  case 4:
    if(enteringNewState_trnBrake) {
      DEBUG_PRINTLN("TrnBrake: 4");
      enteringNewState_trnBrake = false;
      delay(trn_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (trn_X_low + trn_Detent1 + (((trn_Detent2*trnBrake_currentPOS)-trn_Detent2)-trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('j');
      delay(100);
      Keyboard.release('j');            
    }    
    //Move Up  
    if(currentReading > (trn_X_low + trn_Detent1 + ((trn_Detent2*trnBrake_currentPOS)+trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('u');
      delay(100);
      Keyboard.release('u');      

    }        
  break;
  case 5:
    if(enteringNewState_trnBrake) {
      DEBUG_PRINTLN("TrnBrake: 5");
      enteringNewState_trnBrake = false;
      delay(trn_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (trn_X_low + trn_Detent1 + (((trn_Detent2*trnBrake_currentPOS)-trn_Detent2)-trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('j');
      delay(100);
      Keyboard.release('j');            
    }    
    //Move Up  
    if(currentReading > (trn_X_low + trn_Detent1 + ((trn_Detent2*trnBrake_currentPOS)+trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('u');
      delay(100);
      Keyboard.release('u');      

    }        
  break;

  case 6:
    if(enteringNewState_trnBrake) {
      DEBUG_PRINTLN("TrnBrake: 6");
      enteringNewState_trnBrake = false;
      delay(trn_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (trn_X_low + trn_Detent1 + (((trn_Detent2*trnBrake_currentPOS)-trn_Detent2)-trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('j');
      delay(100);
      Keyboard.release('j');            
    }    
    //Move Up  
    if(currentReading > (trn_X_low + trn_Detent1 + ((trn_Detent2*trnBrake_currentPOS)+trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('u');
      delay(100);
      Keyboard.release('u'); 
    }        
  break;

  case 7:
    if(enteringNewState_trnBrake) {
      DEBUG_PRINTLN("TrnBrake: 7");
      enteringNewState_trnBrake = false;
      delay(trn_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (trn_X_low + trn_Detent1 + (((trn_Detent2*trnBrake_currentPOS)-trn_Detent2)-trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('j');
      delay(100);
      Keyboard.release('j');            
    }  
        //Move Up  
    if(currentReading > (trn_X_low + trn_Detent1 + ((trn_Detent2*trnBrake_currentPOS)+trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('u');
      delay(100);
      Keyboard.release('u');
    }        
  break;

  case 8:
    if(enteringNewState_trnBrake) {
      DEBUG_PRINTLN("TrnBrake: 8");
      enteringNewState_trnBrake = false;
      delay(trn_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (trn_X_low + trn_Detent1 + (((trn_Detent2*trnBrake_currentPOS)-trn_Detent2)-trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('j');
      delay(100);
      Keyboard.release('j');            
    }  
        //Move Up  
    if(currentReading > (trn_X_low + trn_Detent1 + ((trn_Detent2*trnBrake_currentPOS)+trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('u');
      delay(100);
      Keyboard.release('u');
    }        
  break;

  case 9:
    if(enteringNewState_trnBrake) {
      DEBUG_PRINTLN("TrnBrake: 9");
      enteringNewState_trnBrake = false;
      delay(trn_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (trn_X_low + trn_Detent1 + (((trn_Detent2*trnBrake_currentPOS)-trn_Detent2)-trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('j');
      delay(100);
      Keyboard.release('j');            
    }  
        //Move Up  
    if(currentReading > (trn_X_low + trn_Detent1 + ((trn_Detent2*trnBrake_currentPOS)+trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('u');
      delay(100);
      Keyboard.release('u');
    }        
  break;

  case 10:
    if(enteringNewState_trnBrake) {
      DEBUG_PRINTLN("TrnBrake: 10");
      enteringNewState_trnBrake = false;
      delay(trn_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (trn_X_low + trn_Detent1 + (((trn_Detent2*trnBrake_currentPOS)-trn_Detent2)-trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('j');
      delay(100);
      Keyboard.release('j');            
    }  
        //Move Up  
    if(currentReading > (trn_X_low + trn_Detent1 + ((trn_Detent2*trnBrake_currentPOS)+trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS++;
      // send keyboard command and debug code
      Keyboard.press('u');
      delay(100);
      Keyboard.release('u');
    }        
  break;

  case 11:
    if(enteringNewState_trnBrake) {
      DEBUG_PRINTLN("TrnBrake: 11");
      enteringNewState_trnBrake = false;
      delay(trn_keypressDelay);
    } 
    //Move Dn  
    if(currentReading < (trn_X_low + trn_Detent1 + (((trn_Detent2*trnBrake_currentPOS)-trn_Detent2)-trn_offset))) {      
      enteringNewState_trnBrake = true;
      trnBrake_currentPOS--;
      // send keyboard command and debug code
      Keyboard.press('j');
      delay(100);
      Keyboard.release('j');            
    }      
  break;

  default:
  break;

}
}