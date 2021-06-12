#include <Adafruit_MLX90614.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include "AnotherIFTTTWebhook.h"
#include <AdafruitIO_WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <DHT11.h>

#define MPU_ADDR 0x68
unsigned long long time_previous1,time_previous2,time_previous3,time_previous4;
unsigned long long time_current;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
int16_t old_ac_x, old_ac_y, old_ac_z, old_gy_x, old_gy_y, old_gy_z;
double dgy_x; //double type acc
int step_num = 0;// 소 활동지수
int estrus=0;//소 발정지수
int i;// dht 센서 변수
int mating;// 발정지수 딜레이용
float humi,temp;// dht 습도, 온도
char dhttemp[80];// dht 온도 문자형
char todayweather[80];// api 날씨 문자형
char dhthumidity[80];// dht 습도 문자형
char cowtemp[80];// 소 체온 문자형
char activityindex[80];// 활동지수 문자형
char NumberOfEstrus[80];// 발정지수 문자형
int count = 0;
DHT11 dht11(12);//12번핀
DynamicJsonDocument test(2048);//임시데이터영역
WiFiClient tcpClient;
PubSubClient mqttClient;
HTTPClient myClient;
AdafruitIO_WiFi ioConn("leeinyeong","aio_pIQb38IW5ZCLawxDCOAku3hLB2lX","w-5360333-3","");
AdafruitIO_Feed *todaytempfeed = ioConn.feed("todaytemp");
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void cbFunc1(AdafruitIO_Data *datum)
{
  //Serial.printf("adafruit->esp8266 디버그용도 cow_temp:%s\r\n",datum->value());
}

void setup() {
  Wire.begin(4,5);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x01); //슬립모드 해제
  Wire.endTransmission(true);
  
  /*full scale range 16g*/
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);
  Wire.write(0x18);
  Wire.endTransmission(true);
  
  /*full scale range 2000'/s*/
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1B);
  Wire.write(0x18);
  Wire.endTransmission(true);
  
  Serial.begin(115200);
  delay(1000);
  ioConn.connect();
  Serial.printf("Stand by\r\n");
  while(1){
    if(ioConn.status()==AIO_CONNECTED)break;
    else delay(500);
  }
  Serial.printf("Now ready\r\n");
  todaytempfeed->onMessage(cbFunc1);//콜백함수등록
  todaytempfeed->get();//토픽리스트 받아옴
  mlx.begin();//적외선온도센서
  time_previous1=millis();  
  time_previous2=millis();
  time_previous3=millis();
  time_previous4=millis();
}

void loop() {
  time_current=millis();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission();
  Wire.requestFrom(MPU_ADDR,14,true);
  
  AcX=Wire.read()<<8|Wire.read();
  AcY=Wire.read()<<8|Wire.read();
  AcZ=Wire.read()<<8|Wire.read();
  Tmp=Wire.read()<<8|Wire.read();
  GyX=Wire.read()<<8|Wire.read();
  GyY=Wire.read()<<8|Wire.read();
  GyZ=Wire.read()<<8|Wire.read();
  
  //deg = atan2(AcX,AcZ) * 180 / PI;  //rad to deg
  //dgy_x = GyY / 131.;  //16-bit data to 250 deg/sec
  //angle = (0.95 * (angle + (dgy_x * 0.001))) + (0.05 * deg); //complementary filter 
 
 if(time_current-time_previous1>=400)
  {
    count = count + 1;
    time_previous1=time_current;
    step_num = 0;
    estrus=0;
    /*다리를 앞으로 움직이면, 걸음 수를 카운트*/
    if((AcX > -1700) && (AcZ > 1200))
    {
      if((AcX - old_ac_x > 400) && (AcZ - old_ac_z > 1100)) 
      {
        step_num = 1;
        snprintf(activityindex,sizeof(activityindex),"%d",step_num);
        send_webhook("cow_manager2","-tf0NBv4XzZWMKPERFl8D",activityindex,"","");
      }
    }
      else
      {
        snprintf(activityindex,sizeof(activityindex),"%d",step_num);
        send_webhook("cow_manager2","-tf0NBv4XzZWMKPERFl8D",activityindex,"","");
      }
    /*기승위 자세 인식*/
    if((AcX > -1400) && (AcZ > 1700))
    {
      mating = mating + 1;        
        if(mating == 3)
        {
          estrus=1;
          mating = 0;
          snprintf(NumberOfEstrus,sizeof(NumberOfEstrus),"%d",estrus);
          send_webhook("cow_manager4","dva3UlcOjED86GO3-YD23T",NumberOfEstrus,"",""); 
        } 
    }
    else
    {
      snprintf(NumberOfEstrus,sizeof(NumberOfEstrus),"%d",estrus);
      send_webhook("cow_manager4","dva3UlcOjED86GO3-YD23T",NumberOfEstrus,"",""); 
    }
    //old_angle = angle;
    old_ac_x = AcX;
    old_ac_z = AcZ;
    //Serial.println(step_num);
    //Serial.println(AcX);
    //Serial.println(AcZ);
  } 
  myClient.begin("http://api.openweathermap.org/data/2.5/weather?q=gochang&appid=b11f7b1fd11f9ba525deead8cfb52669");
  int getResult=myClient.GET();
  String receivedData;
  const char* detailWeahter;
  if(getResult==HTTP_CODE_OK)
  {
    receivedData =myClient.getString();
    deserializeJson(test,receivedData);//해석
    detailWeahter =test["weather"][0]["description"];

    char* weather_data = (char*)detailWeahter;//send_webhook 함수에 const char*가 불가능 해서 타입을 바꿔줌
   if(time_current-time_previous4>=5000)
    {
      time_previous4=time_current;
      weather_data = "Snow";
      if(strcmp(weather_data,"Snow")==0)
      {
        Serial.print("현재 눈이 내리고 있습니다.\r\n");
        send_webhook("weather", "0dzpHnFyxhAziHqOzRCPQ", weather_data, "","");
      }
      else if(strcmp(weather_data,"Rain")==0)
      {
        Serial.print("현재 비가 내리고 있습니다.\r\n");   
        send_webhook("weather", "0dzpHnFyxhAziHqOzRCPQ", weather_data, "","");
      }
    }
  }
  myClient.end();
  if((i=dht11.read(humi,temp))==0){}
  if(count == 20)
  {
      if((mlx.readObjectTempC() > 40) || (mlx.readObjectTempC() < 35))
    {
      send_webhook("cowtemp", "0dzpHnFyxhAziHqOzRCPQ", cowtemp, "","");
    }
    if((temp > 50) || (humi <  20) )
    {
      send_webhook("fire_warning", "0dzpHnFyxhAziHqOzRCPQ", dhttemp, dhthumidity,"");
    }
    count= 0;
  } 
 if(time_current-time_previous2>=3000)
 {
  time_previous2=time_current;
  snprintf(todayweather,sizeof(todayweather),"%s",detailWeahter);
  snprintf(dhttemp,sizeof(dhttemp),"%.1f",temp);
  snprintf(dhthumidity,sizeof(dhthumidity),"%.1f",humi);
  send_webhook("cow_manager1","-tf0NBv4XzZWMKPERFl8D",dhttemp,dhthumidity,todayweather);
 }
 if(time_current-time_previous3>=10000)
 {
  time_previous3=time_current;
  snprintf(cowtemp,sizeof(cowtemp),"%.2f",mlx.readObjectTempC()); 
  send_webhook("cow_manager3","-tf0NBv4XzZWMKPERFl8D",cowtemp,"","");
 }
 ioConn.run();
 }
