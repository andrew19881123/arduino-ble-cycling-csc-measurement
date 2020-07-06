
#include <ArduinoBLE.h>
#include <stdlib.h>

// -------------------------------- BLE --------------------------------- 
// advertised name              2A00              Wahoo CADENCE BF59
// appearance                   2A01              [115] Cycling: Cadence Sensor (Cyclig subtype)
// peripheral preferred conn.   2A04              conn interval: 500.00ms - 1000.00ms, slave latency: 0, supervision timeout multiplier: 600

#define DEVICE_NAME             "Wahoo CADENCE BF58"
#define DEVICE_APPEARANCE       0x2a01



//----------------------generic_access--------------------------
#define BLE_UUID_GENERIC_ACCESS_SERVICE                                     "1800" 
BLEService bleGenericAccessService( BLE_UUID_GENERIC_ACCESS_SERVICE );


#define BLE_UUID_PERIPHERAL_PREFERRED_CONNECTION_PARAMETERS_CHARACTERISTIC  "2A04"

#define PERIPHERAL_PREFERRED_CONNECTION_PARAMETERS_BYTES                  8

BLECharacteristic blePeripheralPreferredConnectionParametersCharacteristic(BLE_UUID_PERIPHERAL_PREFERRED_CONNECTION_PARAMETERS_CHARACTERISTIC, BLERead , PERIPHERAL_PREFERRED_CONNECTION_PARAMETERS_BYTES);

uint16_t minConnInt = 7;
uint16_t maxConnInt = 50;
uint16_t slaveLat = 0;
uint16_t stm = 600;

byte peripheralPreferredConnectionParametersData[PERIPHERAL_PREFERRED_CONNECTION_PARAMETERS_BYTES];

// --------------------------------------------- DEVICE INFORMATION ----------------
// s device_information                           180A
// c manufacturer_name_string                     2A29  read    ->  value: Wahoo Fitness
// c hardware_revision_string                     2a27  read    ->  value: 7
// c firmware_revision_string                     2A26  read    ->  value: 2.0.19
// c peripheral_preferred_connection_parameters   2A04          ->  value: conn interval: 500.00ms - 1000.00ms, slave latency: 0, supervision timeout multiplier: 600

#define BLE_UUID_DEVICE_INFORMATION_SERVICE                               "180A" 
BLEService bleDeviceInfoService( BLE_UUID_DEVICE_INFORMATION_SERVICE );


#define BLE_UUID_MANUFACTURER_NAME_STRING_CHARACTERISTIC                    "2A29"
#define BLE_UUID_HARDWARE_REVISION_STRING_CHARACTERISTIC                    "2A27"
#define BLE_UUID_CSC_FIRMWARE_REVISION_STRING_CHARACTERISTIC                "2A26"

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

#define BATTERY_LEVEL_BYTES                                               1

uint8_t battery_level_percentage = 80;
size_t battery_level_percentage_bytes = 1;

BLECharacteristic bleBatteryLevelCharacteristic(BLE_UUID_BATTERY_LEVEL_CHARACTERISTIC, BLERead | BLENotify , BATTERY_LEVEL_BYTES);

byte batteryLevelData[BATTERY_LEVEL_BYTES];

/* -------------------- CYCLING_SPEED_AND_CADENCE -----------------------
 s cycling_speed_and_cadence      1816
 c csc_measurement                2A5B    notify

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

 c csc_feature                    2A5C    read
 c sensor_location                2A5D    read
 c sc_control_point               2A55    indicate notify

*/

#define BLE_UUID_CYCLING_SPEED_AND_CADENCE_SERVICE                        "1816"  
BLEService bleCyclingSpeedAndCadenceService( BLE_UUID_CYCLING_SPEED_AND_CADENCE_SERVICE );


#define BLE_UUID_CSC_MEASUREMENT_CHARACTERISTIC                           "2A5B"
#define BLE_UUID_CSC_FEATURE_CHARACTERISTIC                               "2A5C"
#define BLE_UUID_SENSOR_LOCATION_CHARACTERISTIC                           "2A5D"
#define BLE_UUID_SC_CONTROL_POINT_CHARACTERISTIC                          "2A55"

#define CSC_MEASUREMENT_BYTES                                             11
#define CSC_FEATURE_BYTES                                                 2
#define SENSOR_LOCATION_BYTES                                             1
#define SC_CONTROL_POINT_BYTES                                            999 // FIXME VALORE DA CALCOLARE

uint8_t csc_measurement_flags = 0b00000011;
int32_t wheelRev = 0;
int16_t wheelTime = 0;
int16_t crankRev = 0;
int16_t crankTime = 0;

uint16_t csc_feature_flag = 0b0000000000000011;

uint8_t sensor_location_flag = 6;

// define characteristics
BLECharacteristic bleCscMeasurementCharacteristic(BLE_UUID_CSC_MEASUREMENT_CHARACTERISTIC, BLENotify, CSC_MEASUREMENT_BYTES);
BLECharacteristic bleCscFeatureCharacteristic(BLE_UUID_CSC_FEATURE_CHARACTERISTIC, BLERead, CSC_FEATURE_BYTES);
BLECharacteristic bleSensorLocationCharacteristic(BLE_UUID_SENSOR_LOCATION_CHARACTERISTIC, BLERead, SENSOR_LOCATION_BYTES);
BLECharacteristic bleScControlPointCharacteristic(BLE_UUID_SC_CONTROL_POINT_CHARACTERISTIC, BLEIndicate | BLEWrite, SC_CONTROL_POINT_BYTES);

byte cscMeasurementData[CSC_MEASUREMENT_BYTES];
byte cscFeatureData[CSC_FEATURE_BYTES];
byte sensorLocationData[SENSOR_LOCATION_BYTES];


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
  BLE.setAdvertisedService(bleCyclingSpeedAndCadenceService);


  // -----GENERIC ACCESS ------
  bleGenericAccessService.addCharacteristic(blePeripheralPreferredConnectionParametersCharacteristic);

  memcpy(peripheralPreferredConnectionParametersData, &minConnInt, 2);
  memcpy(peripheralPreferredConnectionParametersData + 2, &maxConnInt, 2);
  memcpy(peripheralPreferredConnectionParametersData + 4, &slaveLat, 2);
  memcpy(peripheralPreferredConnectionParametersData + 6, &stm, 2);
  blePeripheralPreferredConnectionParametersCharacteristic.writeValue(peripheralPreferredConnectionParametersData, PERIPHERAL_PREFERRED_CONNECTION_PARAMETERS_BYTES);


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

  bleCyclingSpeedAndCadenceService.addCharacteristic(bleCscMeasurementCharacteristic);
  bleCyclingSpeedAndCadenceService.addCharacteristic(bleCscFeatureCharacteristic);
  bleCyclingSpeedAndCadenceService.addCharacteristic(bleSensorLocationCharacteristic);
  bleCyclingSpeedAndCadenceService.addCharacteristic(bleScControlPointCharacteristic);

  BLE.addService(bleCyclingSpeedAndCadenceService);

  memcpy(cscMeasurementData, &csc_measurement_flags, 1);
  memcpy(cscMeasurementData + 1, &wheelRev, 4);
  memcpy(cscMeasurementData + 5, &wheelTime, 2);
  memcpy(cscMeasurementData + 7, &crankRev, 2);
  memcpy(cscMeasurementData + 9, &crankTime, 2);
  bleCscMeasurementCharacteristic.writeValue(cscMeasurementData, CSC_MEASUREMENT_BYTES);


  memcpy(cscFeatureData, &csc_feature_flag, CSC_FEATURE_BYTES);
  bleCscFeatureCharacteristic.writeValue(cscFeatureData, CSC_FEATURE_BYTES);


  memcpy(sensorLocationData, &sensor_location_flag, SENSOR_LOCATION_BYTES);
  bleSensorLocationCharacteristic.writeValue(sensorLocationData, SENSOR_LOCATION_BYTES);

  

  // start advertising
  BLE.advertise();

  Serial.println("CyclingSpeed published");
  Serial.println("");

}


void loop() {
  
  static int etat = 0;
  

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
      wheelRev += printRandoms(4, 5);
      wheelTime = printRandoms(4999, 5001);
      crankRev += printRandoms(9, 10);
      crankTime = printRandoms(4999, 5001);

      Serial.print("wheelRev: ");
      Serial.println(wheelRev);

      Serial.print("wheelTime: ");
      Serial.println(wheelTime);

      Serial.print("crankRev: ");
      Serial.println(crankRev);

      Serial.print("crankTime: ");
      Serial.println(crankTime);

      memcpy(cscMeasurementData, &csc_measurement_flags, 1);
      memcpy(cscMeasurementData + 1, &wheelRev, 4);
      memcpy(cscMeasurementData + 5, &wheelTime, 2);
      memcpy(cscMeasurementData + 7, &crankRev, 2);
      memcpy(cscMeasurementData + 9, &crankTime, 2);
      bleCscMeasurementCharacteristic.writeValue(cscMeasurementData, CSC_MEASUREMENT_BYTES);

      delay(1000);
    }

    // when the central disconnects, print it out:
    Serial.print(("Disconnected from central: "));
    Serial.println(central.address());
  }
  // delay(5000);

}

// Generates and prints 'count' random 
// numbers in range [lower, upper]. 
int printRandoms(int lower, int upper) { 
  return (rand() % (upper - lower + 1)) + lower; 
} 