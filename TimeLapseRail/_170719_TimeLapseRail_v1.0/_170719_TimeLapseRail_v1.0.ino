/*
 * Time Lapse Rail Software V1.0
 * for DIY Time Lapse Rail from Idea Propulsion Systems
 * written by David Hartkop
 * This release: July 19, 2017
 * 
 * Designed for use on an Arduino Uno connected to CNC V3 shield
 * Uses analog encoded 4-button keypad, and i2C 16x2LCD display
 * Drives a single Nema 17 bipolar stepper motor to move your camera periodically from IN to OUT point. 
 * Software lets you set total number of frames, interval for picture taking, and the In / Out positions.
 * 
 * This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send 
 * a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 * 
 * You are free:
 *- to Share - to copy, distribute and transmit the work, and
 *- to Remix - to adapt the work
 * 
 * This software uses the LiquidCrystal_I2C.h library created by Francisco Malpartia on 20 / 08 / 11, licensed under 
 * creative commons license 3.0: Attribution Share Alike CC BY-SA. 
 * 
 */



//include library for LCD *************************************

#include <LiquidCrystal_I2C.h>




//definitions for LCD *************************************

#define I2C_ADDR    0x3F // <<----- Add your address here.  Find it from I2C Scanner
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7




// pin assignments *************************************

LiquidCrystal_I2C  lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

const int analogInPin = A3; // analog io labeled CoolEn on CNC V3 shield, used to read analog encoded keypad. 
const int stepperXdirpin = 5; //digital output pin that determines direction of x axis stepper
const int stepperXstppin = 2; //digital output pin that signals x axis stepper to step
const int stepperEnablepin = 8; //digital output pin that enables all of the step driver cards of the CNC V3 shield.
const int XendstopPin = 9; // digital io labeled x- on CNC V3 shield, used as endstop for camera X-axis movement.
const int camPin = 12; //digital io, labeled SpnEn on CNC V3 shield, used to trigger shutter of DSLR camera.



// declare variables *************************************

int menuNumber = 0;
int optionNumber = 0;

long timelapseFrames = 5;
long framecount;

int cursPos = 0; //indexes position of cursor in cursor function
bool cursMode = 0; // 0 = movement mode, 1 = modify mode. Pressing the '4' button toggles between modes.

long interval_m = 1; 
long interval_s = 10;




long camPosition = 0;
long posCounter = 0;
long keyPositionIn = 356;
long keyPositionOut = 810;

unsigned long currentMillis;
unsigned long prevMillis;
unsigned long previousDispMillis = 0;

int countdownSec = 0;
int countdownMin = 0;



// define functions *******************************

/*
 * takePictureFunction(): Sends a short pulse out through camPin to trigger a DSLR camera.
 */
void takePictureFunction(){
   Serial.println("signals the DSLR camra to take a picture");
   digitalWrite(camPin, HIGH); //Takes first frame of timelapse with DSLR.
   delay(1000); //Button press for a full second to 'wake up' the camera from sleep. 
   digitalWrite(camPin,LOW);
   
}



/*
 * motorInFunction: Drives motor from zero position to the set IN point; 
 * use motorHomeFunction() first before you use this for best accuracy.
 */

void motorInFunction(){
  digitalWrite(stepperXdirpin, HIGH); //sets the direction toward IN position
  digitalWrite(stepperEnablepin, LOW);

    for(long i=0; i < keyPositionIn; i = i+1) { //moves camera to IN position
    
      digitalWrite(stepperXstppin, HIGH);
      delay(4);
      digitalWrite(stepperXstppin, LOW);
      delay(4);
    }

  digitalWrite(stepperEnablepin, HIGH); //disables stepper driver card
  camPosition = keyPositionIn;
}


/*
 * motorOutFunction: Drives motor from zero position to the set OUT point; 
 * use motorHomeFunction() first before you use this for best accuracy.
 */

 void motorOutFunction(){
  digitalWrite(stepperXdirpin, HIGH); //sets the direction toward OUT position
  digitalWrite(stepperEnablepin, LOW);
    for(long i=0; i < keyPositionOut; i = i+1) { //moves camera to OUT position
    
      digitalWrite(stepperXstppin, HIGH);
      delay(4);
      digitalWrite(stepperXstppin, LOW);
      delay(4);
}

digitalWrite(stepperEnablepin, HIGH); //disables stepper driver card
camPosition = keyPositionOut;
 }



/*
 * motorHomeFunction: moves motor toward position Zero until endstop button is pressed. Zeros the position variable.
 */

 void motorHomeFunction(){

  digitalWrite(stepperXdirpin, LOW);
  digitalWrite(stepperEnablepin, LOW);
    
    while (digitalRead(XendstopPin) == 1){
    
      digitalWrite(stepperXstppin, HIGH);
      delay(4);
      digitalWrite(stepperXstppin, LOW);
      delay(4);
      
    }
  
digitalWrite(stepperEnablepin, HIGH);
camPosition = 0;
 }




/*
 * ButtonReadFunction
 */
int buttonReadFunction(){
  int result;
  int sensorValue = analogRead(analogInPin);
  
  if (480 < sensorValue && sensorValue < 520)
     {result = 1;}
  
  else if (300 < sensorValue && sensorValue < 380)
     {result = 2;}
  
  else if (210 < sensorValue && sensorValue < 280)
     {result = 3;}
  
  else if(180 < sensorValue && sensorValue < 220)
    {result = 4;}
  
  else 
    {result = 0;}

return result;

}

/*
 * zeroPadFunction 
 * 
 * INPUTS:
 * num -> the number that needs to be zero padded
 * paddedlen -> the final length of the output in digits including the leading zeros
 * lcd_col -> the index 0-n of the column position in the LCD
 * lcd_row -> the index 0-n of the row of the LCD
 * 
 * OUTPUT: a zero padded number of set length placed where you want it!
 */

long zeroPadFunction(long num, int paddedlen, int lcd_col, int lcd_row) {

 
//first find length in digits of num, pass to var numlen
long i = num;
int numlen = 0;
for ( i; i>0; i = i/10){
numlen = numlen+1;
}


//next create leading zeros based on numlen and start position
for (int n=(paddedlen-numlen); n > 0; n = n-1) { //for loop adds leading zeros. Total length of number space minus numLen.
lcd.setCursor((lcd_col + n - 1),lcd_row); //sets cursor position from right to left with each inrement 
lcd.print(0);  //prints a zero 
}
//finally, print the number after the leading zeros.
lcd.setCursor((lcd_col-numlen+paddedlen),lcd_row); //sets curser so that number will start after leading zeros.
lcd.print(num); //prints the number.
  
}


/*
 * 
 * cursorFunction for TIMES menu
 * 
 * moves cursor to screen position of number digit, select digit, and change it with arrow keys.
 * How it works: Say you have three numbers, each four digits long. The numbers can be arranged anywhere on the LCD, any line even. 
 * This function addresses the digit positions in three numbers as being a single stretch of 12 cursor positions. The arrow keys will move the cursor
 * left or right across the screen, and the cursor will jump from one number to another as you go past the end of one. The arrays
 * included in the function must be set up to reflect things about how you chose to place your numbers on the display. 
 * NOTE: the numbes should be zero padded right justified for it all to work right! 
 * 
 * There's a mode for moving the cursor, and a mode for making a change to the digit the cursor has landed on. The '4' button toggles between the loops.
 * 
 * INPUTS: number of button pressed on keypad ( for the arrow keys 2 and 3 to L and R, and 4 to select. 
 * OUTPUTS: writes a " " onto the screen in appropriate spot. Selection and arrows allow numbers added or subtracted from number places.
 */

int cursorFunction(){

  
//establish arrays dealing with each digit across row of all digits being selectable with cursor
int cursor_col[] = {8, 9, 10, 11, 12, 13, 10, 11, 13, 14}; //each column space for a number or numbers. Can be on different spots of the screen.
int cursor_row[] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1}; //each row for a given number or numbers. Can be on different lines.
int cursor_num[] = {1, 1, 1, 1, 1, 1, 2, 2, 3, 3}; //index number for which number is being affected by selection & changes. 
long cursor_add[] = {100000, 10000, 1000, 100, 10, 1, 10, 1, 10, 1}; //value corresponding to what to add or subt from digit at given position.

if (cursMode == false)  //cursor MOVEMENT mode. Cursor position changes with 2 & 3 buttons.
{
    if (buttonReadFunction()==2 && cursPos > 0){ //be sure to set the end stop ranges for cursor movement.
    delay(100);
    cursPos = cursPos - 1;
    }

    else if (buttonReadFunction()==3 && cursPos < 9){ //be sure to set the end stop ranges for cursor movement.
    delay(100);
    cursPos = cursPos + 1;
    
    }

    else if (buttonReadFunction()==4){ // Sends cursor into MODIFY mode
    delay(100);
    cursMode = true;
    }


}


else if (cursMode == true)  //cursor MODIFY mode. Digit at the cursor position increses or decreases with 2 & 3 buttons. 
{

 if (buttonReadFunction()==2){ 
    delay(100);
       if (cursor_num[cursPos] == 1 && timelapseFrames - cursor_add[cursPos] > -1){ //handles subtraction from timelapseFrames
          timelapseFrames = timelapseFrames - cursor_add[cursPos];
       }

       else if (cursor_num[cursPos] == 2 && interval_m - cursor_add[cursPos] > -1){ //handles subtraction from interval_m
          interval_m = interval_m - cursor_add[cursPos];
       }

      else if (cursor_num[cursPos] == 3 && interval_s - cursor_add[cursPos] > -1){ //handles subtraction from interval_s
         interval_s = interval_s - cursor_add[cursPos];
      }
    
    }
    else if (buttonReadFunction()==3){ 
    delay(100);
       if (cursor_num[cursPos] == 1 && timelapseFrames + cursor_add[cursPos] < 999999){ //handles adding to timelapseFrames
         timelapseFrames = timelapseFrames + cursor_add[cursPos];
        }

       else if (cursor_num[cursPos] == 2 && interval_m + cursor_add[cursPos] < 100){ //handles adding to interval_m
         interval_m = interval_m + cursor_add[cursPos];
        }

       else if (cursor_num[cursPos] == 3 && interval_s + cursor_add[cursPos] < 60){ //handles adding to interval_s
        interval_s = interval_s + cursor_add[cursPos];
        }
    
    }

    else if (buttonReadFunction()==4){ //sends cursor back to MOVEMENT mode.
    delay(100);
    cursMode = false;
    }





    
    }




lcd.setCursor(cursor_col[cursPos],cursor_row[cursPos]); //set lcd cursor to cursor position 
lcd.print(" "); //print " " over the number that was there

}

// create arrays of strings for display *********************
 
 char* mainMenu[]={   //MENU 0
 "  [MOVE] times  ",  //index 0
 "   keys  start  ",  //index 1

 "   move [TIMES] ",  //index 2
 "   keys  start  ",  //index 3

 "   move  times  ",  //index 4
 "  [KEYS] start  ",  //index 5

 "   move  times  ",  //index 6
 "   keys [START] "   //index 7
  };


char* moveMenu[]={    //MENU 1
 "MoveMenu:",         //index 0
 
 "[HOM]<< >> key  ",  //index 1
 " hom[<<]>> key  ",  //index 2
 " hom <<[>>]key  ",  //index 3
 " hom << >>[KEY] "   //index 4
  };

char* timesMenu[]={   //MENU 2
 "Frames:",           //index 0
 "Interval:00M 00s"   //index 1
  };

char* keysMenu[]={    //MENU 3
 " Set Keyframes: ",  //index 0
 
 "[IN]  out  move ",  //index 1
 " in  [OUT] move ",  //index 2
 " in   out [MOVE]"   //index 3
  };

char* startMenu[]={   //MENU 4
 " [START] test   ",  //index 0
 "  goIn  goOut   ",  //index 1

 " start  [TEST]  ",  //index 2
 "  goIn  goOut   ",  //index 3

 " start  test    ",  //index 4
 " [GOIN] goOut   ",  //index 5

 " start  test    ",  //index 6
 "  goIn [GOOUT]  ",  //index 7

 
  };

char* recordMenu[]={  //MENU 5
 "*REC*    [PAUSE]",  //index 0
 "00000f   00m 00s",  //index 1
 "*PSD*   [RESUME]",  //index 2
 "00000f   00m 00s"   //index 3
  };



void setup() {
  
Serial.begin(9600);

lcd.begin (16,2); //  <<----- My LCD was 16x2
lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE); // Switch on the backlight
lcd.setBacklight(HIGH);

pinMode(stepperXdirpin, OUTPUT);
pinMode(stepperXstppin, OUTPUT);
pinMode(stepperEnablepin, OUTPUT);
digitalWrite(stepperEnablepin, HIGH);

pinMode(camPin, OUTPUT);
digitalWrite(camPin, LOW);

pinMode(XendstopPin, OUTPUT);
digitalWrite(XendstopPin, HIGH); //enables internal pullup resistors 

}

void loop() {
//Display builder: looks at menuNumber and optionNumber and composes the display for the LCD **************



//Main Menu loop
 while (menuNumber == 0){ 
 lcd.setCursor(0,0);
 lcd.print(mainMenu[optionNumber]);  
 lcd.setCursor(0,1);
 lcd.print(mainMenu[optionNumber+1]);   

 if (optionNumber < 6 && buttonReadFunction() == 3){
  delay(250);
  optionNumber = optionNumber + 2;}

  else if (optionNumber > 0 && buttonReadFunction() == 2){
    delay(250);
    optionNumber = optionNumber - 2;}

  else if (optionNumber == 0 && buttonReadFunction() == 4){  //sends menu to Move Menu
    delay(250);
    lcd.setCursor(0,0);
    lcd.print("                ");  
    lcd.setCursor(9,0);
    lcd.print(camPosition);
    lcd.setCursor(0,1);
    lcd.print("                ");  
    optionNumber = 0;
    menuNumber = 1; 
  }

 else if (optionNumber == 2 && buttonReadFunction() == 4){  //sends menu to Times Menu
    delay(250);
    optionNumber = 0;
    menuNumber = 2;  
  }

  else if (optionNumber == 4 && buttonReadFunction() == 4){  //sends menu to Keys Menu
    delay(250);
    optionNumber = 0;
    menuNumber = 3;  
  }

  else if (optionNumber == 6 && buttonReadFunction() == 4){  //sends menu to Start Menu
    delay(250);
    optionNumber = 0;
    menuNumber = 4;  
  }
 
 }



//Move Menu loop
 while (menuNumber == 1){  
 lcd.setCursor(0,0);
 lcd.print(moveMenu[0]);  
 lcd.setCursor(0,1);
 lcd.print(moveMenu[optionNumber+1]);  
 
 if (optionNumber < 3 && buttonReadFunction() == 3){
  delay(250);
  optionNumber = optionNumber + 1;}

  else if (optionNumber > 0 && buttonReadFunction() == 2){
    delay(250);
    optionNumber = optionNumber - 1;}

  else if (optionNumber == 0 && buttonReadFunction() == 4){  // drives stepper motor back to home position
   motorHomeFunction(); 
    lcd.setCursor(9,0);
    lcd.print("0                "); 
  }

 else if (optionNumber == 1 && buttonReadFunction() == 4){  // drives stepper motor toward home
    digitalWrite(stepperXdirpin, LOW);
    digitalWrite(stepperEnablepin, LOW);
    
    while (180 < analogRead(analogInPin) && analogRead(analogInPin) < 220 && camPosition > 0){
      
      digitalWrite(stepperXstppin, HIGH);
      delay(2);
      digitalWrite(stepperXstppin, LOW);
      delay(2);
      camPosition = camPosition - 1;

      
      lcd.setCursor(9,0);
      lcd.print(camPosition);
    }
    
    digitalWrite(stepperEnablepin, HIGH);
    
  }

  else if (optionNumber == 2 && buttonReadFunction() == 4){  //drives stepper motor away from home
      digitalWrite(stepperXdirpin, HIGH);
      digitalWrite(stepperEnablepin, LOW);
      
      while (180 < analogRead(analogInPin) && analogRead(analogInPin) < 220){
        
        digitalWrite(stepperXstppin, HIGH);
        delay(2);
        digitalWrite(stepperXstppin, LOW);
        delay(2);
        camPosition = camPosition + 1;
        lcd.setCursor(9,0);
        lcd.print(camPosition);
        
       }
       digitalWrite(stepperEnablepin, HIGH);
      
  }

  else if (optionNumber == 3 && buttonReadFunction() == 4){  //sends menu to Keys Menu
    delay(250);
    optionNumber = 0;
    menuNumber = 3;  
  }
 else if (buttonReadFunction() == 1){  //sends menu to Main Menu
    delay(250);
    optionNumber = 0;
    menuNumber = 0;  
  }

 }




//Times Menu loop
 while (menuNumber == 2){ 

// prints top line of times menu
lcd.setCursor(0,0);
lcd.print("Frames:         ");
zeroPadFunction(timelapseFrames, 6, 8, 0);

//prints bottom line of times menu
lcd.setCursor(0,1);
lcd.print("Interval: ");
zeroPadFunction(interval_m, 2, 10, 1);
lcd.setCursor(12,1);
lcd.print(":");
zeroPadFunction(interval_s, 2, 13, 1);


delay(80);
cursorFunction(); //inserts the cursor's blank space and handles number modifications
delay(80);




if (buttonReadFunction() == 1){  //sends menu to Main Menu
    delay(250);
    optionNumber = 0;
    menuNumber = 0;  
  }
   
 }




//Keys Menu loop
 while (menuNumber == 3){ 
 lcd.setCursor(0,0);
 lcd.print(keysMenu[0]);  
 lcd.setCursor(0,1);
 lcd.print(keysMenu[optionNumber+1]); 

if (optionNumber < 2 && buttonReadFunction() == 3){
  delay(250);
  optionNumber = optionNumber + 1;}

  else if (optionNumber > 0 && buttonReadFunction() == 2){
    delay(250);
    optionNumber = optionNumber - 1;}

  else if (optionNumber == 0 && buttonReadFunction() == 4){  // Sets IN motion key frame
    delay(250);
    keyPositionIn = camPosition;
    Serial.print("IN:");
    Serial.println(keyPositionIn);

    lcd.setCursor(0,0);
    lcd.print("  Keyframe set  ");  
    lcd.setCursor(0,1);
    lcd.print(("for IN position."));
    delay(1000); //displays IN position message for one second
    lcd.setCursor(0,0); // clears screen
    lcd.print("                ");  
    lcd.setCursor(9,0);
    lcd.print(camPosition);
    lcd.setCursor(0,1);
    lcd.print("                "); 
    optionNumber = 0;
    menuNumber = 1; 
    
  }

 else if (optionNumber == 1 && buttonReadFunction() == 4){  // Sets OUT motion key frame
    delay(250);
    keyPositionOut = camPosition;
    Serial.print("OUT:");
    Serial.println(keyPositionOut);

    lcd.setCursor(0,0);
    lcd.print("  Keyframe set  ");  
    lcd.setCursor(0,1);
    lcd.print(("for OUT position"));
    delay(1000); //displays OUT Position message for one second
    lcd.setCursor(0,0); // clears screen
    lcd.print("                ");  
    lcd.setCursor(9,0);
    lcd.print(camPosition);
    lcd.setCursor(0,1);
    lcd.print("                "); 
    optionNumber = 0;
    menuNumber = 1; 
    
  }

 else if (optionNumber == 2 && buttonReadFunction() == 4){  //sends menu back to MOVE menu
    delay(250);
    lcd.setCursor(0,0);
    lcd.print("                ");  
    lcd.setCursor(9,0);
    lcd.print(camPosition);
    lcd.setCursor(0,1);
    lcd.print("                "); 
    optionNumber = 0;
    menuNumber = 1;  
  }

  
 else if (buttonReadFunction() == 1){  //sends menu to Main Menu
    delay(250);
    optionNumber = 0;
    menuNumber = 0;  
  }

 }




//Start Menu loop
 while (menuNumber == 4){ 
 lcd.setCursor(0,0);
 lcd.print(startMenu[optionNumber]);  
 lcd.setCursor(0,1);
 lcd.print(startMenu[optionNumber+1]); 

 if (optionNumber < 6 && buttonReadFunction() == 3){
  delay(250);
  optionNumber = optionNumber + 2;}

  else if (optionNumber > 0 && buttonReadFunction() == 2){
    delay(250);
    optionNumber = optionNumber - 2;}

  else if (optionNumber == 4 && buttonReadFunction() == 4){  //drives stepper to the IN point
    delay(250);
    Serial.println("drives stepper to the IN point");
    motorHomeFunction(); //move camera to home, zero position.
    motorInFunction(); //moves camera from home to IN position
    }

  else if (optionNumber == 6 && buttonReadFunction() == 4){  //drives stepper to the OUT point
    delay(250);
    Serial.println("drives stepper to the OUT point");
    motorHomeFunction(); //move camera to home, zero position.
    motorOutFunction();//moves camera from home to IN position
    }

  else if (optionNumber == 2 && buttonReadFunction() == 4){  //signals the DSLR camra to take a picture
    delay(250);
   takePictureFunction();
  }

  else if (optionNumber == 0 && buttonReadFunction() == 4){  //STARTS the timelapse sequence
    delay(250);
    optionNumber = 0;
    menuNumber = 5;  
  }
  
  else if (buttonReadFunction() == 1){  //sends menu to Main Menu
    delay(250);
    optionNumber = 0;
    menuNumber = 0;  
  }
 
 }




//Record Menu loop

while (menuNumber == 5){ 
 lcd.setCursor(0,0);
 lcd.print(recordMenu[0]);  
 
 lcd.setCursor(5,0);
 lcd.print("           "); 
 
 lcd.setCursor(0,1);
 lcd.print("moving to start!"); 

framecount = 0; //initializes framecount for the timelapse sequence

motorHomeFunction(); //move camera to home, zero position.
delay(250);
motorInFunction(); //moves camera from home to IN position

delay(2000); //let camera settle

takePictureFunction(); //Takes first frame of timelapse sequence, ignored if seqence is QUIT.
framecount++; 

zeroPadFunction(framecount, 5, 0, 1);
    lcd.setCursor(5,1);
    lcd.print("f   ");  //letter 'f' after frame count

    

long interval = (interval_m * 60000) + (interval_s * 1000);
long movementIncrement = (keyPositionOut - keyPositionIn) / (timelapseFrames-1);   
  

prevMillis = millis();
  
  while(framecount < timelapseFrames && menuNumber == 5){ //inner Record Menu loop, provides options during recording.
    
  currentMillis = millis();
    


  lcd.setCursor(0,0);
  lcd.print(recordMenu[optionNumber]);  
  
  

   if (optionNumber == 0 && buttonReadFunction() == 4){  //Pauses recording
    delay(250);
    optionNumber = 2;
   }


   else if (optionNumber ==2 && buttonReadFunction() == 4){ //Resumes recording
    delay(250);
    optionNumber = 0;
  }

   
   else if (optionNumber ==2 && buttonReadFunction() == 2){ //moves selection from 'RESUME' to 'QUIT' option
    delay(250);
    optionNumber = 2;
  }


   else if (optionNumber==0 && currentMillis - prevMillis > interval) { //Executes the timelapse capture sequence
      //move to new frame position
      digitalWrite(stepperEnablepin, LOW); //enables stepper driver card
    
      if (movementIncrement > 0){ //sets direction of stepper motor movement
          digitalWrite(stepperXdirpin, HIGH);
          }
      else{
        digitalWrite(stepperXdirpin, LOW);
          }

          for (long i=0; i < abs(movementIncrement); i = i + 1){ //loop drives stepper motor one increment of the total timelapse
      
           digitalWrite(stepperXstppin, HIGH);
           delay(8);
           digitalWrite(stepperXstppin, LOW);
           delay(8); 
           
            }
           
    camPosition = camPosition + movementIncrement;
    digitalWrite(stepperEnablepin, HIGH); //disables stepper driver card
    
    delay(2000);//let camera settle
    takePictureFunction();//take picture
    framecount++; //increment framecounter by one
    zeroPadFunction(framecount, 5, 0, 1);
    lcd.setCursor(5,1);
    lcd.print("f");  

    

    
    prevMillis = currentMillis;
    countdownSec = 0; //resets the frame countdown variable for the display after a picture is taken
   }


   else if(buttonReadFunction() == 1){ //Quit process, go back to MAIN menu.
      delay(250);
      optionNumber = 0;
      menuNumber = 0; //Quits recording, returns to MAIN menu.
      
  }

   else  { //update the display of framecount and timer countdown

    if (currentMillis - previousDispMillis > 1000 && optionNumber==0){

  

    zeroPadFunction((((interval/1000)-countdownSec)/60), 2, 9, 1);  //displays whole number minutes remaining before exposure
    Serial.println(countdownSec);

    lcd.setCursor(11,1);
    lcd.print("m ");  
    
    zeroPadFunction(((interval/1000)-countdownSec)-(60*(((interval/1000)-countdownSec)/60)), 2, 13, 1); //displays whole number seconds (59 or less) remaining before exposure
    Serial.println(countdownSec);

    lcd.setCursor(15,1);
    lcd.print("s ");  
    
    
    countdownSec ++;

  
    previousDispMillis = currentMillis;
    }


    
   }
 }
 



while(buttonReadFunction()==0 && menuNumber == 5){ //Displays the Recording Done! message.
 delay(250);
 lcd.setCursor(0,0);
 lcd.print("Recording Done! ");  
 lcd.setCursor(0,1);
 lcd.print("[Press any key] "); 

}
optionNumber = 0; 
menuNumber = 0; //Returns to MAIN menu.

}
}


