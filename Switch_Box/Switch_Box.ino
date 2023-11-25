#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

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

// 초기 설정 함수
void setup() {
  Serial.begin(9600); // 시리얼 통신 시작
  radio.begin(); // 라디오 통신 시작
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
  /*put : 서버에 deviceData.cloude 값 업로드 (deviceName변수값에따라 구분해서 접근해야할듯)*/
}

// 메인 루프 함수
void loop() {
  //Serial.println(analogRead(A0)); // 아날로그 핀에서 읽은 값을 출력

  // 공기청정기, 가습기, 스위치에 대해 템플릿 함수를 사용하여 업데이트합니다.

  updateDeviceData(AIR_CLEANER_ADDRESS, airCleaner, "Air Cleaner");
  updateDeviceData(HUMIDIFIER_ADDRESS, humidifier, "Humidifier");
  updateDeviceData(SWITCH_ADDRESS, switchControl, "Switch");
  Serial.println("--------------------");
  delay(1000); // 100ms 지연
}

template<typename T>
void updateDeviceData(const byte address[], T &deviceData, const char* deviceName) {
  
  // 장치에서 최신 데이터를 가져옵니다.
  get(address, &deviceData.device, sizeof(deviceData.device), deviceName);
  /*get : 서버에서 deviceData.cloude로 데이터 가져오기 (deviceName변수값에따라 구분해서 접근해야할듯)*/

  if(deviceData.device.changed) {// 장치 데이터에 변화가 있는 경우
    deviceData.device.changed = false;
    // 장치의 변화를 'cloude'에 반영합니다.
    Serial.println("change detected device");
    deviceData.cloude = deviceData.device;
    /*put : 서버에 deviceData.cloude 값을 업로드 (deviceName변수값에따라 구분해서 접근해야할듯)*/
  } 
  else if (deviceData.cloude.changed) { // 클라우드 데이터에 변화가 있는 경우
    deviceData.cloude.changed = false;
    // 클라우드의 변화를 장치에 반영합니다.
    Serial.println("change detected cloude");
    deviceData.device = deviceData.cloude;
    put(address, &deviceData.device, sizeof(T), deviceName);
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
