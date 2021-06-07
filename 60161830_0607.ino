#include <Adafruit_MLX90614.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include "AnotherIFTTTWebhook.h"
#include <AdafruitIO_WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <DHT11.h>

#define mpu_add 0x68 //mpu6050 mpu_add
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
int16_t old_ac_x, old_ac_y, old_ac_z, old_gy_x, old_gy_y, old_gy_z; //acc, gyro data 
double angle = 0, deg, old_angle=0; // angle, deg data
double dgy_x; //double type acc data
char Temp[10], Acz[10], httpADD[100];
int step_num = 0;
unsigned long long time_previous1,time_previous2;
unsigned long long time_current;
int i,mating;
int estrus=0;
int read_activity_index=0;//활동량
int read_cow_temp=0;//소 체온
int read_cow_numer=0;// 소 개체번호
float humi,temp;
char dhttemp[80];
char todayweather[80];
char dhthumidity[80];
char cowtemp[80];
char activityindex[80];
char NumberOfEstrus[80];

DHT11 dht11(12);//12번핀
DynamicJsonDocument test(2048);//임시데이터영역
WiFiClient tcpClient;
PubSubClient mqttClient;
HTTPClient myClient;
AdafruitIO_WiFi ioConn("leeinyeong","aio_jCCf85H7Dh73PkzXdXFiCVZvZoZx","wifitime","a5641078");
AdafruitIO_Feed *todaytempfeed = ioConn.feed("todaytemp");
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void cbFunc1(AdafruitIO_Data *datum)
{
  //Serial.printf("adafruit->esp8266//cow_temp:%s\r\n",datum->value());
}

void setup() {
  Wire.begin(4,5);
  Wire.beginTransmission(mpu_add);
  Wire.write(0x6B);
  Wire.write(0x01); //슬립모드 해제
  Wire.endTransmission(true);
  
  /*full scale range 16g*/
  Wire.beginTransmission(mpu_add);
  Wire.write(0x1C);
  Wire.write(0x18);
  Wire.endTransmission(true);
  
  /*full scale range 2000'/s*/
  Wire.beginTransmission(mpu_add);
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
}
void loop() {
  time_current=millis();
  Wire.beginTransmission(mpu_add);
  Wire.write(0x3B);
  Wire.endTransmission();
  Wire.requestFrom(mpu_add,14,true);
  
  AcX=Wire.read()<<8|Wire.read();
  AcY=Wire.read()<<8|Wire.read();
  AcZ=Wire.read()<<8|Wire.read();
  Tmp=Wire.read()<<8|Wire.read();
  GyX=Wire.read()<<8|Wire.read();
  GyY=Wire.read()<<8|Wire.read();
  GyZ=Wire.read()<<8|Wire.read();
  
  deg = atan2(AcX,AcZ) * 180 / PI;  //rad to deg
  
  dgy_x = GyY / 131.;  //16-bit data to 250 deg/sec

  //angle = (0.95 * (angle + (dgy_x * 0.001))) + (0.05 * deg); //complementary filter
  
 if(time_current-time_previous1>=400)
  {
    time_previous1=time_current;
    
    /*다리를 앞으로 움직이면, 걸음 수를 카운트*/
    if((AcX > -1700) && (AcZ > 1200))
    {
      if((AcX - old_ac_x > 400) && (AcZ - old_ac_z > 1100)) 
      {
        step_num =  step_num + 1;
      }
    }    
    /*기승위 자세 인식*/
    if((AcX > -1400) && (AcZ > 1700))
    {
        mating = mating + 1;        
      if(mating == 3)
        {
          estrus++;
          mating = 0;  
        }
    }

    old_angle = angle;
    old_ac_x = AcX;
    old_ac_z = AcZ;
    Serial.println(step_num);
    Serial.println(AcX);
    Serial.println(AcZ);
  } 
  
  
  myClient.begin("http://api.openweathermap.org/data/2.5/weather?q=gochang&appid=b11f7b1fd11f9ba525deead8cfb52669");
  int getResult=myClient.GET();
  String receivedData;
  const char* detailWeahter;
    if(getResult==HTTP_CODE_OK){
    receivedData =myClient.getString();
    deserializeJson(test,receivedData);//해석
    detailWeahter =test["weather"][0]["description"];
  }
  
  myClient.end();
  
   if((i=dht11.read(humi,temp))==0){}
   
  
 
 if(time_current-time_previous2>=2000){
  
  time_previous2=time_current;
  snprintf(NumberOfEstrus,sizeof(NumberOfEstrus),"%d",estrus);
  snprintf(activityindex,sizeof(activityindex),"%d",step_num);
  snprintf(todayweather,sizeof(todayweather),"%s",detailWeahter);
  snprintf(dhttemp,sizeof(dhttemp),"%.1f",temp);
  snprintf(dhthumidity,sizeof(dhthumidity),"%.1f",humi);
  snprintf(cowtemp,sizeof(cowtemp),"%.2f",mlx.readObjectTempC());
  
  send_webhook("cow_manager1","-tf0NBv4XzZWMKPERFl8D",dhttemp,dhthumidity,todayweather);
  send_webhook("cow_manager2","-tf0NBv4XzZWMKPERFl8D",activityindex,"","");
  send_webhook("cow_manager3","-tf0NBv4XzZWMKPERFl8D",cowtemp,"","");
  ioConn.run();
 }
 }
