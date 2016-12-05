/*
  DigitalReadSerial
 Reads a digital input on pin 2, prints the result to the serial monitor

 This example code is in the public domain.
 */
#include <SoftwareSerial.h>
#include "printf.h"
#define DEBOUNCE_CONSTANT 10
#define BUFFER_SIZE 128
#define MPUAddr 0x68

#include <SPI.h>
#include "RF24.h"
#include <Wire.h>


int battery_pin = A3;

#define r1 1437
#define r2 5321 

#define logic_level 3.297



/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(9,10);
/**********************************************************/

byte addresses[][6] = {"1Node","2Node"};

// digital pin 2 has a pushbutton attached to it. Give it a name:
int button_left = 4;
int button_right = 6;



typedef enum btnPrs {
  none,
  left,
  right
};

typedef enum Battery_Level {
  bt_low,
  bt_medium,
  bt_high
};


typedef struct TransmissionStruct {
  char button_press;
  char battery_level;
  int16_t a_x;
  int16_t a_y;
  int16_t a_z;
  int16_t g_x;
  int16_t g_y;
  int16_t g_z;
};

typedef enum : uint8_t
{
  GYRO_PREC_250 = 0,
  GYRO_PREC_500,
  GYRO_PREC_1000,
  GYRO_PREC_2000
} gyro_precision_e;
typedef enum : uint8_t
{
  ACCEL_PREC_2 = 0,
  ACCEL_PREC_4,
  ACCEL_PREC_8,
  ACCEL_PREC_16
} accel_precision_e;



TransmissionStruct packet;

//debouncing counters
int dbLS = 0;
int dbC = 0;




// This function must be able to toggle sleep mode for the MPU6050
void setSleep(bool enable) {
  Wire.beginTransmission(MPUAddr);
  Wire.write(0x6B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPUAddr, 1, true);
  uint8_t power = Wire.read(); // read 0x6B
  Serial.print("Power byte: ");
  Serial.println(power);
  power &= ~(1 << 6);
  power |= (power ? 1 : 0) << 6;

  Wire.beginTransmission(MPUAddr);
  Wire.write(0x6B);
  Wire.write(power);
  Wire.endTransmission(true);
}

void read3_16Bits( int a_h, int16_t* ax, int b_h, int16_t* bx, int c_h, int16_t* cx) {

  for (int j = 0; j < 3; j++) {

    int address;
    int16_t* value;

    if (j == 0) {
      address = a_h;
      value = ax;
    } else if (j == 1) {
      address = b_h;
      value = bx;
    } else if (j == 2) {
      address = c_h;
      value = cx;
    }

    Wire.beginTransmission(MPUAddr);
    Wire.write(address);
    bool lastTransmission = (j == 2);
    Wire.endTransmission(true);
    Wire.requestFrom(MPUAddr, 2, lastTransmission);
    int16_t readValue = Wire.read() << 8 | Wire.read(); // read 0x6B
    *value = readValue;
  }
}

// This function needs to return by reference the accelerometer values stored in the accelerometer registers. 
void getAccelData( int16_t* ax,int16_t* ay, int16_t* az) {
  read3_16Bits(
    0x3B, ax,
    0x3D, ay,
    0x3F, az
  );
}

// This function needs to return by reference the gyroscope values stored in the accelerometer registers. 
void getGyroData( int16_t* gx,int16_t* gy, int16_t* gz) {
  read3_16Bits(
    0x43, gx,
    0x45, gy,
    0x47, gz
  );
}

// This function needs to set the precision bits for the gyroscope to val (refer to the lecture slides)
void setGyroPrec(gyro_precision_e prec) {
  Wire.beginTransmission(MPUAddr);
  Wire.write(0x1B);
  Wire.write((0b11 & prec) << 3);
  Wire.endTransmission(true);
}

// This function needs to set the precision bits for the accelerometer to val (refer to the lecture slides)
void setAccelPrec(accel_precision_e prec) {
  Wire.beginTransmission(MPUAddr);
  Wire.write(0x1C);
  Wire.endTransmission(false);
  Wire.requestFrom(MPUAddr, 1, true);
  uint8_t accel_config = Wire.read(); // read 0x6B
  accel_config &= ~(0b11 << 3);
  accel_config |= (0b11 & prec) << 3;
  Serial.print("accel_config: ");
  Serial.println(accel_config);

  Wire.beginTransmission(MPUAddr);
  Wire.write(0x1C);
  Wire.write(accel_config);
  Wire.endTransmission(true);

}


// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  Wire.begin();

  pinMode(button_left, INPUT);
  pinMode(button_right, INPUT);

  pinMode(battery_pin, INPUT);

//  pinMode(rX_Ard, INPUT);
//  pinMode(tX_Ard, OUTPUT);
  // mySerial.begin(9600);

  // 0 the sequence buffer
  memset(&packet, 0, sizeof(packet));


  printf_begin();
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setChannel(11);
  radio.printDetails();
  
  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1,addresses[1]);
  
  // Start the radio listening for data
  radio.startListening();

  setSleep(false);
  setGyroPrec(GYRO_PREC_1000);
  setAccelPrec(ACCEL_PREC_2);
  
  delay(10);

}


btnPrs parseInput() {
  while (true){
    //Serial.print("last pressed: ");Serial.println(dbLS);
    //Serial.print("counter: "); Serial.println(dbC);
    int bs1 = digitalRead(button_left);
    int bs2 = digitalRead(button_right);
    
    delay(2);
    if (bs1 == 1){
      //Serial.println("pressed left");
      if (dbLS == 1){
        dbC++;
        if (dbC == DEBOUNCE_CONSTANT){
          dbC = 0;
          return left;  
        }
      }
      else {
        dbLS = 1;
        dbC = 0;  
      }  
    }

    else if (bs2 == 1){
      //Serial.println("pressed right");
      if (dbLS == 2){
        dbC++;
        if (dbC == DEBOUNCE_CONSTANT){
          dbC = 0;
          return right;  
        }
      }
      else {
        dbLS = 2;
        dbC = 0;  
      }  
    }

    else{
      if (dbLS == 0){
        dbC++;
        if (dbC == DEBOUNCE_CONSTANT){
          dbC = 0;
          return none;  
        }
      }
      else {
        dbLS = 0;
        dbC = 0;  
      }  
    }
  }

  return none;
}


char convertBatteryLevel(double battery_voltage) {
  if (battery_voltage >= 3.9) {
    return bt_high;
  } else if (battery_voltage >= 3.7) {
    return bt_medium;
  } else {
    return bt_low;
  }
}

void printPacket(TransmissionStruct packet) {
  
  Serial.print("Button: ");
  Serial.print((int) packet.button_press);

  Serial.print("; Battery: ");
  Serial.print((int) packet.battery_level);

  Serial.print("; Accel: ");
  Serial.print(packet.a_x); 
  Serial.print(", ");
  Serial.print(packet.a_y); 
  Serial.print(", ");
  Serial.print(packet.a_z); 

  Serial.print("; Gyro: ");
  Serial.print(packet.g_x); 
  Serial.print(", ");
  Serial.print(packet.g_y); 
  Serial.print(", ");
  Serial.print(packet.g_z); 

  Serial.println("");
}
// the loop routine runs over and over again forever:
void loop() {


  double vOut = analogRead(battery_pin)*logic_level/1023;
  double vBattery = vOut*(r1+r2)/r2;
  Serial.println(vBattery);


  digitalWrite(13, HIGH);

  memset(&packet, 0, sizeof(packet));
  packet.button_press = parseInput();
  packet.battery_level = convertBatteryLevel(vBattery); 

  getAccelData(&packet.a_x, &packet.a_y, &packet.a_z);
  getGyroData(&packet.g_x, &packet.g_y, &packet.g_z);

  //printPacket(packet);


  digitalWrite(13, LOW);
  radio.stopListening();    
  radio.write(&packet, sizeof(TransmissionStruct));
  radio.startListening();
  delay(20);
}


