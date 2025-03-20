#include <L298NX2.h>
#include <Wire.h> 
#include <esp_sleep.h>
#include <ESP32Time.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#pragma region Define
#define SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define SENSOR_HUMIDITY_CHARACTERISTIC_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"
#define NEED_CHARACTERISTIC_UUID "19b10002-e8f2-537e-4f6c-d104768a1214"

#define ENA  6      // Motors
#define IN1A 7     //    6, 41, 40 -> Left
#define IN2A 8     //    7, 50, 51 -> Right
#define IN1B 10
#define IN2B 20
#define ENB 9

#define ECHO 4     //HM-40
#define TRIG 5

#define LIGHTLA 3 //Light sensors
#define LIGHTRA 2

#define HUM 21 // Humidity sensor
#define WATER 0
#pragma endregion

#pragma region Constants
const int LIGHT_MARGIN = 10;      
const int HUMIDITY_MAX = 20;     
const int DISTANCE_MIN = 40;         
#pragma endregion

#pragma region Variables
int lightSensorR, lightSensorL,lightL, lightR, year, month, date, hour, minute, second,  plantLightNeeds, beginNightH, endNightH, dayWater;; // Operating values
double light, dist, humidity; // Final values
long timeInfraredSensor = 0;
bool flagHumidity, flagConfiguration, flagTurn, flagSleep = false;
bool flagDay, flagNeedWater = true;
int cont = 0;
String config;
#pragma endregion

ESP32Time rtc; 



L298NX2 motors(ENA, IN1A, IN2A, ENB, IN1B, IN2B);

#pragma region BLE
BLEServer* pServer = NULL;
BLECharacteristic* pSensorHumidityCharacteristic = NULL;
BLECharacteristic* pNeedCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
#pragma endregion


void logicRobot(){
 
  getLight();
  getHumidity(); 
  getDistance(); 
  movementLogic();
}

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* onCharacteristic) {
    
    String nValue = onCharacteristic->getValue();
    
    if (nValue.length() > 0 ){
      config += static_cast<int>(nValue[0]);
      cont ++;
      Serial.println(static_cast<int>(nValue[0]));
    }

    if (cont == 11){
      plantLightNeeds = config.substring(0,2).toInt();
      dayWater = config.substring(2,3).toInt();
      beginNightH = config.substring(3,5).toInt();
      endNightH = config.substring(5,7).toInt();
      rtc.setTime(config.substring(7,9).toInt(),config.substring(9,11).toInt(), 
                  config.substring(11,13).toInt(),config.substring(13,15).toInt(),
                  config.substring(15,17).toInt(),config.substring(17).toInt()); 
      flagConfiguration = true;
    }
  }
};


void setup(){
  Serial.begin(115200);

  BLEDevice::init("ESP32");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pSensorHumidityCharacteristic = pService->createCharacteristic(
                      SENSOR_HUMIDITY_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // Create the configuration Characteristic
  pNeedCharacteristic = pService->createCharacteristic(
                      NEED_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pNeedCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
 
  pSensorHumidityCharacteristic->addDescriptor(new BLE2902());
  pNeedCharacteristic->addDescriptor(new BLE2902());

  pService->start();


  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");

  motors.setSpeed(100);

  pinMode(LIGHTLA , INPUT);
  pinMode(LIGHTRA , INPUT);
  pinMode(HUM , INPUT);
  pinMode(TRIG , OUTPUT);
  pinMode(ECHO , INPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN1A, OUTPUT);
  pinMode(IN2A, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1B, OUTPUT);
  pinMode(IN2B, OUTPUT);
 
}

void loop(){

  if (flagConfiguration){
    if(flagDay){
      logicRobot();
    }

    if (flagSleep){
      Serial.println("SLEEP");
      delay(100);
      esp_sleep_enable_timer_wakeup(300 * 1000000); // 5 minute
      esp_light_sleep_start();
      delay(100);
      
      byte theHour = rtc.getHour(true);
      if((theHour >= beginNightH || theHour <= endNightH)){
          flagDay = false;
      }else{
          flagDay = true;
          flagSleep = false;
      }
      if (rtc.getDayofWeek() == dayWater){
        if (flagNeedWater){
          waterPlant();
          flagNeedWater = false;
        }
      }else{
        flagNeedWater = true;
      }
    }
  }

  if (deviceConnected) {
    pSensorHumidityCharacteristic->setValue(String(lightL).c_str());
    pSensorHumidityCharacteristic->notify();
    delay(3000); 
  }

  if (!deviceConnected && oldDeviceConnected) {
    Serial.println("Device disconnected.");
    delay(500); 
    pServer->startAdvertising(); 
    Serial.println("Start advertising");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {

    oldDeviceConnected = deviceConnected;
    Serial.println("Device Connected");
  }
}