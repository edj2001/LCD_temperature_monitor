
/*
 * IRremote: IRrecvDemo - demonstrates receiving IR codes with IRrecv
 * An IR detector/demodulator must be connected to the input RECV_PIN.
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 */

/*
Pin 7 is pb nput to select max/min or logged data
*/

#define UseLCDI2C
#define UseLCDRows 4
#define UseLCDColumns 20
#undef UseInternal1V1
//Internal reference is 2.56V for Leonardo, 1.1 for Nano.
#ifdef __AVR_ATmega32U4__
  #define VInternal 2.56
#else
  #define VInternal 1.1
#endif

#include <IRremote.h>
#include <LED.h>
#include <TimerObject.h>
//include the library
#include <eRCaGuy_analogReadXXbit.h>
#include <Time.h>
#include "List.h"

// calculate Analog Reference Voltage from built-in 1.1V reference
#include <AnalogVRef.h>

float VAREF;

#ifndef UseLCDI2C
  #include <LiquidCrystal595.h>
  // initialize the library with the numbers of the interface pins + the row count
  // datapin, latchpin, clockpin, num_lines
  LiquidCrystal595 lcd(2,3,4);

#else
  #include <Wire.h>
  #include <LiquidCrystal_I2C.h>
  LiquidCrystal_I2C lcd(0x27,UseLCDColumns,UseLCDRows); //set the LCD address to 0x27 for a 16 chars and 2 line display

#endif

List DataList(UseLCDRows);

//instantiate an object of this library class; call it "adc"
eRCaGuy_analogReadXXbit adc;

boolean SerialDebug = false;

//Global constants
const unsigned int num_samples = 10;  //change this to 1 to take only a single reading; leave it >1 to return an avg. of this # of readings
//const uint8_t pin = A0; //analogRead pin
const uint8_t pin = 0; //analogRead pin
//constants required to determine the voltage at the pin
//BE SURE YOU USE THE CORRECT ONE OF THESE WHEN CALCULATING THE VOLTAGE FROM A READING! Take notes of how these constants are used below.
const float MAX_READING_10_bit = 1023.0;
const float MAX_READING_11_bit = 2047.0;
const float MAX_READING_12_bit = 4095.0;
const float MAX_READING_13_bit = 8191.0;
const float MAX_READING_14_bit = 16383.0;
const float MAX_READING_15_bit = 32736.0;
const float MAX_READING_16_bit = 65472.0;
const float MAX_READING_17_bit = 130944.0;
const float MAX_READING_18_bit = 261888.0;
const float MAX_READING_19_bit = 523776.0;
const float MAX_READING_20_bit = 1047552.0;
const float MAX_READING_21_bit = 2095104.0;

//Global variables
float Vin_ARef; // 1.1V reference read in mv
float VCC_ARef; // Analog Reference voltage in mv

float analog_reading; //the ADC reading
float V; //Voltage calculated on the analog pin
float Temperature; //Rounded to 0.1 deg C
float Max_Temperature;
float Min_Temperature;
int Min_Hours;
int Min_Minutes;
int Max_Hours;
int Max_Minutes;

int RECV_PIN = 8;

IRrecv irrecv(RECV_PIN);

decode_results results;
String AssembledString = "0000";
int AssembleResult;

boolean TimeMenu;
boolean CollectMinMax;
boolean MinMaxUpdated;
boolean ShowMinMax = false;

//LED status
LED statusLED = LED(LED_BUILTIN);

//Task Timers
TimerObject *T_freemem = new TimerObject(1000);
TimerObject *T_LCDUpdate = new TimerObject(500);
TimerObject *T_TimeMenu = new TimerObject(180000);
//TimerObject *T_LogData = new TimerObject(900000);
TimerObject *T_LogData = new TimerObject(600000);

//TimerObject *T_TimeMenu = new TimerObject(10000);
//TimerObject *T_LogData = new TimerObject(60000);

TimerObject *T_ScrollDisplay = new TimerObject(1500);

int AssembleInput(String *InputResult, unsigned long NewEntry)
{
  String NewEntryString = "";
  int ReturnCode = 0;
  switch (NewEntry) {
    case 0xFF6897:
      NewEntryString = "0";
      break;
    case 0xFF30CF:
      NewEntryString = "1";
      break;
    case 0xFF18E7:
      NewEntryString = "2";
      break;
    case 0xFF7A85:
      NewEntryString = "3";
      break;
    case 0xFF10EF:
      NewEntryString = "4";
      break;
    case 0xFF38C7:
      NewEntryString = "5";
      break;
    case 0xFF5AA5:
      NewEntryString = "6";
      break;
    case 0xFF42BD:
      NewEntryString = "7";
      break;
    case 0xFF4AB5:
      NewEntryString = "8";
      break;
    case 0xFF52AD:
      NewEntryString = "9";
      break;
    case 0xFF22DD: //play/pause
      ReturnCode = 1;
      break;
//    default: 
      // if nothing else matches, do the default
      // default is optional
  }  
  *InputResult += NewEntryString;
  return ReturnCode;
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void PrintSerialData()
//calculate and print to serial port free ram amount
{
  Serial.print("Free Ram: ");
  Serial.print(freeRam()); 
  
  Serial.print("   VARef/VCC/Vin/Temperatures: ");
  Serial.print(VAREF,3);
  Serial.print("V ");
  Serial.print(VCC_ARef,3); 
  Serial.print("V ");
  Serial.print(V);
  Serial.print("V ");
  Serial.print(Temperature);
  Serial.print(" deg C");
  Serial.println("");
}

void PrintLCDFloat(float fIn)
{
  float temp;
  temp = fIn;

  lcd.print(fIn,1);

/*  
  lcd.print(int(temp));
  lcd.print(".");
  lcd.print(abs(int(10*temp)-10*int(temp)));
  */
  //not sure why this is needed
  //covers up giberish
  lcd.print(" ");
  
}

void PrintHHMMLCD(int Hour, int Minute)
{
      if (Hour < 10 ) lcd.print(" ");
      lcd.print(Hour);
      lcd.print(":");
      if (Minute < 10 ) lcd.print("0");
      lcd.print(Minute);

}

void GetCurrentHHMM(int *Hour, int *Minute)
{
      time_t t = now(); // Store the current time in time variable t 
      *Hour = hour(t);
      *Minute = minute(t);

}


void PrintLCDData()
//update the data on the LCD display
{
    int CurrentHour;
    int CurrentMinute;
    
      T_ScrollDisplay->Update();
      lcd.setCursor(12, 0);
      //display current Temperature
      PrintLCDFloat(Temperature);
      
      if ( MinMaxUpdated)
      {
        //display Min/Max or logged temperatures
        if ( ShowMinMax)
        {
          // show Min and Max temperatures
          lcd.setCursor(0,0);
          PrintHHMMLCD(Max_Hours, Max_Minutes);
          lcd.print("X");
          PrintLCDFloat(Max_Temperature);
          lcd.setCursor(0,1);
          PrintHHMMLCD(Min_Hours, Min_Minutes);
          lcd.print("N");
          PrintLCDFloat(Min_Temperature);
        }
        else
        {
          //show Logged temperatures
          int displayLine;
          for ( displayLine = 0; displayLine < UseLCDRows; displayLine++ )
          {
            lcd.setCursor(0,displayLine);
            if ( DataList.isDisplay(displayLine))
            {
              PrintHHMMLCD(DataList.displayHours(displayLine), DataList.displayMinutes(displayLine));
              lcd.print(" ");
              PrintLCDFloat(DataList.displayValue(displayLine));
            }
            else lcd.print("----------");
            
          }
        }
      }
      
      //display current time
      lcd.setCursor(11,1);
      GetCurrentHHMM(&CurrentHour, &CurrentMinute);
      PrintHHMMLCD(CurrentHour, CurrentMinute);
}

void StartMinMaxCollection()
{
  CollectMinMax = true;
  Max_Temperature = -1000.0;
  Min_Temperature = 1000.0;
  lcd.clear();  
/* Log the first data point and start the timer for periodic logging */
  LogData();
  T_LogData->Start();
}

void UpdateMinMax()
{
  if (Temperature > Max_Temperature ) {
    Max_Temperature = Temperature;
    GetCurrentHHMM(&Max_Hours, &Max_Minutes);
  }
  
  if (Temperature < Min_Temperature ) {
    Min_Temperature = Temperature;
    GetCurrentHHMM(&Min_Hours, &Min_Minutes);
  }
  MinMaxUpdated = true;
}

void TimeMenuTimeout()
{
  T_TimeMenu->Stop();
  TimeMenu = false;
  StartMinMaxCollection();
}


void LogData()
{
  int CurrentHour;
  int CurrentMinute;
  
  GetCurrentHHMM(&CurrentHour, &CurrentMinute);
  DataList.insert(CurrentHour, CurrentMinute, Temperature);
}

void ScrollDisplay()
{
  DataList.displayNext();
}

void setup()
{
  #ifndef UseInternal1V1
    analogReference(DEFAULT);
    VAREF = 5.0;
  #else
    analogReference(INTERNAL);
    VAREF = VInternal;
  #endif

  if (SerialDebug) Serial.begin(9600);
  pinMode(6, INPUT_PULLUP); //backlight pushbutton 
  pinMode(7, INPUT_PULLUP); //selection pushbutton
  
  irrecv.enableIRIn(); // Start the receiver
  
  //blink this program number on the status led
  statusLED.off();
  delay(2000);
  statusLED.blink(2*200,2);
  delay(500);
  statusLED.blink(3*200,3);
  
//  irrecv.blink13(1);  //blink built-in LED when receiving.
  #ifndef UseLCDI2C
    // set up the LCD's number of columns and rows: 
    lcd.begin(16, 2);
  #else
    lcd.init();
  #endif
  lcd.print("Enter Time:");
  
  TimeMenu = true;
  CollectMinMax = false;
  
  T_freemem->setOnTimer(&PrintSerialData);
  T_freemem->Start();

  T_LCDUpdate->setOnTimer(&PrintLCDData);
  T_LCDUpdate->Start();

  //if the time isn't set, just go from 00:00
  T_TimeMenu->setOnTimer(&TimeMenuTimeout);
  T_TimeMenu->setSingleShot(true);
  T_TimeMenu->Start();

  //wait to start logging data until time has been set
  //or menu time expired
  T_LogData->setOnTimer(&LogData);

  //scroll the logged temperature display
  T_ScrollDisplay->setOnTimer(&ScrollDisplay);
  T_ScrollDisplay->Start();

}

void setTime_String( String HHMMString){
  int Hours = AssembledString.substring(0,2).toInt();
  int Minutes = AssembledString.substring(2).toInt();
  setTime(Hours,Minutes,0,1,1,2015);
}

void ReceiveIR()
{
  if (irrecv.decode(&results)) {
    if (SerialDebug) {
      Serial.print(results.decode_type);
      Serial.print(" ");
      Serial.println(results.value, HEX);
    }
    
    if ( results.value != REPEAT) {
      AssembleResult = AssembleInput(&AssembledString, results.value);
      if (AssembledString.length() < 4 ) AssembledString =+ "0000";
      if (AssembledString.length() > 4) AssembledString = AssembledString.substring(AssembledString.length()-4);
      lcd.setCursor(0, 1);
      if (AssembleResult == 1 ) {
//        AssembledString = "    ";
        lcd.print("            ");
        setTime_String(AssembledString);
        TimeMenuTimeout();
      }
      else {
        if(TimeMenu) {
          lcd.print(AssembledString.substring(0,2));
          lcd.print(":");
          lcd.print(AssembledString.substring(2));
        }
      }
    }
    
    irrecv.resume(); // Receive the next value
  }

}

void loop() {
  //local variables
  uint8_t bits_of_precision; //bits of precision for the ADC (Analog to Digital Converter)

  //do something with a received IR code
  ReceiveIR(); 

  #ifdef UseInternal1V1
    VAREF = VInternal;
  #else
// calculate Analog Reference voltage by measuring internal 1.1V reference
// these are 2 different versions of the same thing.
    Vin_ARef = read_vin_mv()/1000.0;  //this seems a bit flaky
    VCC_ARef = readVcc()/1000.0;      //this seems solid

    // fudge factor varies by board.
    VAREF = VCC_ARef*0.9541;
    //maybe smooth this result?
  #endif
  
  //12-bit ADC reading
  bits_of_precision = 12; //bits of precision for the ADC (Analog to Digital Converter)
  analog_reading = adc.analogReadXXbit(pin,bits_of_precision,num_samples); //get the avg. of [num_samples] 12-bit readings
  V = analog_reading/MAX_READING_12_bit*VAREF; //voltage

  #undef LM335
  #ifndef LM335
    //for LM35 sensor 10mV/degC
    Temperature = V*100.0;
  #else
    //for LM335 sensor, 10mV/Kelvin
    Temperature = 100*(V-2.7315);
  #endif

  //temperature rounded to 1 decimal
//  Temperature = int(Temperature*10.0+0.5)/10.0;
  
  if (CollectMinMax) UpdateMinMax();
  
  //periodically print data to serial port.  
  if (SerialDebug) T_freemem->Update();
  //periodically print data to LCD screen.  
  T_LCDUpdate->Update();
  //timeout the Time Input Menu
  T_TimeMenu->Update();
  //periodically collect Data logging
  T_LogData->Update();

  #ifdef UseLCDI2C 
    if (digitalRead(6)) {
      lcd.noBacklight();

    }
    else {
      lcd.backlight();
    }
  #endif
    
    if (digitalRead(7)) ShowMinMax=false; else ShowMinMax=true;
  
}
