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

#define PHOTOS_ENABLED 0 // Should be 1 for the real program

//////////////////////////////////////////////////////////////////////////
//    This defines the timers used to control flight operations
//////////////////////////////////////////////////////////////////////////
//  Fast clock --- 1 hour = 5 min = 1/12 of an hour
//  one "milli" -- 1ms

float Speed_up_factor = 1.0f; // = times faster

//////////////////////////////////////////////////////////////////////////

const uint32_t one_sec = 1000;          // one second = 1000 millis
const uint32_t one_min = 60 * one_sec;  // one minute of time
const uint32_t one_hour = 60 * one_min; // one hour of time
const uint32_t one_day = 24 * one_hour; // one day time

//************** Define phases
enum GLOBAL_PHASE
{
    WAITING_PHASE,
    WATER_PHASE,
    MIX_PHASE,
    GROW_PHASE,
    DRY_PHASE
};

//************** Define the duration of the phases
const int Waiting_phase_duration = 30 * one_sec;
const int Water_phase_duration = 30 * one_sec;
const int Mix_phase_duration = 1 * one_min;
const int Grow_phase_duration = 1 * one_min;
// const int Dry_phase_duration = 1 * one_min;  // This phase never ends

//************** Define when to enter the phases (only change the durations above!)
const uint32_t Waiting_phase_start_time = 0;
const uint32_t Water_phase_start_time = Waiting_phase_start_time + (Waiting_phase_duration / 1000.0) / Speed_up_factor;
const uint32_t Mix_phase_start_time = Water_phase_start_time + (Water_phase_duration / 1000.0) / Speed_up_factor;
const uint32_t Grow_phase_start_time = Mix_phase_start_time + (Mix_phase_duration / 1000.0) / Speed_up_factor;
const uint32_t Dry_phase_start_time = Grow_phase_start_time + (Grow_phase_duration / 1000.0) / Speed_up_factor;

//************** Define constants
const int SERVO_ANGLE_CLOSED = 170;
const int SERVO_ANGLE_OPENED = 5;
const int SERVO_ANGLE_OPENED_TOL[2] = {0, 10};
const uint32_t SERVO_WAIT_TIME_MS = 2 * one_sec;
const int TEMPERATURE_UPPER_LIMIT = 40; // If this limit is exceeded, vibration motor is forbidden to run (heats up)
const uint32_t CAM_ILLUMINATION_DURATION_MS = 3 * one_sec;

//************** Define the IOs
#define LED_PIN IO7
#define WATERPUMP_PIN IO6
#define AIRPUMP_F_PIN IO5
#define AIRPUMP_B_PIN IO4
#define VIBRATION_PIN IO3
#define SERVOCONTROL_PIN IO2
#define SERVOPOWER_PIN IO1

//************** Define the default Times for the Routines (may get changed in different Phases)
uint32_t Photo_time;     // takes a photo every .. seconds (configurable for each phase)
uint32_t Vibration_time; // vibrates every .. seconds for ... seconds (configurable for each phase)
uint32_t Airpump_time;   // Changes between waiting, pumping backwards and pumping forward with configurable durations

//************** Global variables
Servo myservo;

enum AIRPUMP_PHASE
{
    AIR_FORWARD_PHASE,
    AIR_BACKWARD_PHASE,
    AIR_WAIT_PHASE
};
bool Airpump_enable;
int Airpump_phase; // Keeps track of the pumps state: off, backward, forward
const int Airpump_phase_sequence_length = 3;
int Airpump_phase_durations_ms[3] = {5 * one_sec, 10 * one_sec, 5 * one_sec}; // TODO: check the times
//                                    [FORWARD]    [BACKWARD]    [WAITING]

enum VIBRATION_PHASE
{
    VIB_ON_PHASE,
    VIB_OFF_PHASE
};
bool Vibration_enable;
int Vibration_phase; // Keeps track of the vibration state: on or off
const int Vibration_phase_sequence_length = 2;
int Vibration_phase_durations_ms[2] = {500, 20 * one_sec};
//                                    [ON]    [OFF]

int phase;      // Defines current phase
int servoangle; // initialize angle variable to read out later
bool illumination_led_state;

//************** Periphery Utility methods

void servo_close()
{
    // Turn on servo power
    digitalWrite(SERVOPOWER_PIN, HIGH);
    // Set servo position to closed
    myservo.write(SERVO_ANGLE_CLOSED);
    // Wait for servo to reach position
    delay(SERVO_WAIT_TIME_MS);
    // Turn off servo power
    digitalWrite(SERVOPOWER_PIN, LOW);
}

void servo_ensure_open()
{
    // Read out servoangle
    servoangle = myservo.read();
    // check if the angle is far off
    if ((servoangle < SERVO_ANGLE_OPENED_TOL[0]) || (servoangle > SERVO_ANGLE_OPENED_TOL[1]))
    {
        // Turn on servo power
        digitalWrite(SERVOPOWER_PIN, HIGH);
        // Set servo position to opened
        myservo.write(SERVO_ANGLE_OPENED);
        // Wait for servo to reach position
        delay(SERVO_WAIT_TIME_MS);
        // Turn off servo power
        digitalWrite(SERVOPOWER_PIN, LOW);
    }
}

void set_illumination_led(bool status)
{
    if (status)
    {
        digitalWrite(LED_PIN, HIGH); // Turn on LED_PIN
        illumination_led_state = true;
    }
    else
    {
        digitalWrite(LED_PIN, LOW); // Turn of LED_PIN
        illumination_led_state = false;
    }
}

void set_vibration_state(bool status)
{
    if (status)
    {
        //*******Check if we are below the Temperature limit:
        // Serial.print("reading BME680: ");
        // read_bme680(); // This is already regularly done in the vibration OFF phase switch
        Serial.println(bme.temperature);
        if (bme.temperature < TEMPERATURE_UPPER_LIMIT)
        {
            //******Turn on Vibration
            digitalWrite(VIBRATION_PIN, HIGH); // Turn on Vibration
        }
        else
        {
            Serial.println("Too hot for Vibration");
        }
    }
    else
    {
        digitalWrite(VIBRATION_PIN, LOW); // Turn off Vibration
    }
}

void set_pump_state(int state)
{
    // TODO: is the polarization and corresponding labeling "forward / backward" still correct like this?

    switch (state)
    {
    case AIR_WAIT_PHASE:
        digitalWrite(AIRPUMP_F_PIN, LOW); // Turn off Airpump
        digitalWrite(AIRPUMP_B_PIN, LOW); // Turn off Airpump
        Serial.println("Airpump wait");
        break;
    case AIR_BACKWARD_PHASE:
        digitalWrite(AIRPUMP_F_PIN, LOW); // Turn on Airpump backward
        digitalWrite(AIRPUMP_B_PIN, HIGH);  // Turn on Airpump backward
        Serial.println("Airpump backwards");
        break;
    case AIR_FORWARD_PHASE:
        digitalWrite(AIRPUMP_F_PIN, HIGH);  // Turn on Airpump forward
        digitalWrite(AIRPUMP_B_PIN, LOW); // Turn on Airpump forward
        Serial.println("Airpump forward");
        break;
    default:
        break;
    }
}

//************** Phase State Transitions
void setup_waiting_phase()
{
    // Turn off the Waterpump
    digitalWrite(WATERPUMP_PIN, LOW);

    // Disable airpump
    Airpump_enable = false;
    set_pump_state(AIR_WAIT_PHASE);

    // Disable vibration
    Vibration_enable = false;
    set_vibration_state(false);

    // Set new Phototime
    Photo_time = 1 * one_hour; // take photo every hour -- only to measure temperatur.
}

void setup_water_phase()
{
    // Making sure the gate to the drychamber is closed before turning waterpump on
    servo_close();

    // Turn on the Waterpump (for 30 sec)
    Serial.println("Waterpump ON");
    digitalWrite(WATERPUMP_PIN, HIGH);
    
    //Turn on the Airpump backwards to fight overpressure
    set_pump_state(AIR_BACKWARD_PHASE);

    // Disable vibration
    Vibration_enable = false;
    set_vibration_state(false);

    // Disable airpump
    Airpump_enable = false;
    set_pump_state(AIR_WAIT_PHASE);

    // Set new Phototime
    Photo_time = 1 * one_sec; // take photo every second
}

void setup_mix_phase()
{
    // Making sure the gate to the drychamber is closed
    // servo_close();  // Takes ~ 2 seconds

    // Turn off the Waterpump
    Serial.println("Waterpump OFF");
    digitalWrite(WATERPUMP_PIN, LOW);
    
    // Disable airpump
    Airpump_enable = false; // in this phase the airpump should not turn on
    set_pump_state(AIR_WAIT_PHASE);

    // Enable vibration
    Serial.print("reading BME680: ");
    read_bme680();
    Vibration_enable = true;
    Vibration_phase = VIB_ON_PHASE;
    Vibration_time = 0;                                         // Makes it initially direcly hit the phase switch of phase 0
    Vibration_phase_durations_ms[VIB_ON_PHASE] = 500;           // 0.5s ON
    Vibration_phase_durations_ms[VIB_OFF_PHASE] = 20 * one_sec; // 20s OFF

    // Set new Phototime
    Photo_time = 20 * one_sec; // take photo every 20 seconds
}

void setup_grow_phase()
{
    // Making sure the gate to the drychamber is closed
    // servo_close();  // Takes ~ 2 seconds

    // Turn off the Waterpump
    digitalWrite(WATERPUMP_PIN, LOW);

    // Disable vibration
    Vibration_enable = false;
    set_vibration_state(false);

    // Enable airpump
    Airpump_enable = true;
    Airpump_phase = AIR_FORWARD_PHASE;
    Airpump_time = 0;                                              // Makes it initially direcly hit the phase switch of phase 0
    Airpump_phase_durations_ms[AIR_FORWARD_PHASE] = 5 * one_sec;   // 5s forward
    Airpump_phase_durations_ms[AIR_BACKWARD_PHASE] = 10 * one_sec; // 10s backward
    Airpump_phase_durations_ms[AIR_WAIT_PHASE] = 5 * one_sec;      // 5s waiting

    // Set new Phototime
    Photo_time = 30 * one_min; // take photo every 30 minutes
}

void setup_dry_phase()
{
    // Open the molecular sieve chamber
    servo_ensure_open();

    // Disable vibration
    Vibration_enable = false;
    set_vibration_state(false);

    // Disable airpump
    Airpump_enable = false;
    set_pump_state(AIR_WAIT_PHASE); // TODO: No pump on here?

    // Set new Phototime
    Photo_time = 1 * one_hour; // take photo every hour
}

//************** (Not needed -- from Howell)
int sensor1count = 0; // counter of times the sensor has been accessed
int sensor2count = 0; // counter of times the sensor has been accessed

///////////////////////////////////////////////////////////////////////////
/**
   @brief Flying function is used to capture all logic of an experiment during flight.
*/
//************************************************************************
//   Beginning of the flight program setup

void Flying()
{
    Serial.println("Here to Run Flight program, not done yet 20230718");
    Serial.println(" 20231116 working on it"); // TODO: what is this?

    uint32_t PhotoTimer = millis();           // set Phototimer to effective 0
    uint32_t VibrationTimer = millis();       // clear VibrationTimer to effective 0
    uint32_t AirpumpTimer = millis();         // clear AirpumpTimer to effective 0
    uint32_t IlluminationLedTimer = millis(); // clear IlluminationLedTimer to effective 0

    uint32_t one_secTimer = millis(); // set happens every second
    uint32_t sec60Timer = millis();   // set minute timer

    //*****************************************************************
    //   Here to set up flight conditions i/o pins, atod, and other special condition
    //   of your program
    //****************************************************************

    //************** Setup Servo
    myservo.attach(SERVOCONTROL_PIN); // attach it to IO SERVOCONTROL_PIN

    //************** Put All IO's to LOW
    digitalWrite(IO7, LOW);
    digitalWrite(IO6, LOW);
    digitalWrite(IO5, LOW);
    digitalWrite(IO4, LOW);
    digitalWrite(IO3, LOW);
    digitalWrite(IO2, LOW);
    digitalWrite(IO1, LOW);
    digitalWrite(IO0, LOW);

    //************** Boolean to control if the Routines are triggered (may get changed in different Phases)
    Airpump_enable = false;
    Vibration_enable = false;

    //************** Initializing some values
    Airpump_phase = 0;     // Keeps track of the pumps state: forward, backward or off
    phase = WAITING_PHASE; // Defines current phase

    //******************************************************************
    //------------ flying -----------------------

    Serial.println("Flying NOW  -  x=abort");             // terminal output for abort back to test
    Serial.println("Terminal must be reset after abort"); // terminal reset requirement upon soft reset

    missionMillis = millis(); // Set mission clock millis, you just entered flight conditions

    /////////////////////////////////////////////////////
    //----- Here to start a flight from a reset ---------
    /////////////////////////////////////////////////////

    DateTime now = rtc.now();                 // get time now
    currentunix = (now.unixtime());           // get current unix time, don't count time not flying
    writelongfram(currentunix, PreviousUnix); // set fram Mission time start now counting seconds unix time

    //***********************************************************************
    //***********************************************************************
    //  All Flight conditions are now set up,  NOW to enter flight operations
    //
    //***********************************************************************
    //***********************************************************************

    ////////////////////////////////////////////////////////////////////
    // Determine which Phase we are in. After an unexpected power-cycle,
    // This may lead to jumping directly to a phase > WAITING_PHASE.
    ////////////////////////////////////////////////////////////////////

    uint32_t t = readlongFromfram(CumUnix); // read out the mission clock that does not get reset when power is lost

    //********************Check what phase we are in
    if (t < Water_phase_start_time)
    {
        phase = WAITING_PHASE;
        setup_waiting_phase();
    }
    else if ((Water_phase_start_time <= t) && (t < Mix_phase_start_time))
    {
        phase = WATER_PHASE;
        Serial.println("Unexpected power-cycle. Directly continuing with WATER phase.");
        setup_water_phase();
    }
    else if ((Mix_phase_start_time <= t) && (t < Grow_phase_start_time))
    {
        phase = MIX_PHASE;
        Serial.println("Unexpected power-cycle. Directly continuing with MIX phase.");
        setup_mix_phase();
    }
    else if ((Grow_phase_start_time <= t) && (t < Dry_phase_start_time))
    {
        phase = GROW_PHASE;
        Serial.println("Unexpected power-cycle. Directly continuing with GROW phase.");
        setup_grow_phase();
    }
    else if (t >= Dry_phase_start_time)
    {
        phase = DRY_PHASE;
        Serial.println("Unexpected power-cycle. Directly continuing with DRY phase.");
        setup_dry_phase();
    }

    while (1) // Event loop -- don't sleep in here, only wait for time to elapse.
    {
        //
        //----------- Test for terminal abort command (x) from flying ----------------------
        //
        while (Serial.available())
        {                           // Abort flight program progress
            byte x = Serial.read(); // get the input byte
            if (x == 'x')
            {           // check the byte for an abort x
                return; // return back to operation selection
            }           // end check
        }               // end abort check

        //------------------------------------------------------------------

        uint32_t t = readlongFromfram(CumUnix); // read out the mission clock that does not get reset when power is lost

        ////////////////////////////////////////////////////////////////////
        // Different Phase Routines
        // Note that the stuff here only covers the repetitive work _during_
        // a phase and the checks when to transit to a new phase. The actual
        // *changes* during the phase transition happen in `setup_..._phase()`.
        ////////////////////////////////////////////////////////////////////
        switch (phase)
        {
        case WAITING_PHASE: //********* WAITINGPHASE ****************************************

            if (t >= Water_phase_start_time)
            {
                // Phase transition
                phase = WATER_PHASE;
                Serial.println("Switch Phase: WATER");
                setup_water_phase();
            }
            break;

        case WATER_PHASE: //********* WATERPHASE ****************************************

            if (t >= Mix_phase_start_time)
            {
                // Phase transition
                phase = MIX_PHASE;
                Serial.println("Switch Phase: MIX");
                setup_mix_phase();
            }
            break;

        case MIX_PHASE: //********* MIXPHASE *******************************************

            if (t >= Grow_phase_start_time)
            {
                // Phase transition
                phase = GROW_PHASE;
                Serial.println("Switch Phase: GROW");
                setup_grow_phase();
            }
            break;

        case GROW_PHASE: //********* GROWPHASE ******************************************

            if (t >= Dry_phase_start_time)
            {
                // Phase transition
                phase = DRY_PHASE;
                Serial.println("Switch Phase: DRY");
                setup_dry_phase();
            }
            break;

        case DRY_PHASE: //********* DRYPHASE ********************************************

            // No transition from here on
            break;

        default:
            break;
        }

        //*******************************************************************************
        //*********** One second counter timer will trigger every second ****************
        //*******************************************************************************
        //  Here one sec timer - every second
        //
        if ((millis() - one_secTimer) > one_sec)
        {                            // one sec counter
            one_secTimer = millis(); // reset one second timer
            DotStarYellow();         // turn on Yellow DotStar to Blink for running
            //
            //****************** NO_NO_NO_NO_NO_NO_NO_NO_NO_NO_NO_ *************************
            // DO NOT TOUCH THIS CODE IT IS NECESARY FOR PROPER MISSION CLOCK OPERATIONS
            //    Mission clock timer
            //    FRAM keep track of cunlitive power on time
            //    and RTC with unix seconds
            //------------------------------------------------------------------------------
            DateTime now = rtc.now();       // get the time time,don't know how long away
            currentunix = (now.unixtime()); // get current unix time
            Serial.print(currentunix);
            Serial.print(" ");                                                       // testing print unix clock
            uint32_t framdeltaunix = (currentunix - readlongFromfram(PreviousUnix)); // get delta sec of unix time
            uint32_t cumunix = readlongFromfram(CumUnix);                            // Get cumulative unix mission clock
            writelongfram((cumunix + framdeltaunix), CumUnix);                       // add and Save cumulative unix time Mission
            writelongfram(currentunix, PreviousUnix);                                // reset PreviousUnix to current for next time
            //
            //********* END_NO_END_NO_END_NO_END_NO_END_NO_END_NO_ **************************
            //
            //  This part prints out every second
            //
            Serial.print(": Mission Clock = ");      // testing print mission clock
            Serial.print(readlongFromfram(CumUnix)); // mission clock
            Serial.print(" is ");                    // spacer
            //
            //------Output to the terminal  days hours min sec
            //
            getmissionclk();
            Serial.print(xd);
            Serial.print(" Days  ");
            Serial.print(xh);
            Serial.print(" Hours  ");
            Serial.print(xm);
            Serial.print(" Min  ");
            Serial.print(xs);
            Serial.print(" Sec");

            //*************Printing phase we are in
            // TODO: Do you really want to print that every second?
            Serial.print("We are in Phase: ");
            switch (phase)
            {
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
        } // end of one second routine

        //**********************************************************************
        //********************** Vibration Routine *****************************
        //**********************************************************************
        if (Vibration_enable && ((millis() - VibrationTimer) > Vibration_time))
        {                              // Is it time to go in Vibration routine
            VibrationTimer = millis(); // Reset vibrationtimer

            //************ Cycling through on and off
            switch (Vibration_phase)
            {
            case VIB_ON_PHASE:
                set_vibration_state(true);
                Vibration_time = Vibration_phase_durations_ms[Vibration_phase];
                break;

            case VIB_OFF_PHASE:
                set_vibration_state(false);
                Vibration_time = Vibration_phase_durations_ms[Vibration_phase];
                Serial.print("reading BME680: ");
                read_bme680();

            default:
                break;
            }
            Vibration_phase = (++Vibration_phase) % Vibration_phase_sequence_length; // Cycle through phase
        }

        //**********************************************************************
        //************************ AirPump Routine *****************************
        //**********************************************************************
        if (Airpump_enable && ((millis() - AirpumpTimer) > Airpump_time))
        {
            AirpumpTimer = millis();

            //************ Cycling through forwards, backwards, and waiting
            switch (Airpump_phase)
            {
            case AIR_FORWARD_PHASE:
                set_pump_state(AIR_FORWARD_PHASE);
                Airpump_time = Airpump_phase_durations_ms[Airpump_phase];
                break;
            case AIR_BACKWARD_PHASE:
                set_pump_state(AIR_BACKWARD_PHASE);
                Airpump_time = Airpump_phase_durations_ms[Airpump_phase];
                break;
            case AIR_WAIT_PHASE:
                set_pump_state(AIR_WAIT_PHASE);
                Airpump_time = Airpump_phase_durations_ms[Airpump_phase];
                break;
            }
            Airpump_phase = (++Airpump_phase) % Airpump_phase_sequence_length; // Cycle through phase
        }

        //**********************************************************************
        //************************ Photo Routine *******************************
        //**********************************************************************
        if ((millis() - PhotoTimer) > Photo_time)
        {
            PhotoTimer = millis();
            Serial.println("TakePhoto Start");

            //*******Making a photo
            if (illumination_led_state != true)
            {
                set_illumination_led(true);
            }
            // This works like a retriggerable mono-stable flipflop: the illumination LED is
            // kept ON at least CAM_ILLUMINATION_DURATION_MS from now on.
            IlluminationLedTimer = millis();

            if (PHOTOS_ENABLED)
            {
                cmd_takeSpiphoto(); // Take photo
            }
        }
        if ((illumination_led_state == true) && (millis() - IlluminationLedTimer) > CAM_ILLUMINATION_DURATION_MS)
        {
            // By now, the last time a photo was requested is more than CAM_ILLUMINATION_DURATION_MS ago and
            // the LED is still on --> turn off LED
            set_illumination_led(false);
        }
    }
}
//
// FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
//    This is a function to adds three values to the user_text_buffer
//    Written specificy for 2023-2024 Team F, Team B,
//    Enter the function with "add2text(1st interger value, 2nd intergre value, 3rd intergervalue);
//    the " - value1 " text can be changed to label the value or removed to same space
//    ", value2 " and ", value 3 " masy also be removed or changed to a lable.
//    Space availiable is 1024 bytes, Note- - each Data line has a ncarrage return and a line feed
//
// example of calling routine:
//       //
//      int value1 = 55;
//      int value2 = 55000;
//      int value3 = 14;
//      add2text(value1, value2, value3);
//      //
// EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
//
void add2text(int value1, int value2, int value3)
{ // Add value to text file
    if (strlen(user_text_buf0) >= (sizeof(user_text_buf0) - 100))
    {                                       // Check for full
        Serial.println("text buffer full"); // yes, say so
        return;                             // back to calling
    }
    char temp[11];  // Maximum number of digits for a 32-bit integer
    int index = 10; // Start from the end of the temperary buffer
    char str[12];   // digits + null terminator
    //--------- get time and convert to str for entry into text buffer ----
    DateTime now = rtc.now();        // get time of entry
    uint32_t value = now.unixtime(); // get unix time from entry
    do
    {
        temp[index--] = '0' + (value % 10); // Convert the least significant digit to ASCII
        value /= 10;                        // Move to the next digit
    } while (value != 0);
    strcpy(str, temp + index + 1); // Copy the result to the output string
                                   //---------- end of time conversion uni time is now in str -------------
    strcat(user_text_buf0, (str)); // write unix time
    //
    // unit time finish entry into this data line
    //
    strcat(user_text_buf0, (" - count= ")); // seperator
    strcat(user_text_buf0, (itoa(value1, ascii, 10)));
    strcat(user_text_buf0, (", value2= "));
    strcat(user_text_buf0, (itoa(value2, ascii, 10)));
    strcat(user_text_buf0, (", value3= "));
    strcat(user_text_buf0, (itoa(value3, ascii, 10)));
    strcat(user_text_buf0, ("\r\n"));

    // Serial.println(strlen(user_text_buf0));  //for testing
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
void dataappend(int counts, int ampli, int SiPM, int Deadtime)
{ // entry, add line with values to databuffer
    //----- get and set time to entry -----
    DateTime now = rtc.now();                    // get time of entry
    String stringValue = String(now.unixtime()); // convert unix time to string
    const char *charValue = stringValue.c_str(); // convert to a C string value
    appendToBuffer(charValue);                   // Sent unix time to databuffer
    //----- add formated string to buffer -----
    String results = " - " + String(counts) + " " + String(ampli) + " " + String(SiPM) + " " + String(Deadtime) + "\r\n"; // format databuffer entry
    const char *charValue1 = results.c_str();                                                                             // convert to a C string value
    appendToBuffer(charValue1);                                                                                           // Send formated string to databuff
                                                                                                                          //
                                                                                                                          //  Serial.println(databufferLength);                                   //print buffer length for testing only
}
//-----------------------                                               //end dataappend
//----- sub part od dataappend -- append to Buffer -----
//-----------------------
void appendToBuffer(const char *data)
{                                  // enter with charator string to append
    int dataLength = strlen(data); // define the length of data to append
                                   //  ----- Check if there is enough space in the buffer                           //enough space?
    if (databufferLength + dataLength < sizeof(databuffer))
    { // enouth space left in buffer
        // ----- Append the data to the buffer
        strcat(databuffer, data);       // yes enough space, add data to end of buffer
        databufferLength += dataLength; // change to length of the buffer
    }
    else
    {
        Serial.println("Buffer is full. Data not appended."); // Not enough space, say so on terminal
    }                                                         // end not enough space
} // end appendToBuffer
//

//=================================================================================================================
//
