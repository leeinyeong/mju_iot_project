/*
#include <Adafruit_MLX90614.h>//비접촉온도센서 라이브러리
#include <Wire.h>//온도센서 I2C통신
*/
#include <ESP8266WiFi.h>
#include "AnotherIFTTTWebhook.h"
#include <AdafruitIO_WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

HTTPClient myClient;
DynamicJsonDocument test(2048);//임시데이터영역
AdafruitIO_WiFi ioConn("leeinyeong","aio_IoAt93xBtDO9jVviZOqNam93NhWp","iptime","");
AdafruitIO_Feed *todaytempfeed = ioConn.feed("todaytemp");
  
void cbFunc1(AdafruitIO_Data *datum)
{
  Serial.printf("adafruit->esp8266//cow_temp:%s\r\n",datum->value());
}

void setup() {
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
}

//DATA TEST

int read_activity_index=0;//활동량
int read_cow_temp=0;//소 체온
int read_cow_numer=0;// 소 개체번호


void loop() {
 
  myClient.begin("http://api.openweathermap.org/data/2.5/weather?q=gochang&appid=b11f7b1fd11f9ba525deead8cfb52669");
  int getResult=myClient.GET();
  String receivedData;
  const char* detailWeahter;
  float temp;
  int humidity;
    if(getResult==HTTP_CODE_OK){
    receivedData =myClient.getString();
    deserializeJson(test,receivedData);//해석
    detailWeahter =test["weather"][0]["description"];
    temp = test["main"]["temp"];
    humidity=test["main"]["humidity"];
  }
  
  myClient.end();

  char todaytemp[80];
  char todayweather[80];
  char todayhumidity[80];
  snprintf(todayweather,sizeof(todaytemp),"%s",detailWeahter);
  snprintf(todaytemp,sizeof(todaytemp),"%f",temp-273.0);
  snprintf(todayhumidity,sizeof(todayhumidity),"%d",humidity);
  
  send_webhook("cow_manager1","-tf0NBv4XzZWMKPERFl8D",todaytemp,"","");
  send_webhook("cow_manager2","-tf0NBv4XzZWMKPERFl8D",todayhumidity,"","");
  send_webhook("cow_manager3","-tf0NBv4XzZWMKPERFl8D",todayweather,"","");
  ioConn.run();
  delay(1000);
}
