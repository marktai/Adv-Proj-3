/*
  DigitalReadSerial
 Reads a digital input on pin 2, prints the result to the serial monitor

 This example code is in the public domain.
 */
#include <SoftwareSerial.h>
#include "printf.h"
#define DEBOUNCE_CONSTANT 10
#define BUFFER_SIZE 128
#define MPUAddr 1

#include <SPI.h>
#include "RF24.h"


int battery_pin = A1;

#define r1 326
#define r2 1176

#define logic_level 3.486



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

typedef struct TransmissionStruct {
  int button_press;
  double battery_voltage;
};



TransmissionStruct packet;

//debouncing counters
int dbLS = 0;
int dbC = 0;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

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

// the loop routine runs over and over again forever:
void loop() {


  double vOut = analogRead(battery_pin)*logic_level/1023;
  double vBattery = vOut*(r1+r2)/r2;

  // read the input pin:
  //int currentState = parseInput();
  // print out the state of the button:
  //Serial.print(digitalRead(pBY));


//  radio.startListening();
  // will be high when waiting for a sequence
  // digitalWrite(13, HIGH);
  // sequence.length = 0;
  
  // Serial.println("waiting for input");
  // while (sequence.length == 0) {
  //   if (radio.available()) {
  //     radio.read(&sequence, sizeof(RX_Sequence));
  //   }
  // }

  // // will be low when waiting for a button press (but will turn on for registered button presses)
  // digitalWrite(13, LOW);

  // digitalWrite(13, HIGH);
  
  packet.button_press = parseInput();
  packet.battery_voltage = vBattery; 

  // radio.stopListening();    
  radio.write(&packet, sizeof(TransmissionStruct));
}
