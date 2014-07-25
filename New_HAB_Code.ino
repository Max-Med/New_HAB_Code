#include <TinyGPS.h>
TinyGPS gps;
float flat, flon;
unsigned long fix_age, time, date, count, alt;
int year;
byte month, day, hour, minute, second, hundredths;
char lat[10], lon[10], datastring[100];


void setup() {
  Serial.begin(9600);
}

void loop() {
  while  (Serial.available()) {
    int c = Serial.read(); 
    if (gps.encode(c))  {
      gps.get_datetime(&date, &time, &fix_age);
      gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &fix_age);
      
      gps.f_get_position(&flat, &flon , &fix_age);
      //process lat and lon here
      dtostrf (flat, 9, 6, lat); 
      dtostrf (flon, 9, 6, lon);
      
      alt= gps.altitude()/100;
      
      if (fix_age == TinyGPS::GPS_INVALID_AGE){
        snprintf(datastring,sizeof(datastring),"$$MAX,no fix detected");
      }
      else {
        snprintf(datastring,sizeof(datastring),"$$MAX,%u,%u:%u:&u,%s,%s,%u",count,hour,minute,second,lat,lon,alt);
        count = count + 1;
      }
      //radio + checksum stuff goes in here
    }
  }
}
    
     
      
