/*
#include <Adafruit_MLX90614.h>//비접촉온도센서 라이브러리
#include <Wire.h>//온도센서 I2C통신
*/
#include <ESP8266WiFi.h>
#include "AnotherIFTTTWebhook.h"
#include <AdafruitIO_WiFi.h>
AdafruitIO_WiFi ioConn("leeinyeong","aio_aOWT60QEmC54nKwfiUFaKBLVRsYd","iptime","");
AdafruitIO_Feed *temptest = ioConn.feed("temptest");
  
void cbFunc1(AdafruitIO_Data *datum)
{
  Serial.printf("adafruit->esp8266//cow_temp:%s\r\n",datum->value());
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  randomSeed(analogRead(1));//randomSeed의 매개변수로 1번 채널(1번 핀)에서 읽은 아날로그 값을 전달
  ioConn.connect();
   Serial.printf("Stand by\r\n");
  
  while(1){
    if(ioConn.status()==AIO_CONNECTED)break;
    else delay(500);
  }

  Serial.printf("Now ready\r\n");
  temptest->onMessage(cbFunc1);//콜백함수등록
  temptest->get();//토픽리스트 받아옴
}

int read_activity_index=0;
int read_cow_temp=0;
void loop() {
 
  read_cow_temp+=1;
  char activity_index[80];
  char cow_temp[80];
  
  snprintf(activity_index,sizeof(activity_index),"%d",read_activity_index);
  snprintf(cow_temp,sizeof(cow_temp),"%d",read_cow_temp);
  
  send_webhook("cow_manager","-tf0NBv4XzZWMKPERFl8D",cow_temp,activity_index,"");
  Serial.printf("esp82660->ifttt//cow_temp:%s\r\n",cow_temp);
  ioConn.run();
  delay(1000);
}
