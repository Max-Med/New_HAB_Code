/* HAB tracking code:
    sets ublox gps in flight mode then reads data and transmits over the radio, also periodically saves gps information to sd card
  
  ** MOSI - pin 11
  ** MISO - pin 12
  ** CLK - pin 13
  ** CS - pin 4
  ** UBLOX Tx - Rx
  ** UBLOX Rx - Tx
  ** NTX2B pin 7 - pin 9
*/

#include <string.h>
#include <util/crc16.h>
#include <SD.h>
#include <TinyGPS.h>
TinyGPS gps;

byte gps_set_sucess = 0 ;
const int chipSelect = 4;

long balt;
byte bhour, bminute, bsecond;
char blat [10], blon[10];

char datastring[100];
unsigned long count, previousmillis,sdcount, truecount;

#define RADIOPIN 9
float flat, flon;


void setup() {
  pinMode(10, OUTPUT);
    // see if the card is present and can be initialized:
  SD.begin(chipSelect);
   
  Serial.begin(9600);

  pinMode(RADIOPIN,OUTPUT);
  setPwmFrequency(RADIOPIN, 1);
  flat=0;  //initially set flat as 0 so can see if have ever had gps fix
  previousmillis = 0;
  sdcount = 0;
  truecount = 0;     //count of number of "good" gps datastrings
  
  uint8_t setNav[] = {
    0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00,
    0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0xDC };
  while(!gps_set_sucess)
  {
    sendUBX(setNav, sizeof(setNav)/sizeof(uint8_t));
    gps_set_sucess=getUBX_ACK(setNav);
  }
  gps_set_sucess=0;
  
  Serial.print("$PUBX,41,1,0007,0003,4800,0*13\r\n"); 
  Serial.flush();
  Serial.begin(4800);

  }

void loop() {
  unsigned long fix_age, time, date; ////defining local variables
  long alt;
  int year;
  byte month, day, hour, minute, second, hundredths;
  char lat[10], lon[10], checksum_str[8];
  
  while  (Serial.available()) {
    int c = Serial.read(); 
    if (gps.encode(c))  {
      
      gps.get_datetime(&date, &time, &fix_age);   //retrieve gps date and time and age of last good gps fix
      
      if (fix_age != TinyGPS::GPS_INVALID_AGE){  //check to make sure fix_age is valid suggesting has good gps lock

      gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &fix_age);  //if fix_age is valid then break up time/date into indivual hours, minutes etc.
      
      gps.f_get_position(&flat, &flon , &fix_age);  //gets the latittude and longitude in degrees
      //process lat and lon here
      dtostrf (flat, 9, 6, lat);       //turns latitude and longitude into strings rather than floats so can be transmitted
      dtostrf (flon, 9, 6, lon);
      
      alt= gps.altitude()/100;   //gets gps altitude in cm then divides by 100 to get in m (lose some precision but only need to nearest m)

      if (fix_age < 100 && gps.satellites() > 3 && gps.satellites() != 255 ){  //check to see if data is "good"
        bhour = hour;                         //if data is good save it as a "back-up" copy to use if gps fix is lost
        bminute = minute;
        bsecond = second;
        strncpy (blat,lat,10);
        strncpy (blon,lon,10);
        balt = alt;
        truecount =truecount + 1;      //add one to number of "good" gps datastrings
      }
      
      snprintf(datastring,sizeof(datastring),"$$MAX,%lu,%lu,%02u:%02u:%02u,%s,%s,%ld,%d,%lu",count,truecount,hour,minute,second,lat,lon,alt,gps.satellites(),fix_age);  //puts together datstring with gps information in standard format
      unsigned int CHECKSUM = gps_CRC16_checksum(datastring); // Calculates the checksum for this datastring
      snprintf(checksum_str,sizeof(checksum_str), "*%04X\n", CHECKSUM);
      strcat(datastring,checksum_str);
      rtty_txstring (datastring);
      count = count +1;
      previousmillis = millis();
      sdcount= sdcount + 1;  //add 1 to the count for sd card
       
      
      if (sdcount >= 3){   //when count reaches 3 then writes data to sd card, so only writes every 3rd datastring
         File dataFile = SD.open("datalog.txt", FILE_WRITE); // open the file. note that only one file can be open at a time,
         if (dataFile) {   // if the file is available, write to it:
            dataFile.println(datastring);
            dataFile.close();
            sdcount = 0 ;   
         } 
      }
        
      }
    }      
    else if (millis() - previousmillis > 30000){       //if no GPS fix or GPS fix is lost sends out datastring every 30 seconds
        previousmillis = millis();                     //reset previous millis to current millisqq
        if (flat == 0){                                //unlikely that flat will ever be exactly zero unless still waiting for initial gps fix so if flat=0 then send message saying waiting for gps fix
          snprintf(datastring,sizeof(datastring),"$$MAX,Waiting for fix,%d",gps.satellites());
        }
        
        else {               //if flat not equal to zero then must have lost gps fix so send old data with warning added 
         snprintf(datastring,sizeof(datastring),"$$MAX,WARNING STALE DATA:%lu,%lu,%02u:%02u:%02u,%s,%s,%ld,%d,0",count,truecount,bhour,bminute,bsecond,blat,blon,balt,gps.satellites());  //puts together datstring with old gps information in standard format
         }
         
        unsigned int CHECKSUM = gps_CRC16_checksum(datastring); // Calculates the checksum for this datastring
        snprintf(checksum_str,sizeof(checksum_str), "*%04X\n", CHECKSUM);
        strcat(datastring,checksum_str);
        rtty_txstring (datastring);
        count = count +1;
    }
  }


}

// Send a byte array of UBX protocol to the GPS
void sendUBX(uint8_t *MSG, uint8_t len) {
  for(int i=0; i<len; i++) {
    Serial.write(MSG[i]);
  }
  Serial.println();
}
boolean getUBX_ACK(uint8_t *MSG) {
  uint8_t b;
  uint8_t ackByteID = 0;
  uint8_t ackPacket[10];
  unsigned long startTime = millis();
 
  // Construct the expected ACK packet    
  ackPacket[0] = 0xB5;	// header
  ackPacket[1] = 0x62;	// header
  ackPacket[2] = 0x05;	// class
  ackPacket[3] = 0x01;	// id
  ackPacket[4] = 0x02;	// length
  ackPacket[5] = 0x00;
  ackPacket[6] = MSG[2];	// ACK class
  ackPacket[7] = MSG[3];	// ACK id
  ackPacket[8] = 0;		// CK_A
  ackPacket[9] = 0;		// CK_B
 
  // Calculate the checksums
  for (uint8_t i=2; i<8; i++) {
    ackPacket[8] = ackPacket[8] + ackPacket[i];
    ackPacket[9] = ackPacket[9] + ackPacket[8];
  }
 
  while (1) {
 
    // Test for success
    if (ackByteID > 9) {
      // All packets in order!
      return true;
    }
 
    // Timeout if no valid response in 3 seconds
    if (millis() - startTime > 3000) { 
      return false;
    }
 
    // Make sure data is available to read
    if (Serial.available()) {
      b = Serial.read();
 
      // Check that bytes arrive in sequence as per expected ACK packet
      if (b == ackPacket[ackByteID]) { 
        ackByteID++;
      } 
      else {
        ackByteID = 0;	// Reset and look again, invalid order
      }
 
    }
  }
}

void rtty_txstring (char * string)
 {
 
 /* Simple function to sent a char at a time to
 ** rtty_txbyte function.
 ** NB Each char is one byte (8 Bits)
 */
 
 char c;
 
 c = *string++;
 
 while ( c != '\0')
  {
  rtty_txbyte (c);
  c = *string++;
  }
}

void rtty_txbyte (char c)
 {
 /* Simple function to sent each bit of a char to
 ** rtty_txbit function.
 ** NB The bits are sent Least Significant Bit first
 **
 ** All chars should be preceded with a 0 and
 ** proceed with a 1. 0 = Start bit; 1 = Stop bit
 **
 */
 
 int i;
 
 rtty_txbit (0); // Start bit
 
 // Send bits for for char LSB first
 
 for (i=0;i<7;i++) // Change this here 7 or 8 for ASCII-7 / ASCII-8
  {
  if (c & 1) rtty_txbit(1);
  
  else rtty_txbit(0);
  
  c = c >> 1;
  
  }
  
 rtty_txbit (1); // Stop bit
 rtty_txbit (1); // Stop bit
 }
 
void rtty_txbit (int bit)
 {
 if (bit)
  {
  // high
  analogWrite(RADIOPIN,110);
  }
  
 else
  {
  // low
  analogWrite(RADIOPIN,100);
  }
 
 // delayMicroseconds(3370); // 300 baud
 delayMicroseconds(10000); // For 50 Baud uncomment this and the line below.
 delayMicroseconds(10150); // You can't do 20150 it just doesn't work as the
 // largest value that will produce an accurate delay is 16383
 // See : http://arduino.cc/en/Reference/DelayMicroseconds
 
}
 
uint16_t gps_CRC16_checksum (char *string)
 {
 size_t i;
 uint16_t crc;
 uint8_t c;
 
 crc = 0xFFFF;
 
 // Calculate checksum ignoring the first two $s
 for (i = 2; i < strlen(string); i++)
  {
  c = string[i];
  crc = _crc_xmodem_update (crc, c);
  }
 
 return crc;
 }
 
void setPwmFrequency(int pin, int divisor) {
 byte mode;
 if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
  switch(divisor) {
   case 1:
   mode = 0x01;
   break;
   case 8:
   mode = 0x02;
   break;
   case 64:
   mode = 0x03;
   break;
   case 256:
   mode = 0x04;
   break;
   case 1024:
   mode = 0x05;
   break;
   default:
   return;
   }
  if(pin == 5 || pin == 6) {
   TCCR0B = TCCR0B & 0b11111000 | mode;
   }
  else {
   TCCR1B = TCCR1B & 0b11111000 | mode;
   }
 }
 else if(pin == 3 || pin == 11) {
  switch(divisor) {
   case 1:
   mode = 0x01;
   break;
   case 8:
   mode = 0x02;
   break;
   case 32:
   mode = 0x03;
   break;
   case 64:
   mode = 0x04;
   break;
   case 128:
   mode = 0x05;
   break;
   case 256:
   mode = 0x06;
   break;
   case 1024:
   mode = 0x7;
   break;
   default:
   return;
  }
  TCCR2B = TCCR2B & 0b11111000 | mode;
 }
}
    
     
      
