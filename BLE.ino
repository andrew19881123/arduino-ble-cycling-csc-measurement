
#include <ArduinoBLE.h>


// BLE UUIDs
#define BLE_UUID_CYCLING_SPEED_CADENCE_SERVICE    "1816"
#define BLE_UUID_CSC_MEASUREMENT                  "2A5B"


// define BLE services
BLEService bleCyclingService( BLE_UUID_CYCLING_SPEED_CADENCE_SERVICE );

// define characteristics
BLECharacteristic bleCscMeasurementCharacteristic(BLE_UUID_CSC_MEASUREMENT, BLEIndicate, 11);

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

byte cyclingData[11] = {3,};

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
  BLE.setLocalName("CyclingSpeed");
  BLE.setAdvertisedService(bleCyclingService);

  // add the characteristic to the service
  bleCyclingService.addCharacteristic(bleCscMeasurementCharacteristic);

  // add service
  BLE.addService(bleCyclingService);

  int32_t wheelRev = 0;
  int16_t wheelTime = 0;
  int16_t crankRev = 0;
  int16_t crankTime = 0;
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
  int32_t wheelRev = 21000;
  int16_t wheelTime = 0;
  int16_t crankRev = 10000;
  int16_t crankTime = 0;

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
      wheelRev += 1;
      wheelTime += 0;
      crankRev += 2;
      crankTime += 0;

      Serial.print("wheelRev: ");
      Serial.println(wheelRev);

      Serial.print("wheelTime: ");
      Serial.println(wheelTime);

      Serial.print("crankRev: ");
      Serial.println(crankRev);

      Serial.print("crankTime: ");
      Serial.println(crankTime);

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
