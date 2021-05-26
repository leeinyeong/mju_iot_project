
/*
 esp8266에서 처리할 데이터를 Adafruit I/O에 HTTP로 송수신, 
 Dashboard 시각화 UI 설계, 
 IFTTT에서 휴대전화 알림 및 선풍기 세기 제어, 
 하드웨어 제작
 IFTTT<->Adafruit<->esp8266 
 비접촉온도센서 데이터 가공
 */

#include <AdafruitIO_WiFi.h>//Adafruit 라이브러리 MQTT Broker
#include <Adafruit_MLX90614.h>//비접촉온도센서 라이브러리
#include <Wire.h>//온도센서 I2C통신
#include "AnotherIFTTTWebhook.h"//IFTTT 라이브러리

AdafruitIO_WiFi ioConn("leeinyeong","aio_Mdnb20LKt3olNjh3aG2xwHiTYivc","wifitime","a5641078");
AdafruitIO_Feed *test = ioConn.feed("temptest")


void setup() {
  
}
void loop() {

}
