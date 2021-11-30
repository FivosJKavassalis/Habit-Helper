#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

#define ONE_SECOND (1000)

SoftwareSerial BTSerial(3, 4);

char BluetoothData; // the data given from Computer
//Global Variables used in ISR
volatile byte cnt = 0; //counts through minute
volatile byte prevcnt = 0;
volatile byte yess = 0; //number of minutes spent in a row slouching
volatile byte nos = 0; //number of minutes spent in a row not slouching
volatile byte yesa = 0; //number of minutes spent in a row inactive
volatile byte noa = 0; //number of minutes spent in a row not inactive
volatile byte prevs = 0;//previously slouching
volatile byte preva = 0;//previously inactive
volatile byte scnt = 0; //slouch counter within minute
volatile byte acnt = 0; //Inactivity counter within minute
volatile byte iss = 0; //is slouching. 1 if slouching, 0 if not.
volatile byte isa = 0; //is inactive. 1 if inactive, 0 if not.
volatile unsigned int i = 0; //unsigned number from 0-65535, since we must be able to count (at most) the number of minutes in a day (1440)
volatile unsigned int j = 0; //same as above
byte inactivityArr[500] = {0};
byte one = 1;
byte zero = 0;
Adafruit_BNO055 bno = Adafruit_BNO055();
byte measureS(imu::Vector<3> accelerometer);
byte measureA(imu::Vector<3> linearaccel);


void setup() {

   Serial.begin(9600);
   BTSerial.begin(9600);
   pinMode(5, OUTPUT); // for vibrating motor...
  
   /* Initialise the sensor */
   if (!bno.begin())
   {
     /* There was a problem detecting the BNO055 ... check your connections */
     Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
     while (1);
   }
   
   delay(1000);
   bno.setExtCrystalUse(true);
}


void loop() {
  
   Adafruit_BNO055 bno = Adafruit_BNO055(55);
   imu::Vector<3> accelerometer = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
   imu::Vector<3> linearaccel = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
  
   if (BTSerial.available()) {
       BluetoothData = BTSerial.read();
       if (BluetoothData == 'p') { // request for posture data
      
           int k;
           int minSlouch = 0;
           int minNotSlouch = 0;
          
           for (k = (i-1); k >= 0; k--) {
               if ((minSlouch + minNotSlouch) > 1440) {
                  break;
               }
              
               if (k % 2) { // (k mod 2) == 1
                  minSlouch = minSlouch + EEPROM.read(k);
               } else {
                  minNotSlouch = minNotSlouch + EEPROM.read(k);
               }
          
           }
           BTSerial.print("Slouching for ");
           BTSerial.print(minSlouch, DEC);
           BTSerial.println(" minutes");
           BTSerial.print("Not slouching for ");
           BTSerial.print(minNotSlouch, DEC);
           BTSerial.println(" minutes");
       }
       
       if (BluetoothData == 'a') { // request for pgysical inactivity data
           int m;
           //byte printArr[1000] = {0}; //placeholder!
           int minInactive = 0;
           int minActive = 0;
          
           for (m = (j-1); m >= 0; m--) {
          
               if ((minInactive + minActive) > 1440) {
                  break;
               }
               
               if (m % 2) {
                  minInactive = minInactive + inactivityArr[m];
               } else {
                  minActive = minActive + inactivityArr[m];
               }
          
           }
          
           BTSerial.print("Inactive for ");
           BTSerial.print(minInactive, DEC);
           BTSerial.println(" minutes");
           BTSerial.print("Active for ");
           BTSerial.print(minActive, DEC);
           BTSerial.println(" minutes");
      
       }
       
       if (BluetoothData == 'g') { // For sloucing graph
           byte slouchVal;
           byte noSlouchVal;
          
           int f;
           for (f=0; f<i; f++) {
               if((f%2) != 0) {
                   slouchVal = EEPROM.read(f);
                   
                   while (slouchVal > 0) {
                       BTSerial.write(130); // considered HIGH value in graph
                       slouchVal--;
                   }
              
               } else {
                   noSlouchVal = EEPROM.read(f);
                  
                   while (noSlouchVal > 0) {
                       BTSerial.write(20); // considered LOW value in graph
                       noSlouchVal--;
                   }
              
               }
           }
       }
       
       if (BluetoothData == 'h') {
           byte inactivityVal;
           byte activityVal;
          
           int g;
           for (g=0; g<j; g++) {
               if((g%2) != 0) {
                   inactivityVal = inactivityArr[g];
                   
                   while (inactivityVal > 0) {
                       BTSerial.write(130);
                       //Serial.print(1);
                       inactivityVal--;
                   }
              
               } else {
                   activityVal = inactivityArr[g];
                   Serial.print("Init activity value: ");
                   Serial.println(activityVal);
                   Serial.write(activityVal);
                   while (activityVal > 0) {
                       BTSerial.write(20);
                       activityVal--;
                   }
              
               }
           }
       }
  
   }
   //in the first 10 seconds
   if (cnt < 10) {
  
       //get measurement(either 1 slouching or 0 not slouching)
       iss = measureS(accelerometer); //measureS returns 1 if slouching, 0 otherwise
       //Serial.print(iss);
       isa = measureA(linearaccel); //measureA returns 1 if inactive, 0 otherwise
       Serial.print(isa);
       scnt = scnt + iss;
       acnt = acnt + isa;
       iss = 0;
       isa = 0;
   }
   
   // If the 11th (10) to the 59th (58) second, do nothing (except potential motor vibration).
   // At 12th second, vibrate if scnt > 5
   if ((cnt == 12) && (scnt > 5)) {
  
       digitalWrite(5, HIGH);
       cnt++;
       delay(1000);
       digitalWrite(5, LOW);
  
   }
   
   if (cnt == 59) {
  
       //If the user is slouching this minute
       if (scnt > 5) {
           if (prevs == 1) { //If was slouching last minute
              yess++;//add 1 minute spent slouching
              
           } else { //If was not slouching
               //SEND NOS TO THE ROM ARRAY, this stores the number of consecutive minutes spent not slouching
               EEPROM[i] = nos;
               i++;//move to the next cell for next time
               nos = 0;
               yess++;//add 1 minute spent slouching
               prevs = 1; //was slouching this minute (this info will be used in the next minute)
           }
           
       } else { // if the user is not slouching this minute (if scnt<=5)
           if (prevs == 1) { //If was slouching last minute
              //SEND YESS TO THE ROM ARRAY
              EEPROM[i] = yess;
              i++;//move to the next cell for next time
              yess = 0;
              nos++;
              prevs = 0;
           } else { //If was not slouching
              nos++;
           }
       }
      
       if (acnt > 5) {
           if (preva == 1) {
               yesa++;//add 1 minute
           } else {
               inactivityArr[j] = noa;
               j++;//move to the next cell for next time
               noa = 0;
               yesa++;
               preva = 1;
           }
       } else {
           if (preva == 1) {
          
               inactivityArr[j] = yesa;
               Serial.print("YESA ");
               Serial.print(yesa);
               Serial.println(" YESA");
               j++;//move to the next cell for next time
               yesa = 0;
               noa++;
               preva = 0;
           } else {
               noa++;
           }
       }
       scnt = 0; //reset slouch counter within minute
       acnt = 0; //reset inactivity counter within minute
   }
  
   cnt = (cnt + 1) % 60; // tranverses from 0 to 59 as a representation of a second within a minute
   if (i == 999) i = 0; // MAY CHANGE(CHECK EEPROM)!!!!!
   delay(ONE_SECOND);
}

// function to check if the user is slouching with IMU
// returns 1 if slouching, 0 if not
byte measureS(imu::Vector<3> accelerometer) {
   byte slouch;
   if ((accelerometer.y() < 8) && (accelerometer.z() > 6))
      slouch = 1;
   else
      slouch = 0;
      
   return slouch;
}

// function to check if the user is inactive with IMU
// returns 1 if inactive, 0 if not
byte measureA(imu::Vector<3> linearaccel) {
   byte inactive;
   if ((linearaccel.x() <= 0.85) && (linearaccel.x() >= -0.85) && (linearaccel.y() <= 0.85) &&
      (linearaccel.y() >= -0.85) && (linearaccel.z() <= 0.85) && (linearaccel.z() >= -0.85))
      inactive = 1;
   else
      inactive = 0;
   
   return inactive;
}

 
