#include <gprs.h>     //for gsm 
#include <TinyGPS.h>    //for gps
#include <LiquidCrystal_PCF8574.h>     // for LCD
#include <DHT.h>    // for sensor temp and humi
#include "RTClib.h"     //for timer
#include <SD.h>     //for sd card

#define DHTPIN  2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;
File myFile;
DateTime now;
GPRS gsm;
LiquidCrystal_PCF8574 lcd(0x3F);
TinyGPS gps;

byte   Pin_temt6000 = A1;     //light sensor
byte   Pin_red = 5;   //red led
byte   Pin_green = 6;   //green led
byte   Pin_humidite_sol = A3;   //humidity sol sensor
byte   green = 25;
byte   red = 50;
float  hum, temp;   //for DHT sensor
float  aaa, bbb;    //aaa for light sensor & bbb for humidity sol sensor
unsigned long lecture = 0, time_gps = 0 ;
int    offset_gps = 45;
byte   offset = 30;
byte   h, m, dd, mm;    //for RTC
float  lati,lon;    //for GPS
char   mess[150];
String message;   //message with info to send
char   buf[50];
char   num1[14]="+212659796385";
String ssid ="wildSpirit";
String password="hellsing";
String data;
String server = "www.aractronic.ma";
String uri = "/test/data.php";

//-------commande for Wifi module-----------
void httppost () 
{
  Serial2.println("AT+CIPSTART=\"TCP\",\"" + server + "\",80");
  delay(1000);
  String postRequest =
  "POST " + uri + " HTTP/1.0\r\n" +
  "Host: " + server + "\r\n" +
  "Accept: *" + "/" + "*\r\n" +
  "Content-Length: " + data.length() + "\r\n" +
  "Content-Type: application/x-www-form-urlencoded\r\n" +
  "\r\n" + data;
  String sendCmd = "AT+CIPSEND=";
  Serial2.print(sendCmd);
  Serial2.println(postRequest.length() );
  delay(500);
  if(Serial2.find(">"))
  {
    Serial2.print(postRequest);
    if( Serial2.find("SEND OK")) 
    { 
      while (Serial2.available()) 
      {
        String tmpResp = Serial2.readString();
      }
      Serial2.println("AT+CIPCLOSE");   //close the connection
    }
  }
}

//-----Send data to specific num ,file (sensros.csv) and server---//
void sensors_file(float a, float b, float c, float d, float e, float f)
{
    lcd.clear();
    lcd.print("lat: ");
    lcd.print(String(e,6));
    lcd.setCursor(0,1);
    lcd.print("lon: ");
    lcd.print(String(f,6));
    myFile=SD.open("sensors.csv",FILE_WRITE);   //open sensors.csv for saving info
    myFile.print(now.day());
    myFile.print("/");
    myFile.print(now.month());
    myFile.print("/");
    myFile.print(now.year());
    myFile.print(";");
    myFile.print(now.hour());
    myFile.print(":");
    myFile.print(now.minute());
    myFile.print(":");
    myFile.print(now.second());
    myFile.print(";");
    myFile.print(a);
    myFile.print(";");
    myFile.print(b);
    myFile.print(";");
    myFile.print(c);
    myFile.print(";");
    myFile.print(d);
    myFile.print(";");
    myFile.print(e,6);
    myFile.print(";");
    myFile.print(f,6);
    myFile.println();
    myFile.close();   //close sensors.csv
    sprintf(buf, "%02d/%02d/%04d;%02d:%02d:%02d",now.day(),now.month(),now.year(),now.hour(),now.minute(),now.second());
    message=buf;
    message+=";device:25d5e;";
    message+=String(a);
    message+=";";
    message+=String(b);
    message+=";";
    message+=String(c,2);
    message+=";";
    message+=String(d,2);
    message+=";";
    message+=String(e,6);
    message+=";";
    message+=String(f,6);
    message.toCharArray(mess, message.length()+1);
    gsm.sendSMS(num1,mess);   //send sms with info to the num
    data="value=9";
    httppost();   //for the website
    delay(1000);
    Serial.println(mess);
    lcd.setCursor(0,0);
    lcd.print(h); 
    lcd.write(":");
    lcd.print(m);
    lcd.setCursor(5,0);
    lcd.print(" T: "); 
    lcd.print(temp);
    lcd.print(" C");
    lcd.setCursor(0,1);
    lcd.print(dd);
    lcd.write("/");
    lcd.print(mm);
    lcd.setCursor(5,1);
    lcd.print(" H: "); 
    lcd.print(hum);
    lcd.print(" %");
}

//-------read gps position-------
void device_pos()
{
  bool newdata = false;
    unsigned long start = millis();
    while (millis() - start < 5000)
    {
      if (Serial1.available())
      {
        char c = Serial1.read();
        if (gps.encode(c))    //encode gps received position
        {
          gps.f_get_position(&lati,&lon);   //receive gps pos and put it in lati and lon
          break;
        }
      }
    }
}

//--------reset GSM-----------------
void reset() 
{
  Serial2.println("AT+RST");
  delay(1000);
  Serial2.println("AT+CWMODE=3");
  delay(1000);
}

//---------- to connect wifi to certain ssid-------
void connectWifi() 
{
  String cmd = "AT+CWJAP=\"" +ssid+"\",\"" + password + "\"";
  Serial2.println(cmd);
  delay(4000);
}


void setup() 
{
  Serial.begin(115200);
  Serial1.begin(9600);    //GPS Specifique
  Serial2.begin(115200);    //Wifi specifique
  lcd.begin(16,2);
  Serial.println("Initialisation");
  lcd.clear();
  lcd.print("Initialisation");
  dht.begin();    //DHT initialisation
  delay(500);
  rtc.begin();    //RTC initialisation
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  delay(500);
  sd_card_detection :
  if (!SD.begin(4))    //sd card detection
  {
     lcd.setCursor(6,1);
     lcd.print("No SdCard"); 
     while(1)
     {
       analogWrite(Pin_red, red);
       delay(100);
       analogWrite(Pin_red, 0);
       delay(100);
       goto sd_card_detection;
     }
  }
  gsm_detection :
  gsm.preInit();    //gsm initialisation
  delay(2000);
  while(0 != gsm.init())    //gsm detection
  {
      lcd.setCursor(6,1);
      lcd.print("No SimCard");
      analogWrite(Pin_red, red);
      delay(1000);
      analogWrite(Pin_red, 0);
      goto gsm_detection;
  }
  reset();
  connectWifi();
  lcd.setCursor(6,1);
  lcd.print("Succes");
  delay(500);
  analogWrite(Pin_green, green);
}


void loop() 
{
  if (time_gps+offset_gps < millis()/1000)
  {
    device_pos();
    time_gps=millis()/1000;
  }
  if (lecture+offset < millis()/1000)
  {
    now = rtc.now();    //read time
    hum = dht.readHumidity();
    temp= dht.readTemperature();
    h = now.hour();
    m = now.minute();
    dd= now.day();
    mm= now.month();
    aaa = analogRead(Pin_temt6000);   //read temt6000 info
    bbb = analogRead(Pin_humidite_sol);   //read humidity soil info
    analogWrite(Pin_green, 0);
    analogWrite(Pin_red, red);
    sensors_file(aaa, bbb, hum, temp, lati, lon);
    aaa=0;    //reset the values
    bbb=0;
    hum=0;
    temp=0;
    dd=0;
    mm=0;
    delay(1000);
    analogWrite(Pin_red, 0);
    analogWrite(Pin_green, green);
    lecture=millis()/1000;
  }
}
