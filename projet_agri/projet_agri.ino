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
byte   Pin_humidite_sol = A3;
byte   green = 25;
byte   red = 50;
float  hum, temp;
float  aaa, bbb;
unsigned long lecture = 0, time_gps = 0 ;
int    offset_gps = 45;
byte   offset = 30;
byte   h, m, dd, mm;
float  lati,lon;
char   mess[150];
String message;
char   buf[50];
char   num1[14]="+212659796385";
String ssid ="wildSpirit";
String password="hellsing";
String data;
String server = "www.aractronic.ma";
String uri = "/test/data.php";

void lcdPrint2Digits(uint8_t n)
{
  if (n < 10) lcd.print("0");
  lcd.print(n);
}

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
    lcdPrint2Digits(h); 
    lcd.write(":");
    lcdPrint2Digits(m);
    lcd.setCursor(5,0);
    lcd.print(" T: "); 
    lcd.print(temp);
    lcd.print(" C");
    lcd.setCursor(0,1);
    lcdPrint2Digits(dd);
    lcd.write("/");
    lcdPrint2Digits(mm);
    lcd.setCursor(5,1);
    lcd.print(" H: "); 
    lcd.print(hum);
    lcd.print(" %");
}

void setup() 
{
  Serial.begin(115200);
  Serial1.begin(9600);//GPS Specifique
  Serial2.begin(115200);//Wifi specifique
  lcd.begin(16,2);
  lcd.setBacklight(255);
  Serial.println("Initialisation");
  lcd.clear();
  lcd.print("Initialisation");
  dht.begin();
  delay(500);
  rtc.begin();
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  delay(500);
  if (!SD.begin(4))    //sd card detection
  {
     lcd.setCursor(6,1);
     lcd.print("failed SD ");
     while(1)
     {
       analogWrite(Pin_red, red);
       delay(100);
       analogWrite(Pin_red, 0);
       delay(100);
     }
  }
  gsm.preInit();    //gsm starter
  delay(2000);
  while(0 != gsm.init())    //gsm detection
  {
      lcd.setCursor(6,1);
      lcd.print("SimCard NO");
      analogWrite(Pin_red, red);
      delay(1000);
      analogWrite(Pin_red, 0);
  }
  reset();
  connectWifi();
  lcd.setCursor(6,1);
  lcd.print("succes    ");
  delay(500);
  analogWrite(Pin_green, green);
}

void reset() 
{
  Serial2.println("AT+RST");
  delay(1000);
  Serial2.println("AT+CWMODE=3");
  delay(1000);
}

void connectWifi() 
{
  String cmd = "AT+CWJAP=\"" +ssid+"\",\"" + password + "\"";
  Serial2.println(cmd);
  delay(4000);
}

void loop() 
{
  if (time_gps+offset_gps < millis()/1000)
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
          gps.f_get_position(&lati,&lon);   //receive gps position
          break;
        }
      }
    }
    time_gps=millis()/1000;
  }
  if (lecture+offset < millis()/1000)
  {
    now = rtc.now();
    hum = dht.readHumidity();
    temp= dht.readTemperature();
    h = now.hour();
    m = now.minute();
    dd= now.day();
    mm= now.month();
    aaa = analogRead(Pin_temt6000);
    bbb = analogRead(Pin_humidite_sol);
    analogWrite(Pin_green, 0);
    analogWrite(Pin_red, red);
    sensors_file(aaa, bbb, hum, temp, lati, lon);
    aaa=0;
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

//commande at sim800
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
      Serial2.println("AT+CIPCLOSE");
    }
  }
}
