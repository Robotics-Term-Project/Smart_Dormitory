#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(10, 9);
const byte address[6] = "00003";

typedef struct {
  bool power;
  int quality;
  bool changed;
} DATA;

DATA currentDate;
DATA receivedData;

int button = 7;//임의로 7로 정함
int fan = 6;//임의로 6로 정함

void setup() {
  Serial.begin(9600);
  radio.begin();
}
void loop() {
  detectButton();
  detectSignal();
  actByData();
}

bool prv_value
void detectButton(){
  bool readValue = digitalRead(button);
  if(prv_value!=readValue&&readValue==false)
  {
    currentDate.power = !currentDate.power;
    currentDate.changed = true;
  }
  prv_value = readValue;
}
void detectSignal(){
  setReceiveMode();
  if (radio.available()) {
    Serial.println("Data received");
    char request[4];
    radio.read(&request, sizeof(request));
    if(strcmp(request, "get")==0)
    {
      delay(500);
      setSendMode();
      radio.write(&currentDate, sizeof(DATA));
      currentDate.changed = false;
    }
    else if(strcmp(request, "put")==0)
    {
      setReceiveMode();
      radio.read(&receivedData, sizeof(DATA));
      //if(receivedData.changed)currentData=receivedData;
      currentData=receivedData;
    }
  }
}
void actPower(int i,bool p){
  /*fan*/
  digitalWrite(fan, p);
}

void actByData() {
  actPower(i,currentData.power[i]);
  Serial.println("-----");
}
void setSendMode(){
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  //Serial.println("Send mode set");
}

void setReceiveMode(){
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  //Serial.println("Receive mode set");
}
