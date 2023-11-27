#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

Servo servo1;
Servo servo2;
Servo servo3;

struct SERVO{
  Servo* servoPtr;
  int buttonPin;
  bool prv_value;
};

SERVO servos[3];

RF24 radio(10, 9);
const byte address[6] = "00003";

typedef struct {
  bool power[3];
  bool changed;
} DATA;


DATA currentDate;
DATA receivedData;

void setup() {
  Serial.begin(9600);
  radio.begin();

  servo1.attach(7);
  servo2.attach(6);
  servo3.attach(5);

  servos[0].servoPtr = &servo1;
  servos[1].servoPtr = &servo2;
  servos[2].servoPtr = &servo3;

  servos[0].buttonPin = 4;
  servos[1].buttonPin = 3;
  servos[2].buttonPin = 2;

  servos[0].prv_value = false;
  servos[1].prv_value = false;
  servos[2].prv_value = false;


  currentDate = {{0,0,0},0};
  receivedData = {{0,0,0},0};
  //processSwitchData();

  servo1.write(90);
}
void loop() {
  detectButton();
  detectSignal();
  actByData();
}

void detectButton(){
  Serial.println(digitalRead(servos[0].buttonPin));
  for(int i = 0;i<3;i++)
  {
    bool readValue = digitalRead(servos[i].buttonPin);
    if(servos[i].prv_value!=readValue&&readValue==false)
    { 
      currentDate.power[i] = !currentDate.power[i];
      currentDate.changed = true;
    }
    servos[i].prv_value = readValue;
  }
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
void actServo(int i,bool p){
  if(p)
  {
    servos[i].servoPtr->write(110);
    Serial.print(i);
    Serial.println(":turn on");
  }
  else
  {
    servos[i].servoPtr->write(70);
    Serial.print(i);
    Serial.println(":turn off");
  }
}

void actByData() {
  for(int i = 0;i<3;i++)
  {
    actServo(i,currentData.power[i]);
  }
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
