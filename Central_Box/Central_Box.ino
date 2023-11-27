// nRF24 모듈 관련 라이브러
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// 보드 관련 라이브러리
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

// firebase 관련 라이브러리
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// 통신 관련 셋팅
#define WIFI_SSID "와이파이 아이디"
#define WIFI_PASSWORD "와이파이 비빌번호"
#define API_KEY "AIzaSyBy5y3roWIZD6bVH70McUOXtnEUVIMHup8"
#define DATABASE_URL "https://smart-dormitory-protaku-789615-default-rtdb.firebaseio.com/" 

// 장치 주소 상수 정의
const byte AIR_CLEANER_ADDRESS[6] = "00001"; // 공기청정기 주소
const byte HUMIDIFIER_ADDRESS[6] = "00002"; // 가습기 주소
const byte SWITCH_ADDRESS[6] = "00003"; // 스위치 주소

// 데이터 구조체 정의
typedef struct {
  bool power;
  int quality;
  bool changed;
} AirCleanerData; // 공기청정기 데이터 구조체

typedef struct {
  bool power;
  int temp;
  int humidity;
  bool changed;
} HumidifierData; // 가습기 데이터 구조체

typedef struct {
  bool power[3];
  bool changed;
} SwitchData; // 스위치 데이터 구조체

// 각 장치에 대한 데이터를 포함하는 구조체 정의
struct AIR{
  AirCleanerData device;
  AirCleanerData cloude;
}airCleaner;

struct HUM{
  HumidifierData device;
  HumidifierData cloude;
}humidifier;

struct SWI{
  SwitchData device;
  SwitchData cloude;
}switchControl;

SwitchData test_s = {{1,1,1},0};

RF24 radio(10, 9); // RF24 객체 생성 (CE, CSN 핀 번호 지정)

// Firebase 객체
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

// 초기 설정 함수
void setup() {
  Serial.begin(9600); // 시리얼 통신 시작
  radio.begin(); // 라디오 통신 시작

  // 와이파이 통신 시
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else Serial.printf("%s\n", config.signer.signupError.message.c_str());
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // 각 장치에 대한 초기화 수행
  init(AIR_CLEANER_ADDRESS, airCleaner, "Air Cleaner");
  init(HUMIDIFIER_ADDRESS, humidifier, "Humidifier");
  init(SWITCH_ADDRESS, switchControl, "Switch");
}

// 장치 데이터를 초기화하는 템플릿 함수
template<typename T>
void init(const byte address[], T &deviceData, const char* deviceName) {
  // 장치에서 데이터를 가져옵니다.
  get(address, &deviceData.device, sizeof(deviceData.device), deviceName);
  // 장치의 'origin'과 'cloude' 상태를 현재 장치 상태로 초기화(동기화)합니다.
  deviceData.cloude = deviceData.device;
  // 서버에 deviceData.cloude 값 업로드
  toFirebase(deviceData.cloude);
}

// 메인 루프 함수
void loop() {
  //Serial.println(analogRead(A0)); // 아날로그 핀에서 읽은 값을 출력

  // 공기청정기, 가습기, 스위치에 대해 템플릿 함수를 사용하여 업데이트합니다.

  syncData(AIR_CLEANER_ADDRESS, airCleaner, "Air Cleaner");
  syncData(HUMIDIFIER_ADDRESS, humidifier, "Humidifier");
  syncData(SWITCH_ADDRESS, switchControl, "Switch");
  Serial.println("--------------------");
  delay(1000); // 100ms 지연
}

template<typename T>
bool fromFirebase(T &deviceData, const char *deviceName) {
  Serial.println("Unknown Device Type");
  return false;
}
template<>
bool fromFirebase<AirCleanerData>(AirCleanerData &deviceData, const char *deviceName) {
  // Firebase를 사용할 준비가 되지 않았다면, false 반환
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }

  // 기기명으로 DB의 객체에 접근
  // 만약 접근에 실패했다면 에러코드를 출력하며 false 반환
  if (Firebase.RTDB.getJSON(&fbdo, deviceName) == false) {
    Serial.println(fbdo.errorReason());
    return false;
  }
  
  // json으로 저장된 데이터를 센터에 저장하는 과정
  FirebaseJson json = fbdo.jsonObject();
  FirebaseJsonData data;
  json.get(data, "power");
  deviceData.power = data.to<bool>();
  json.get(data, "quality");
  deviceData.quality = data.to<int>();
  return true;
}
template<>
bool fromFirebase<HumidifierData>(HumidifierData &deviceData, const char *deviceName) {
  // Firebase를 사용할 준비가 되지 않았다면, false 반환
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }

  // 기기명으로 DB의 객체에 접근
  // 만약 접근에 실패했다면 에러코드를 출력하며 false 반환
  if (Firebase.RTDB.getJSON(&fbdo, deviceName) == false) {
    Serial.println(fbdo.errorReason());
    return false;
  }
  
  // json으로 저장된 데이터를 센터에 저장하는 과정
  FirebaseJson json = fbdo.jsonObject();
  FirebaseJsonData data;
  json.get(data, "power");
  deviceData.power = data.to<bool>();
  json.get(data, "humidity");
  deviceData.humidity = data.to<int>();
  json.get(data, "temp");
  deviceData.temp = data.to<int>();
  return true;
}
template<>
bool fromFirebase<SwitchData>(SwitchData &deviceData, const char *deviceName) {
  // Firebase를 사용할 준비가 되지 않았다면, false 반환
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }

  // 기기명으로 DB의 객체에 접근
  // 만약 접근에 실패했다면 에러코드를 출력하며 false 반환
  if (Firebase.RTDB.getJSON(&fbdo, deviceName) == false) {
    Serial.println(fbdo.errorReason());
    return false;
  }
  
  // json으로 저장된 데이터를 센터에 저장하는 과정
  FirebaseJson json = fbdo.jsonObject();
  FirebaseJsonData data;
  FirebaseJsonArray ary;
  json.get(data, "power");
  data.get<FirebaseJsonArray>(ary);
  for (size_t i=0 ; i < ary.size() ; i++) {
    ary.get(data, i);
    deviceData.power[i] = data.to<bool>();
  }
  return true;
}

template<typename T>
bool toFirebase(T &deviceData) {
  Serial.println("Unknown Device Type");
  return false;
}
template<>
bool toFirebase<AirCleanerData>(AirCleanerData &deviceData) {
  // Firebase를 사용할 준비가 되지 않았다면, false 반환
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }

  // 센터의 데이터를 json으로 변환하는 과정
  FirebaseJson json;
  json.set("/Air Cleaner/power", deviceData.power);
  json.set("/Air Cleaner/quality", deviceData.quality);
  if (Firebase.RTDB.setJSON(&fbdo, "/", &json) == false) {
    Serial.println(fbdo.errorReason());
    return false;    
  }
  return true;
}
template<>
bool toFirebase<HumidifierData>(HumidifierData &deviceData) {
  // Firebase를 사용할 준비가 되지 않았다면, false 반환
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }

  // 센터의 데이터를 json으로 변환하는 과정
  FirebaseJson json;
  json.set("/Humidifier/power", deviceData.power);
  json.set("/Humidifier/humidity", deviceData.humidity);
  json.set("/Humidifier/temp", deviceData.temp);
  if (Firebase.RTDB.setJSON(&fbdo, "/", &json) == false) {
    Serial.println(fbdo.errorReason());
    return false;    
  }
  return true;
}
template<>
bool toFirebase<SwitchData>(SwitchData &deviceData) {
  // Firebase를 사용할 준비가 되지 않았다면, false 반환
  if (!Firebase.ready() || !signupOK) {
    Serial.println(fbdo.errorReason());
    return false;
  }

  // 센터의 데이터를 json으로 변환하는 과정
  FirebaseJson json;
  for (int i=0 ; i < 3 ; i++)
    json.set("/Switch/power/[" + String(i) + "]", deviceData.power[i]);
  if (Firebase.RTDB.setJSON(&fbdo, "/", &json) == false) {
    Serial.println(fbdo.errorReason());
    return false;    
  }
  return true;
}

template<typename T>
void syncData(const byte address[], T &deviceData, const char* deviceName) {
  
  // 장치에서 최신 데이터를 가져옵니다.
  get(address, &deviceData.device, sizeof(deviceData.device), deviceName);
  /*get : 서버에서 deviceData.cloude로 데이터 가져오기 (deviceName변수값에따라 구분해서 접근해야할듯)*/

  if(deviceData.device.changed) {// 장치 데이터에 변화가 있는 경우
    deviceData.device.changed = false;
    // 장치의 변화를 'cloude'에 반영합니다.
    Serial.println("change detected device");
    deviceData.cloude = deviceData.device;
    // 서버에 deviceData.cloude 값을 업로드
    toFirebase(deviceData.cloude);
  } 
  else if (deviceData.cloude.changed) { // 클라우드 데이터에 변화가 있는 경우
    deviceData.cloude.changed = false;
    // 클라우드의 변화를 장치에 반영합니다.
    Serial.println("change detected cloude");
    fromFirebase(deviceData.cloude, deviceName);
    deviceData.device = deviceData.cloude;
    // 장치에 변화를 적용합니다.
    put(address, &deviceData.device, sizeof(deviceData.device), deviceName);
  } 
  else {// 변경 사항이 없는 경우
    Serial.println("[no changed]");
  }
}

// 특정 주소의 장치로부터 데이터를 받아오는 함수
void get(const byte address[], void* data, size_t dataSize, const char* deviceName){
  setSendMode(address); // 데이터 전송 모드로 설정
  
  Serial.println("get1");
  printDeviceData(&switchControl.device, "Switch"); 
  printDeviceData(&switchControl.cloude, "Switch"); 
  char request[4] = "get"; // 'get' 요청 메시지
  radio.write(&request, sizeof(request)); // 요청 메시지 전송
  //Serial.println(String(deviceName) + " : get : write"); // 로그 출력
  
   Serial.println("get2");
   printDeviceData(&switchControl.device, "Switch"); 
   printDeviceData(&switchControl.cloude, "Switch"); 

  
  //데이터 수신 확인
  if (!receiveDataWithTimeout(address)) {
    Serial.println(String("*") + deviceName + " system is turned off"); // 시스템 꺼짐 메시지 출력
  }
  else {
    radio.read(data, dataSize); // 데이터 수신
    //switchControl.cloude = test_s;
    printDeviceData(data, deviceName); // 데이터 출력
  }
  
  Serial.println("get3");
  printDeviceData(&switchControl.device, "Switch"); 
  printDeviceData(&switchControl.cloude, "Switch"); 
}

// 특정 주소의 장치에 데이터를 전송하는 함수
void put(const byte address[], void* data, size_t dataSize, const char* deviceName){
  setSendMode(address); // 데이터 전송 모드 설정
  char request[4] = "put"; // 'put' 요청 메시지
  radio.write(&request, sizeof(request)); // 요청 메시지 전송
  //Serial.println(String(deviceName) + " : put : write"); // 로그 출력
  delay(100); // 안전을 위한 지연
  radio.write(data, dataSize); // 데이터 전송
  //Serial.println(String(deviceName) + " : data : write"); // 로그 출력
}

// 각 장치 데이터를 업데이트하는 템플릿 함수

// 장치 데이터를 출력하는 함수 -> 추후 LCD에 출력시키는 메소드로 변경하면 됨
void printDeviceData(void* data, const char* deviceName) {
  // 각 장치 타입에 따라 적절한 데이터 형식으로 캐스팅하여 출력
  if (strcmp(deviceName, "Air Cleaner") == 0) {
    AirCleanerData* acData = (AirCleanerData*)data;
    Serial.print("* Power: "); Serial.print(acData->power);
    Serial.print(", Quality: "); Serial.println(acData->quality);
  } 
  else if (strcmp(deviceName, "Humidifier") == 0) {
    HumidifierData* humData = (HumidifierData*)data;
    Serial.print("* Power: "); Serial.print(humData->power);
    Serial.print(", Temp: "); Serial.print(humData->temp);
    Serial.print(", Humidity: "); Serial.println(humData->humidity);
  }
  else if (strcmp(deviceName, "Switch") == 0) {
    SwitchData* swData = (SwitchData*)data;
    Serial.print("* Switch States: ");
    for (int i = 0; i < 3; i++) {
      Serial.print(swData->power[i]); Serial.print(" ");
    }
    Serial.print(swData->changed);
    Serial.println();
  }
}

// 타임아웃을 설정하여 데이터 수신 여부를 확인하는 함수
bool receiveDataWithTimeout(const byte address[]) {
  unsigned long startTime = millis(); // 시작 시간 기록
  setReceiveMode(address); // 데이터 수신 모드 설정
  // 0.5초 동안 데이터 수신 대기
  while (millis() - startTime < 500) { 
    if (radio.available()) {
      return true; // 데이터 수신 성공
    }
  }
  return false; // 타임아웃 발생
}

// 데이터 전송 모드 설정 함수
void setSendMode(const byte address[]){
  radio.openWritingPipe(address); // 쓰기 파이프 열기
  radio.setPALevel(RF24_PA_MIN); // 전력 수준 설정
  radio.stopListening(); // 수신 중단
  //Serial.println("Send mode set"); // 로그 출력
}

// 데이터 수신 모드 설정 함수
void setReceiveMode(const byte address[]){
  radio.openReadingPipe(0, address); // 읽기 파이프 열기
  radio.setPALevel(RF24_PA_MIN); // 전력 수준 설정
  radio.startListening(); // 수신 시작
  //Serial.println("Receive mode set"); // 로그 출력
}