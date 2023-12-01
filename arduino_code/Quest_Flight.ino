/**
 * 
 *                 20231110 Version 1.0
   @file flying.ino
   @author howell ivy
   @brief Logic for experiment My-Cool-Experiment
   @version 1.0
   @date 2023-07-15

  20231120 CLI_V1.0 Required to make a complete program - this file, CLI_V1.0,Quest_CLI.h, Quest_Flight.h,Quest_flight.cpp
                  cmd_takeSphot , cmd_takeSpiphoto, nophotophoto, nophoto30K --,clean up code for understanding.              
*/

#include "Quest_Flight.h"
#include "Quest_CLI.h"
#include <Servo.h>

//////////////////////////////////////////////////////////////////////////
//    This defines the timers used to control flight operations
//////////////////////////////////////////////////////////////////////////
//  Fast clock --- 1 hour = 5 min = 1/12 of an  hour
//     one millie -- 1ms

#define SpeedFactor 1    // = times faster

//////////////////////////////////////////////////////////////////////////

#define one_sec   1000                       // one second = 1000 millis
#define one_min   60*one_sec                 // one minute of time
#define one_hour  60*one_min                 // one hour of time
#define one_day   24*one_hour                // one day time

//**************Defining when to enter the phases
#define Water_time     (((one_sec * 30) / 1000) / SpeedFactor)     //go to Water Phase after 30 seconds
#define Mix_time       (((one_min * 1) / 1000) / SpeedFactor)      //go to Mixing Phase after 1 min
#define Grow_time      (((one_min * 2) / 1000) / SpeedFactor)      //"
#define Dry_time       (((one_min* 3) / 1000) / SpeedFactor)       //"

//**************Defining the IOs
#define LED IO7
#define WATERPUMP IO6
#define AIRPUMPF IO5
#define AIRPUMPB IO4
#define VIBRATOR IO3
#define SERVOCONTROL IO2
#define SERVOPOWER IO1

//**************Defin the default Times for the Routines (may get changed in different Phases)
uint32_t Photo_time = ((one_sec * 20) / SpeedFactor);         //takes a photo every 20 seconds per default
uint32_t Vibration_time = ((one_sec * 20) / SpeedFactor);     //vibrates every 20 seconds per default
uint32_t Airpump_time = ((one_sec * 5) / SpeedFactor);        //Changes between pumping forward, backwards and waiting every 5 sec (if not changed in Airpump Routine)

//**************Von Howel bruachen wir nicht
int sensor1count = 0;     //counter of times the sensor has been accessed
int sensor2count = 0;     //counter of times the sensor has been accessed

///////////////////////////////////////////////////////////////////////////
/**
   @brief Flying function is used to capture all logic of an experiment during flight.
*/
//************************************************************************
//   Beginning of the flight program setup

void Flying() {
  Serial.println("Here to Run Flight program, not done yet 20230718");
  Serial.println(" 20231116 working on it");
  //
  uint32_t PhotoTimer = millis();               //set Phototimer to effective 0
  uint32_t VibrationTimer = millis();             //clear VibrationTimer to effective 0
  uint32_t AirpumpTimer = millis();             //clear AirpumpTimer to effective 0
  //
  uint32_t one_secTimer = millis();             //set happens every second
  uint32_t sec60Timer = millis();               //set minute timer
  
  //*****************************************************************
  //   Here to set up flight conditions i/o pins, atod, and other special condition
  //   of your program
  //****************************************************************
  
  //**************Setup Servo
  Servo myservo;                    //initialzie servo
  myservo.attach(SERVOCONTROL);     //attach it to Io Servocontroll
  int servoangle;                   //initialize angle variable to read out later
  
  //**************Boolean to control if the Routines are triggeret (may get changed in different Phases)
  boolean Airpump_Acces = false;
  boolean Vibration_Acces = false;
  
  //**************Initiallising some Values
  int Airpumpcycle = 0;           //definiert ob die Pumpe vorwärts rückwärts oder gar nicht läuft
  int phase = 0;                  //Variable die Definiert in welcher phase der code ist
  int TemperatureLimit = 40;      //Temperaturlimite die nicht üebrschritten werden darf. Wird geprüft bevor der Vibrationsmotor angeschaltet wird
  
  //******************************************************************
  //------------ flying -----------------------

  Serial.println("Flying NOW  -  x=abort");                 //terminal output for abort back to test
  Serial.println("Terminal must be reset after abort");     //terminal reset requirement upon soft reset

  missionMillis = millis();     //Set mission clock millis, you just entered flight conditions
  
  /////////////////////////////////////////////////////
  //----- Here to start a flight from a reset ---------
  /////////////////////////////////////////////////////
 
  DateTime now = rtc.now();                   //get time now
  currentunix = (now.unixtime());             //get current unix time, don't count time not flying
  writelongfram(currentunix, PreviousUnix);   //set fram Mission time start now counting seconds unix time
  
  //***********************************************************************
  //***********************************************************************
  //  All Flight conditions are now set up,  NOW to enter flight operations
  //
  //***********************************************************************
  //***********************************************************************
  
  while (1) {
    //
    //----------- Test for terminal abort command (x) from flying ----------------------
    //
    while (Serial.available()) {      //Abort flight program progress
      byte x = Serial.read();         //get the input byte
      if (x == 'x') {                 //check the byte for an abort x
        return  ;                     //return back to poeration sellection
      }                               //end check
    }                                 //end abort check
  
//------------------------------------------------------------------

////////////////////////////////////////////////////////////////////
////////////////Determine which Phase we are in/////////////////////
////////////////////////////////////////////////////////////////////

  uint32_t t = readlongFromfram(CumUnix);  //read out the mission clock that does not get reset when power is lost

//********************Check what phase we are in and set the phase variable accordingly
  if (t < Water_time){
    phase = 0;
  }else if ((Water_time <= t) && (t < Mix_time)){
    phase = 1;
  }else if ((Mix_time <= t) && (t < Grow_time)){
    phase = 2;
  }else if ((Grow_time <= t) && (t < Dry_time)){
    phase = 3;
  }else if (Dry_time <= t){
    phase = 4;
  }

////////////////////////////////////////////////////////////////////
///////////////Diffrent Phase Routines//////////////////////////////
////////////////////////////////////////////////////////////////////
  switch (phase){
    case 1: //**********WATERPHASE****************************************

      //*****Making sure the gate to the drychamber is closed before turning waterpump on
      digitalWrite(SERVOPOWER, LOW);
      myservo.write(170);
      delay(2000);
      digitalWrite(SERVOPOWER, HIGH);

      //*****Turn on the Waterpump for 30 sec
      Serial.println("Waterpump on");
      digitalWrite(WATERPUMP, LOW);
      delay(one_sec*30);
      digitalWrite(WATERPUMP, HIGH);
      Serial.println("Waterpump OFF");
      
      //*******Set new Accesses:
      Airpump_Acces = false;    //in this phase the airpump should not turn on
      Vibration_Acces = false;  //in this phase the vibration motor should not turn on

      break;
      
    case 2: //**********MIXPHASE*****************************************

      //*****Set new times for the Mixing phase
      Vibration_time = 20*one_sec;    //Vibrate every 10 Seconds
      Photo_time = 20*one_sec;        //take photo every 10 seconds
      
      //*******Set new Accesses:
      Airpump_Acces = false;    //in this phase the airpump should not turn on
      Vibration_Acces = true;   //the vibration motor should turn on!
      
      break;

    case 3: //**********GROWGPHASE***************************************

      //*****Set new times for the Growing phase
      Photo_time = 20*one_sec;
      
      //*******Set new Acces:
      Airpump_Acces = true;       //theairpump should turn on!
      Vibration_Acces = false;    //in this phase the vibration morot should not turn on
      
      break;


    case 4: //**********DRYPHASE*********************************************



      //****Check if the Gate is already open
      digitalWrite(SERVOPOWER, LOW);                  //give power to servo
      servoangle = myservo.read();                    //read out servoangle
      if ((servoangle < 0) || (servoangle > 10)){     //check if the angle is far off
        myservo.write(5);                             //Turn Servo
        delay(2000);                                  //Spare time for gate to open before taking power
      } 
      digitalWrite(SERVOPOWER, HIGH);                 //take power away
      

      //*******Set new Phototime
      Photo_time = 30*one_sec;

      //*******Set new Accesses:
      Airpump_Acces = false;
      Vibration_Acces = false;

      //******Making sure that the airpump is off when switching phase (could be running when switich phase )
      digitalWrite(AIRPUMPF, HIGH);   //
      digitalWrite(AIRPUMPB, HIGH);   //
      
      break;
      
    default:
      break;
  }

//*******************************************************************************
//*********** One second counter timer will trigger every second ****************
//*******************************************************************************
    //  Here one sec timer - every second
    //
    if ((millis() - one_secTimer) > one_sec) {      //one sec counter
      one_secTimer = millis();                      //reset one second timer
      DotStarYellow();                              //turn on Yellow DotStar to Blink for running
      //
//****************** NO_NO_NO_NO_NO_NO_NO_NO_NO_NO_NO_ *************************
// DO NOT TOUCH THIS CODE IT IS NECESARY FOR PROPER MISSION CLOCK OPERATIONS
//    Mission clock timer
//    FRAM keep track of cunlitive power on time
//    and RTC with unix seconds
//------------------------------------------------------------------------------
      DateTime now = rtc.now();                           //get the time time,don't know how long away
      currentunix = (now.unixtime());                     //get current unix time
      Serial.print(currentunix); Serial.print(" ");      //testing print unix clock
      uint32_t framdeltaunix = (currentunix - readlongFromfram(PreviousUnix)); //get delta sec of unix time
      uint32_t cumunix = readlongFromfram(CumUnix);       //Get cumulative unix mission clock
      writelongfram((cumunix + framdeltaunix), CumUnix);  //add and Save cumulative unix time Mission
      writelongfram(currentunix, PreviousUnix);           //reset PreviousUnix to current for next time
//
//********* END_NO_END_NO_END_NO_END_NO_END_NO_END_NO_ **************************
      //
      //  This part prints out every second
      //
      Serial.print(": Mission Clock = ");      //testing print mission clock
      Serial.print(readlongFromfram(CumUnix));        //mission clock
      Serial.print(" is ");                        //spacer
      //
      //------Output to the terminal  days hours min sec
      //
      getmissionclk();
      Serial.print(xd); Serial.print(" Days  ");
      Serial.print(xh); Serial.print(" Hours  ");
      Serial.print(xm); Serial.print(" Min  ");
      Serial.print(xs); Serial.print(" Sec");
      
      //*************Printing phase we are in
      Serial.print("We are in Phase: ");
      switch(phase){
        case 0:
          Serial.println("Waitingphase");
          break;
        case 1:
          Serial.println("Waterphase");
          break;
        case 2:
          Serial.println("Mixphase");
          break;
        case 3:
          Serial.println("Growphase");
          break;
        case 4:
          Serial.println("Dryphase");
          break;
      }
      //
      //
       DotStarOff();
    }  // end of one second routine
//
//**********************************************************************
//********************** Voibration Routine ****************************
//**********************************************************************
    //
    if (((millis() - VibrationTimer) > Vibration_time) && Vibration_Acces) {    //Is it time to go in Vibration routine
      VibrationTimer = millis();                                                //Reset vibrationtimer
      Serial.println("Vibration Start");                                        //print that we are in vibration routinge

      //*******Check if we are under the Temperature limit:
      Serial.println("reading BME680");
      read_bme680();
      if (bme.temperature < TemperatureLimit){
        //******Turn on Vibrator                                    
        digitalWrite(VIBRATOR, LOW);     //Turn on Vibration                        
        delay(500);                      //let it Vibrate for 0.5Seconds                           
        digitalWrite(VIBRATOR, HIGH);    //Turn off Virbation
      }else{
        Serial.println("Too hot for Vibration");
      }
    }     
    //
//**********************************************************************
//*************************Photo Routine********************************
//********************************************************************** 
    //
    if ((millis() - PhotoTimer) > Photo_time) {
       PhotoTimer = millis();                    
       Serial.println("TakePhoto Start");
       
       //*******Making a photo
       digitalWrite(LED, LOW);     //Turn on LED
  
       //cmd_takeSpiphoto();       //Take photo
       delay(3000);                //Delay to give the camera time
  
       digitalWrite(LED, HIGH);    //Turn of LED
    }
//
//
//**********************************************************************
//*************************AirPump Routine********************************
//********************************************************************** 
    //
    if (((millis() - AirpumpTimer) > Airpump_time) && Airpump_Acces) {
        AirpumpTimer = millis();                   

    //************Cycling through forwards, backwards, and waiting
        switch(Airpumpcycle){
          case 0:
            digitalWrite(AIRPUMPF, HIGH);           //Turn off Airpump
            digitalWrite(AIRPUMPB, HIGH);           //Turn off Airpump
            Serial.println("Airpump wait");
            //We want the wait time to be twice as long as backwards and forwards phase
            //Therefore we factor the time till the routinge gets triggerd again by two
            Airpump_time *= 2;
            break;
          case 1:
            digitalWrite(AIRPUMPF, HIGH);           //Turn on Airpump backwards
            digitalWrite(AIRPUMPB, LOW);            //Turn on Airpump backwards
            Serial.println("Airpump backwards");
            //Reset to normal triggertime
            Airpump_time /= 2;
            break;
          case 2:
            digitalWrite(AIRPUMPF, LOW);            //Turn on Airpump backwards
            digitalWrite(AIRPUMPB, HIGH);           //Turn on Airpump backwards
            break;
        }
        Airpumpcycle = (1 + Airpumpcycle) % 3;      //Cycle through phase
    }
  }
}
//
//FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
//    This is a function to adds three values to the user_text_buffer
//    Written specificy for 2023-2024 Team F, Team B,
//    Enter the function with "add2text(1st interger value, 2nd intergre value, 3rd intergervalue);
//    the " - value1 " text can be changed to lable the value or removed to same space
//    ", value2 " and ", value 3 " masy also be removed or changed to a lable.
//    Space availiable is 1024 bytes, Note- - each Data line has a ncarrage return and a line feed
//
//example of calling routine:
//       //
//      int value1 = 55;
//      int value2 = 55000;
//      int value3 = 14;
//      add2text(value1, value2, value3);
//      //
//EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE     
//
void add2text(int value1,int value2,int value3){                 //Add value to text file
        if (strlen(user_text_buf0) >= (sizeof(user_text_buf0)-100)){    //Check for full
          Serial.println("text buffer full");                           //yes, say so
          return;                                                       //back to calling
        }
        char temp[11];                  // Maximum number of digits for a 32-bit integer 
        int index = 10;                 //Start from the end of the temperary buffer  
        char str[12];                   //digits + null terminator   
//--------- get time and convert to str for entry into text buffer ----
        DateTime now = rtc.now();                   //get time of entry
        uint32_t value = now.unixtime();            //get unix time from entry
        do {
            temp[index--] = '0' + (value % 10);     // Convert the least significant digit to ASCII
            value /= 10;                            // Move to the next digit
        } while (value != 0);
        strcpy(str, temp + index +1);               // Copy the result to the output string
//---------- end of time conversion uni time is now in str -------------       
        strcat(user_text_buf0, (str));              //write unix time
        //
        // unit time finish entry into this data line
        //
        strcat(user_text_buf0, (" - count= "));            // seperator
        strcat(user_text_buf0, (itoa(value1, ascii, 10)));
        strcat(user_text_buf0, (", value2= "));
        strcat(user_text_buf0, (itoa(value2, ascii, 10)));
        strcat(user_text_buf0, (", value3= "));
        strcat(user_text_buf0, (itoa(value3,  ascii, 10)));
        strcat(user_text_buf0, ("\r\n"));

        //Serial.println(strlen(user_text_buf0));  //for testing
}
//------end of Function to add to user text buffer ------       
//
//=============================================================================
//
////FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
//  Function to write into a 30K databuffer
//    char databuffer[30000];         // Create a character buffer with a size of 2KB
//    int databufferLength = 0;       // Initialize the buffer length
//  Append data to the large data buffer buffer always enter unit time of data added
//  enter: void dataappend(int counts, int ampli, int SiPM, int Deadtime) (4 values)
//
void dataappend(int counts,int ampli,int SiPM,int Deadtime) {          //entry, add line with values to databuffer
  //----- get and set time to entry -----
  DateTime now = rtc.now();                                               //get time of entry
  String stringValue = String(now.unixtime());                            //convert unix time to string
  const char* charValue = stringValue.c_str();                            //convert to a C string value
  appendToBuffer(charValue);                                              //Sent unix time to databuffer
  //----- add formated string to buffer -----
  String results = " - " + String(counts) + " " + String(ampli) + " " + String(SiPM) + " " + String (Deadtime) + "\r\n";  //format databuffer entry
  const char* charValue1 = results.c_str();                               //convert to a C string value
  appendToBuffer(charValue1);                                             //Send formated string to databuff
  //
  //  Serial.println(databufferLength);                                   //print buffer length for testing only
}
//-----------------------                                               //end dataappend
//----- sub part od dataappend -- append to Buffer -----
//-----------------------
void appendToBuffer(const char* data) {                                   //enter with charator string to append
  int dataLength = strlen(data);                                          //define the length of data to append
      // ----- Check if there is enough space in the buffer                           //enough space?
  if (databufferLength + dataLength < sizeof(databuffer)) {               //enouth space left in buffer
      // ----- Append the data to the buffer
    strcat(databuffer, data);                                             //yes enough space, add data to end of buffer
    databufferLength += dataLength;                                       //change to length of the buffer
  } else {
    Serial.println("Buffer is full. Data not appended.");                 //Not enough space, say so on terminal
  }       //end not enough space
}         //end appendToBuffer
//

//=================================================================================================================
//
