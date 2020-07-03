
#include <ArduinoBLE.h>


// -------------------------------- BLE --------------------------------- 
// advertised name              2A00              Wahoo CADENCE BF59
// appearance                   2A01              [115] Cycling: Cadence Sensor (Cyclig subtype)
// peripheral preferred conn.   2A04              conn interval: 500.00ms - 1000.00ms, slave latency: 0, supervision timeout multiplier: 600

#define DEVICE_NAME             "Wahoo CADENCE BF58"
#define DEVICE_APPEARANCE       0x2a01
#define CONN_INT_LOW            0x0190
#define CONN_INT_HIGH           0x0320

// --------------------------------------------- DEVICE INFORMATION ----------------
// s device_information         180A
// c manufacturer_name_string   2a29  read    ->  value: Wahoo Fitness
// c hardware_revision_string   2a27  read    ->  value: 7
// c firmware_revision_string   2a26  read    ->  value: 2.0.19

#define BLE_UUID_DEVICE_INFORMATION_SERVICE                               "180A" 
BLEService bleDeviceInfoService( BLE_UUID_DEVICE_INFORMATION_SERVICE );

#define BLE_UUID_MANUFACTURER_NAME_STRING_CHARACTERISTIC                  "2A29"
#define BLE_UUID_HARDWARE_REVISION_STRING_CHARACTERISTIC                  "2A27"
#define BLE_UUID_CSC_FIRMWARE_REVISION_STRING_CHARACTERISTIC              "2A26"

BLECharacteristic bleManufacturerNameStringCharacteristic(BLE_UUID_MANUFACTURER_NAME_STRING_CHARACTERISTIC, BLERead , "Wahoo Fitness");
BLECharacteristic bleHardwareRevisionStringCharacteristic(BLE_UUID_HARDWARE_REVISION_STRING_CHARACTERISTIC, BLERead , "7");
BLECharacteristic bleFirmwareRevisionStringCharacteristic(BLE_UUID_CSC_FIRMWARE_REVISION_STRING_CHARACTERISTIC, BLERead , "2.0.19");


// ------------------------- BATTERY ---------------------------
// s battery_service          180F
// c battery_level            2A19    notify read
//      client_characteristic_configuration     notification indications disabled

#define BLE_UUID_BATTERY_SERVICE                                          "180F"
BLEService bleBatteryService( BLE_UUID_BATTERY_SERVICE );

#define BLE_UUID_BATTERY_LEVEL_CHARACTERISTIC                             "2A19"
#define BATTERY_LEVEL_BYTES         1
uint8_t battery_level_percentage = 80;
size_t battery_level_percentage_bytes = 1;
BLECharacteristic bleBatteryLevelCharacteristic(BLE_UUID_BATTERY_LEVEL_CHARACTERISTIC, BLERead | BLENotify , BATTERY_LEVEL_BYTES);

byte batteryLevelData[BATTERY_LEVEL_BYTES];

// -------------------- CYCLING_SPEED_CADENCE -----------------------
#define BLE_UUID_CYCLING_SPEED_CADENCE_SERVICE    "1816"  

BLEService bleCyclingService( BLE_UUID_CYCLING_SPEED_CADENCE_SERVICE );


#define BLE_UUID_CSC_MEASUREMENT_CHARACTERISTIC                  "2A5B"  // notify   client characteristic configuration 2902
// csc feauture     2a5c      read
// sensor location  2a5d      read 
// sc contrl point  2a55      indicate notify   client characteristic configuration 2902


// define BLE services


// define characteristics
BLECharacteristic bleCscMeasurementCharacteristic(BLE_UUID_CSC_MEASUREMENT_CHARACTERISTIC, BLENotify, 11);

/*
  Flags                 Mandatory     8bit      0     1   Wheel Revolution Data Present  0   false
  Flags                 Mandatory     8bit      0     1   Wheel Revolution Data Present  1   true
  Flags                 Mandatory     8bit      1     1   Crank Revolution Data Present  0   false
  Flags                 Mandatory     8bit      1     1   Crank Revolution Data Present  1   true
  Flags                 Mandatory     8bit      
  Flags                 Mandatory     8bit     
  Flags                 Mandatory     8bit
  Flags                 Mandatory     8bit
  Flags                 Mandatory     8bit
  Flags                 Mandatory     8bit
  Flags                 Mandatory     8bit

  Cumulative Wheel Revolutions        32bit   uint32
  Last Wheel Event Time               16bit   uint16
  Cumulative Crank Revolutions        16bit   uint16
  Last Crank Event Time               16bit   uint16

  TOTAL                               1B+4B+2B+2B+2B = 11B

  FLAG: 00000011 = 3
*/

uint8_t flags = 0b00000011;

byte cyclingData[11];

void setup() {

  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  // set LED pin to output mode

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");

    // while (1);
  }

  // set advertised local name and service UUID:
  BLE.setLocalName(DEVICE_NAME);
  BLE.setDeviceName(DEVICE_NAME);
  BLE.setAppearance(DEVICE_APPEARANCE);
  BLE.setConnectionInterval(CONN_INT_LOW, CONN_INT_HIGH);
  BLE.setAdvertisedService(bleCyclingService);



  // --- DEVICE INFO ---
  bleDeviceInfoService.addCharacteristic(bleManufacturerNameStringCharacteristic);
  bleDeviceInfoService.addCharacteristic(bleHardwareRevisionStringCharacteristic);
  bleDeviceInfoService.addCharacteristic(bleFirmwareRevisionStringCharacteristic);

  BLE.addService(bleDeviceInfoService);



  // ----- BATTERY -----
  bleBatteryService.addCharacteristic(bleBatteryLevelCharacteristic);
  // bleBatteryLevelCharacteristic.addDescriptor();

  BLE.addService(bleBatteryService);

  uint8_t battery_level = battery_level_percentage;
  memcpy(batteryLevelData, &battery_level, battery_level_percentage_bytes);

  bleBatteryLevelCharacteristic.writeValue(batteryLevelData, BATTERY_LEVEL_BYTES);


  // ------- CYCLING SERVICE ------

  bleCyclingService.addCharacteristic(bleCscMeasurementCharacteristic);

  // add service
  BLE.addService(bleCyclingService);

  int32_t wheelRev = 0;
  int16_t wheelTime = 1000;
  int16_t crankRev = 0;
  int16_t crankTime = 1000;
  memcpy(cyclingData, &flags, 1);
  memcpy(cyclingData + 1, &wheelRev, 4);
  memcpy(cyclingData + 5, &wheelTime, 2);
  memcpy(cyclingData + 7, &crankRev, 2);
  memcpy(cyclingData + 9, &crankTime, 2);
  bleCscMeasurementCharacteristic.writeValue(cyclingData, 11);

  // start advertising
  BLE.advertise();

  Serial.println("CyclingSpeed published");
  Serial.println("");

}


void loop() {
  
  static int etat = 0;
  
  int32_t wheelRev = 0;
  int16_t wheelTime = 1000;
  int16_t crankRev = 0;
  int16_t crankTime = 1000;

  // Serial.println(BLE.address());
  // listen for BLE peripherals to connect:
  BLEDevice central = BLE.central();

  if (etat == 0) {
    etat = 1;
    Serial.println("waiting for client to connect");
    Serial.println("");
  }

  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());

    // while the central is still connected to peripheral:
    while (central.connected()) {
      wheelRev += 2;
      wheelTime += 0;
      crankRev += 4;
      crankTime += 0;

      Serial.print("wheelRev: ");
      Serial.println(wheelRev);

      Serial.print("wheelTime: ");
      Serial.println(wheelTime);

      Serial.print("crankRev: ");
      Serial.println(crankRev);

      Serial.print("crankTime: ");
      Serial.println(crankTime);

      memcpy(cyclingData, &flags, 1);
      memcpy(cyclingData + 1, &wheelRev, 4);
      memcpy(cyclingData + 5, &wheelTime, 2);
      memcpy(cyclingData + 7, &crankRev, 2);
      memcpy(cyclingData + 9, &crankTime, 2);
      bleCscMeasurementCharacteristic.writeValue(cyclingData, 11);

      delay(1000);
    }

    // when the central disconnects, print it out:
    Serial.print(("Disconnected from central: "));
    Serial.println(central.address());
  }
  // delay(5000);
}
