#include <string.h>
#include <util/crc16.h>
#include <TinyGPS.h>
TinyGPS gps;

char datastring[100];
unsigned long count;

#define RADIOPIN 9

void setup() {
  Serial.begin(9600);
  pinMode(RADIOPIN,OUTPUT);
  setPwmFrequency(RADIOPIN, 1);
  }

void loop() {
  float flat, flon;                 //defining local variables
  unsigned long fix_age, time, date, alt;
  int year;
  byte month, day, hour, minute, second, hundredths;
  char lat[10], lon[10];
  
  while  (Serial.available()) {
    int c = Serial.read(); 
    if (gps.encode(c))  {
      
      gps.get_datetime(&date, &time, &fix_age);   //retrieve gps date and time and age of last good gps fix
      
      if (fix_age == TinyGPS::GPS_INVALID_AGE){             //check to make sure fix_age is valid suggesting has good gps lock
        snprintf(datastring,sizeof(datastring),"$$MAX,no fix detected");   //transmits an error message
        unsigned int CHECKSUM = gps_CRC16_checksum(datastring); // Calculates the checksum for this datastring
        char checksum_str[6];
        snprintf(checksum_str,sizeof(checksum_str), "*%04X\n", CHECKSUM);
        strcat(datastring,checksum_str);
        rtty_txstring (datastring);  
        count = count +1;    
       }
      
      else {
      gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &fix_age);  //if fix_age is valid then break up time/date into indivual hours, minutes etc.
      
      gps.f_get_position(&flat, &flon , &fix_age);  //gets the latittude and longitude in degrees
      //process lat and lon here
      dtostrf (flat, 9, 6, lat);       //turns latitude and longitude into strings rather than floats so can be transmitted
      dtostrf (flon, 9, 6, lon);
      
      alt= gps.altitude()/100;   //gets gps altitude in cm then divides by 100 to get in m (lose some precision but only need to nearest m
      
      snprintf(datastring,sizeof(datastring),"$$MAX,%lu,%02u:%02u:%02u,%s,%s,%lu",count,hour,minute,second,lat,lon,alt);  //puts together datstring with gps information in standard format
      unsigned int CHECKSUM = gps_CRC16_checksum(datastring); // Calculates the checksum for this datastring
      char checksum_str[6];
      snprintf(checksum_str,sizeof(checksum_str), "*%04X\n", CHECKSUM);
      strcat(datastring,checksum_str);
      rtty_txstring (datastring);
      count = count +1;
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
    
     
      
