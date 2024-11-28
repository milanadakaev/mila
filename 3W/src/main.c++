#include <EEPROM.h>
#include <Arduino.h>
#include "ChainableLED.h"
#include <DS1307.h>
#include <SD.h>
#include <SPI.h>
#include <BME280I2C.h>
#include <Wire.h>
#include <SoftwareSerial.h>

BME280I2C bme;
DS1307 clock;
ChainableLED leds(7, 8, 1);
SoftwareSerial gps(6,7); // RX, TX

byte button1 = 2; // Set the button1 pin
byte button2 = 3; // Set the button2 pin

bool state1 = 1; // Set button1 state
bool state2 = 1; // Set button2 state
uint8_t tried[5]={0,0,0,0,0};
uint8_t revision=0;

unsigned long buttonPressStartTime1 = 0; // Variable to store button press start time
unsigned long buttonPressStartTime2 = 0; // Variable to store button press start time
unsigned long lastActivityTime = 0;  // Variable pour stocker le dernier moment d'activité

enum Mode {
  NORMAL,
  ECO,
  CONFIG,
  MAINT
};

Mode currentMode = NORMAL; // start in normal mode

Mode previousMode; // store previous mode

struct DateHour {
  int year;
  uint8_t month;
  uint8_t day;
  //char* day_written;
  uint8_t hour;
  uint8_t minute;
  //uint8_t seconde;
};

struct GPS {
  float lat;
  float lon;
};


void blinking(int type){
  // loop
  while(true){
    switch(type)
    {
      // clock
      case 0:
        leds.setColorRGB(0, 255, 0, 0);
        delay(1000);
        leds.setColorRGB(0, 0, 0, 255);
        delay(1000);
        break;
      // GPS
      case 1:
        leds.setColorRGB(0, 255, 0, 0);
        delay(1000);
        leds.setColorRGB(0, 255, 255, 0);
        delay(1000);
        break;
      // sensor access
      case 2:
        leds.setColorRGB(0, 255, 0, 0);
        delay(1000);
        leds.setColorRGB(0, 0, 255, 0);
        delay(1000);
        break;
      // unusual data
      case 3:
        leds.setColorRGB(0, 255, 0, 0);
        delay(1000);
        leds.setColorRGB(0, 0, 255, 0);
        delay(2000);
        break;
      // SD full
      case 4:
        leds.setColorRGB(0, 255, 0, 0);
        delay(1000);
        leds.setColorRGB(0, 255, 255, 255);
        delay(1000);
        break;
      // sd inaccessible
      case 5:
        leds.setColorRGB(0, 255, 0, 0);
        delay(1000);
        leds.setColorRGB(0, 255, 255, 255);
        delay(2000);
        break;
    }
  }
}




GPS getGPSData() {
  GPS data;
  unsigned long startTime = millis(); // store the current time
  String gpsData;

  while (millis() - startTime < EEPROM.read(3)*1000) { // wait for 30 seconds
    if (gps.available() > 0) {
      gpsData += (char)gps.read();
      if (gpsData.endsWith("\r\n")) { // if a line of data is available
        if (gpsData.startsWith("$GPGGA")) {
          // extraction of latitude and longitude
          int commaIndex = gpsData.indexOf(',');
          String latStr = gpsData.substring(commaIndex + 1);
          commaIndex = latStr.indexOf(',');
          latStr = latStr.substring(0, commaIndex);
          String lonStr = gpsData.substring(commaIndex + 1);
          lonStr = lonStr.substring(0, lonStr.indexOf(','));

          // conversion of latitude and longitude to decimal degrees
          data.lat = latStr.substring(0, 2).toFloat() + latStr.substring(2).toFloat() / 60;
          data.lon = lonStr.substring(0, 3).toFloat() + lonStr.substring(3).toFloat() / 60;

          return data;
        }
      }
    }
  }

  // if no data is available after 30 seconds return 0,0
  data.lat = 0.0;
  data.lon = 0.0;
  return data;
}



void reset(){

byte logInterval = 10;
int fileMaxSize = 4096;
byte timeout = 30;
bool LUMIN = 1;
int LUMIN_LOW = 255;
int LUMIN_HIGH = 768;
bool TEMP_AIR = 1;
char MIN_TEMP_AIR = -10;
char MAX_TEMP_AIR = 60;
bool HYGR = 1;
char HYGR_MINT = 0;
char HYGR_MAXT = 50;
bool PRESSURE = 1;
int PRESSURE_MIN = 850;
int PRESSURE_MAX = 1030;

EEPROM.write(0, logInterval);

EEPROM.write(1, highByte(fileMaxSize));
EEPROM.write(2, lowByte(fileMaxSize));

EEPROM.write(3, timeout);

EEPROM.write(22, LUMIN);
EEPROM.write(4, highByte(LUMIN_LOW));
EEPROM.write(5, lowByte(LUMIN_LOW));
EEPROM.write(6, highByte(LUMIN_HIGH));
EEPROM.write(7, lowByte(LUMIN_HIGH));

EEPROM.write(23, TEMP_AIR);
EEPROM.write(8, highByte(MIN_TEMP_AIR));
EEPROM.write(9, lowByte(MIN_TEMP_AIR));
EEPROM.write(10, highByte(MAX_TEMP_AIR));
EEPROM.write(11, lowByte(MAX_TEMP_AIR));

EEPROM.write(12, HYGR);
EEPROM.write(13, highByte(HYGR_MINT));
EEPROM.write(14, lowByte(HYGR_MINT));
EEPROM.write(15, highByte(HYGR_MAXT));
EEPROM.write(16, lowByte(HYGR_MAXT));

EEPROM.write(17, PRESSURE);
EEPROM.write(18, highByte(PRESSURE_MIN));
EEPROM.write(19, lowByte(PRESSURE_MIN));
EEPROM.write(20, highByte(PRESSURE_MAX));
EEPROM.write(21, lowByte(PRESSURE_MAX));

}



DateHour display_date_time() {
  clock.getTime();
  //const char* daysOfWeek[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
  DateHour DateHour;

  // Date 
  //DateHour.day_written = (daysOfWeek[clock.dayOfWeek]-1);
  DateHour.year = (clock.year + 2000);
  DateHour.month = clock.month;
  DateHour.day = clock.dayOfMonth;
  


  // Heure
  DateHour.hour = clock.hour;
  DateHour.minute = clock.minute;
  //DateHour.seconde = clock.second;

  return DateHour;
}




void ecriture_SD(int temp, int hum, int pres, int lightValue, DateHour DateHour, GPS data) {
  // create a new file 
  char filename[15];
  snprintf(filename,15, "%02d%02d%02d_%d.LOG", DateHour.year, DateHour.month, DateHour.day, revision);
  File myFile = SD.open(filename, O_CREAT | O_WRITE);

  if (myFile == 0){
  if (myFile.size() < ((EEPROM.read(1)<<8) | (EEPROM.read(2)))){
    myFile.print("\n");
    if (data.lat == 0.0 && data.lon == 0.0){
      myFile.print("NA");
      tried[4]+= 1;
      if (tried[4] >= 2){
        blinking(1);
        tried[4] = 0;
        myFile.close();
      }
    }else{
    myFile.print(data.lat);
    myFile.print(" ");
    myFile.print(data.lon);
    myFile.print(" ");
    }
    myFile.print(DateHour.year);
    myFile.print("/");
    myFile.print(DateHour.month);
    myFile.print("/");
    myFile.print(DateHour.day);
    myFile.print(" ");
    myFile.print(DateHour.hour);
    myFile.print(":");
    myFile.print(DateHour.minute);
    //myFile.print(":");
    //myFile.print(DateHour.seconde);
    myFile.print(" ");
    if (temp < ((EEPROM.read(8)<<8) | (EEPROM.read(9))) || temp > ((EEPROM.read(10)<<8) | (EEPROM.read(11)))) {
      myFile.print("NA");
      tried[0]+= (temp == -45) ? 0 : 1;
      if (tried[0] >= 2){
        blinking(3);
        tried[0] = 0;
        myFile.close();
      }
    }else{
      myFile.print(temp);
      myFile.print("C°");
      tried[0] = 0;
    }
    if (hum < ((EEPROM.read(13)<<8) | (EEPROM.read(14))) || hum > ((EEPROM.read(15)<<8) | (EEPROM.read(16)))) {
      myFile.print("NA");
      tried[1]+=(hum == -45) ? 0 : 1;
      if (tried[1] >= 2){
        blinking(3);
        tried[1] = 0;
        myFile.close();
      }
    }else{
      myFile.print(hum);
      myFile.print("%");
      tried[1] = 0;
    }
    if (pres < ((EEPROM.read(18)<<8) | (EEPROM.read(19))) || pres > ((EEPROM.read(20)<<8) | (EEPROM.read(21)))) {
      myFile.print("NA");
      tried[2]+= (pres == 100) ? 0 : 1;
      if (tried[2] >= 2){
        blinking(3);
        tried[2] = 0;
        myFile.close();
      }
    }else{
      myFile.print(pres);
      myFile.print("HPa");
      tried[2] = 0;
    }
    if (lightValue < ((EEPROM.read(4)<<8) | (EEPROM.read(5))) || lightValue > ((EEPROM.read(6)<<8) | (EEPROM.read(7)))) {
      myFile.print("NA");
      tried[3]+= (lightValue == -5) ? 0 : 1;
      if (tried[3] >= 2){
        blinking(3);
        tried[3] = 0;
        myFile.close();
      }
    }else{
      myFile.print(lightValue);
      myFile.print("Lum");
      tried[3] = 0;
    }
    myFile.close();
  }else{
    myFile.close();
    revision += 1;

  }
}else{
  blinking(4);
}
}


void lecture_capteur(){
    int8_t temp, hum; 
    uint16_t pres;
    GPS data = getGPSData();

    int lightValue;
      if (EEPROM.read(23) == 1){
        temp = bme.temp();
      }else{
        temp = -45;
      }
      if (EEPROM.read(12) == 1){
        hum = bme.hum();
      }else{
        hum = -45;
      }
      if (EEPROM.read(17) == 1){
        pres = bme.pres();
      }else{
        pres = 100;
      }
      if (EEPROM.read(22) == 1){
        lightValue = analogRead(A0);
      }else{
        lightValue = -5;
      }
    DateHour DateHour;
    DateHour = display_date_time();//display date and time on the serial monitor
    if (currentMode == MAINT){
      Serial.print("\n");
      Serial.print(data.lat);
      Serial.print(",");
      Serial.print(data.lon);
      Serial.print(",");
      Serial.print(DateHour.year);
      Serial.print("/");
      Serial.print(DateHour.month);
      Serial.print("/"); 
      Serial.print(DateHour.day);
      Serial.print(",");
      Serial.print(DateHour.hour);
      Serial.print(":");
      Serial.print(DateHour.minute);
      Serial.print(",");
      Serial.print(temp);
      Serial.print("°C,");
      Serial.print(hum);
      Serial.print("%,");
      Serial.print(pres);
      Serial.print("HPa,");
      Serial.print(lightValue);
      Serial.print("Lum");
    }else{
      if (temp == 0 && hum == 0 && pres == 0 && lightValue == 0){
        blinking(2);
      }
      else{
      ecriture_SD(temp, hum, pres, lightValue, DateHour, data);
    }
    }
    
}

void GreenPress() {
  if (!digitalRead(button1)) {
      buttonPressStartTime1 = millis();
  } else if (digitalRead(button1)){
      if ((millis()-buttonPressStartTime1) > 5000) {
      if (currentMode == NORMAL) {
      currentMode = ECO;
      buttonPressStartTime1 = 0;
      } else if (currentMode == ECO) {
        currentMode = NORMAL;
        buttonPressStartTime1 = 0;
      }else{}
    }
  }
}


//function to check if the button is pressed for more than 5 seconds
void RedPress() {
  if (!digitalRead(button2)) {
    buttonPressStartTime2 = millis();
  } else if (digitalRead(button2)) {
    if ((millis() - buttonPressStartTime2) > 5000) {
      if (currentMode == NORMAL || currentMode == ECO) {
        previousMode = currentMode; //store the current mode
        currentMode = MAINT;
        buttonPressStartTime2 = 0;
      } else if (currentMode == MAINT) {
        currentMode = previousMode; // restore the previous mode
        buttonPressStartTime2 = 0;
      }
    }
  }
}

void setup() {
  leds.init();
  leds.setColorRGB(0, 50, 0, 50);
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  state1 = digitalRead(button1);
  state2 = digitalRead(button2);

  Serial.begin(9600);
  gps.begin(9600); 

  Wire.begin();
  clock.begin();
    /*clock.fillByYMD(2023, 10, 24); //Jan 19,2013
    clock.fillByHMS(13, 39, 30); //15:28 30"
    clock.fillDayOfWeek(TUE);//Saturday
    clock.setTime();//write time to the RTC chip*/

  //check if the RTC clock is working and if the sensor is connected and if the SD card is connected and if the GPS is connected 

  if (!bme.begin() || analogRead(A0)>100){
    blinking(2);
  }else if (!SD.begin(4)){
    blinking(5);
  }else if(gps.available()==0){
    //blinking(1);
  }


  pinMode(10, OUTPUT); //Define the pin 10 as output


  attachInterrupt(digitalPinToInterrupt(button1), GreenPress, CHANGE);
  attachInterrupt(digitalPinToInterrupt(button2), RedPress, CHANGE);

  if (state2 == LOW) {
        currentMode = CONFIG;
}

}






void loop() {

  long currentMillis = millis();

  switch (currentMode) {

    case NORMAL:

      leds.setColorRGB(0,0,255,0);
      if (currentMillis - lastActivityTime >= 5000){
      lastActivityTime = currentMillis;

      lecture_capteur();

      }
      break;//End of NORMAL mode


    case ECO:

      leds.setColorRGB(0,0,0,255);
      if (currentMillis - lastActivityTime >= 10000){
      lastActivityTime = currentMillis;

      lecture_capteur();
      }

      break; //End of ECO mode


    case MAINT:
    //Leds in yellow

    leds.setColorRGB(0, 255, 255, 0);
      if (currentMillis - lastActivityTime >= 1000){
      lastActivityTime = currentMillis;
      

      lecture_capteur();

      }


    break; //End of MAINT mode




    case CONFIG:
            leds.setColorRGB(0, 255, 0, 0);
            while ((millis() - lastActivityTime) < (EEPROM.read(3)*1000)) {
              if (Serial.available() > 0) {
                // L'activité est détectée, mettez à day lastActivityTime
                lastActivityTime = millis();

                
                String commande = Serial.readString();
                Serial.println(commande);
                // Maintenant, vous pouvez traiter les commandes ici
                if (commande.startsWith("LOG_INTERVAL=")) {
                  // Exemple de traitement de la commande LOG_INTERVAL
                  byte logInterval = commande.substring(12).toInt();
                  EEPROM.write(0, logInterval);

                } else if (commande.startsWith("FILE_MAX_SIZE=")) {
                  // Exemple de traitement de la commande FILE_MAX_SIZE
                  int fileMaxSize = commande.substring(13).toInt();
                  EEPROM.write(1, highByte(fileMaxSize));
                  EEPROM.write(2, lowByte(fileMaxSize));

                } else if (commande.startsWith("RESET")) {
                  // Réinitialisation de l'EEPROM aux valeurs par défaut          
                  reset();    
                } else if (commande.startsWith("VERSION")) {
                  // Affichez la version du programme et le numéro de lot
                  Serial.println("1.0");
                }else if (commande.startsWith("LUMIN=")) {
                  bool LUMIN = commande.substring(5).toInt();
                  EEPROM.write(22, LUMIN);

                }else if (commande.startsWith("TIMEOUT=")) {
                  bool TIMEOUT = commande.substring(8).toInt();        
                  EEPROM.write(3, TIMEOUT);   
                }else if (commande.startsWith("LUMIN_LOW=")&& commande.substring(10).toInt()>0 && commande.substring(10).toInt()<1024) {
                  int LUMIN_LOW = commande.substring(10).toInt();
                  EEPROM.write(4, highByte(LUMIN_LOW));
                  EEPROM.write(5, lowByte(LUMIN_LOW));

              }else if (commande.startsWith("LUMIN_HIGH=")&& commande.substring(11).toInt()>0 && commande.substring(11).toInt()<1024) {
                  int LUMIN_HIGH = commande.substring(11).toInt();
                  EEPROM.write(6, highByte(LUMIN_HIGH));
                  EEPROM.write(7, lowByte(LUMIN_HIGH));
              }else if (commande.startsWith("TEMP_AIR=")) {
                  bool TEMP_AIR = commande.substring(9).toInt();
                  EEPROM.write(23, TEMP_AIR);
              }else if (commande.startsWith("MIN_TEMP_AIR=") && commande.substring(13).toInt()>-40 && commande.substring(13).toInt()<85) {
                  int MIN_TEMP_AIR = commande.substring(13).toInt();
                  EEPROM.write(8, highByte(MIN_TEMP_AIR));
                  EEPROM.write(9, lowByte(MIN_TEMP_AIR));
              }else if (commande.startsWith("MAX_TEMP_AIR=")&& commande.substring(13).toInt()>-40 && commande.substring(13).toInt()<85) {
                  int MAX_TEMP_AIR = commande.substring(13).toInt();
                  EEPROM.write(10, highByte(MAX_TEMP_AIR));
                  EEPROM.write(11, lowByte(MAX_TEMP_AIR));
              }else if (commande.startsWith("HYGR=")) {
                  bool HYGR = commande.substring(5).toInt();
                  EEPROM.write(12, HYGR);
              }else if (commande.startsWith("HYGR_MINT=")&& commande.substring(10).toInt()>40 && commande.substring(10).toInt()<85) {
                  int HYGR_MINT = commande.substring(10).toInt();
                  EEPROM.write(13, highByte(HYGR_MINT));
                  EEPROM.write(14, lowByte(HYGR_MINT));
              }else if (commande.startsWith("HYGR_MAXT=")&& commande.substring(10).toInt()>40 && commande.substring(10).toInt()<85) {
                int HYGR_MAXT = commande.substring(10).toInt();
                EEPROM.write(15, highByte(HYGR_MAXT));
                EEPROM.write(16, lowByte(HYGR_MAXT));
              }else if (commande.startsWith("PRESSURE=")) {
                bool PRESSURE = commande.substring(9).toInt();
                EEPROM.write(17, PRESSURE);
              }else if (commande.startsWith("PRESSURE_MIN=")&& commande.substring(13).toInt()>300 && commande.substring(13).toInt()<1100) {
                int PRESSURE_MIN = commande.substring(13).toInt();
                EEPROM.write(18, highByte(PRESSURE_MIN));
                EEPROM.write(19, lowByte(PRESSURE_MIN));
              }else if (commande.startsWith("PRESSURE_MAX=")&& commande.substring(13).toInt()>300 && commande.substring(13).toInt()<1100) {
                int PRESSURE_MAX = commande.substring(13).toInt();
                EEPROM.write(20, highByte(PRESSURE_MAX));
                EEPROM.write(21, lowByte(PRESSURE_MAX));
              }else if (commande.startsWith("CLOCK=")) {
                // Exemple de traitement de la commande CLOCK
                // Récupérez les valeurs de de l'heure, des minutes et des secondes
                uint8_t heure = commande.substring(6, 8).toInt();
                uint8_t minute = commande.substring(9, 11).toInt();
                uint8_t seconde = commande.substring(12, 14).toInt();
                clock.fillByHMS(heure, minute, seconde);
              }else if (commande.startsWith("DATE=")) {
                // Exemple de traitement de la commande DATE
                // Récupérez les valeurs de l'année, du month et du day
                int year = commande.substring(5, 9).toInt();
                uint8_t month = commande.substring(10, 12).toInt();
                uint8_t day = commande.substring(13, 15).toInt();
                clock.fillByYMD(year, month, day);
                
              }else if (commande.startsWith("DAY=")) {
                const char* daysOfWeek[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
                // Exemple de traitement de la commande DAY
                // Récupérez la valeur du day de la semaine
                String day = commande.substring(4, 7);
                //récupérer le rang du day de la semaine dans le tableau daysOfWeek et le stocker dans la variable day (0 = Monday)
                uint8_t nbr_day = 0;
                for (int i = 0; i < 7; i++) {if (strcmp(day.c_str(), daysOfWeek[i]) == 0) {nbr_day = i;}}
                clock.fillDayOfWeek(nbr_day);
              }
                

            }
            }
            currentMode = NORMAL;
            Serial.println("Changement de mode automatique vers NORMAL");
    break;//End of CONFIG mode

}
}