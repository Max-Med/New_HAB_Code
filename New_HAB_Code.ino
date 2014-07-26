#include <string.h>
#include <util/crc16.h>
#include <TinyGPS.h>

TinyGPS gps;
#define RADIOPIN 9

void setup() {
  Serial.begin(9600);
  pinMode(RADIOPIN,OUTPUT);
  setPwmFrequency(RADIOPIN, 1);
}

void loop() {
  float flat, flon;
  unsigned long fix_age, time, date, count, alt;
  int year;
  byte month, day, hour, minute, second, hundredths;
  char lat[10], lon[10], datastring[100];
  
  while  (Serial.available()) {
    int c = Serial.read(); 
    if (gps.encode(c))  {
      
      gps.get_datetime(&date, &time, &fix_age);
      
      if (fix_age == TinyGPS::GPS_INVALID_AGE){
        snprintf(datastring,sizeof(datastring),"$$MAX,no fix detected");
      }
      
      else {
      gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &fix_age);
      
      gps.f_get_position(&flat, &flon , &fix_age);
      //process lat and lon here
      dtostrf (flat, 9, 6, lat); 
      dtostrf (flon, 9, 6, lon);
      
      alt= gps.altitude()/100;
      
      snprintf(datastring,sizeof(datastring),"$$MAX,%lu,%02u:%02u:%02u,%s,%s,%lu",count,hour,minute,second,lat,lon,alt);
      count = count + 1;
      }
    }
  }
  unsigned int CHECKSUM = gps_CRC16_checksum(datastring); // Calculates the checksum for this datastring
  char checksum_str[6];
  sprintf(checksum_str, "*%04X\n", CHECKSUM);
  strcat(datastring,checksum_str);
  rtty_txstring (datastring);
  
}
    
     
      
