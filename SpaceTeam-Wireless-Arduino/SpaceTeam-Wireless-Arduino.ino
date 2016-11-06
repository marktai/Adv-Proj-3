/*
  DigitalReadSerial
 Reads a digital input on pin 2, prints the result to the serial monitor

 This example code is in the public domain.
 */
#include <SoftwareSerial.h>
#include "printf.h"
#define DEBOUNCE_CONSTANT 10
#define BUFFER_SIZE 128

#include <SPI.h>
#include "RF24.h"


/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(9,10);
/**********************************************************/

byte addresses[][6] = {"1Node","2Node"};

typedef enum color {
  none,
  yellow,
  green,
  red
};


// digital pin 2 has a pushbutton attached to it. Give it a name:
int pBY = 4;
int pBG = 6;
int pBR = 8;

int rX_Ard = 11;
int tX_Ard = 10;



typedef struct RX_Sequence {
  byte sequence[BUFFER_SIZE];
  int length;
};

RX_Sequence sequence;

//debouncing counters
int dbLS = 0;
int dbC = 0;
SoftwareSerial mySerial (rX_Ard, tX_Ard);

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  // make the pushbutton's pin an input:
  pinMode(pBY, INPUT);
  pinMode(pBG, INPUT);
  pinMode(pBR, INPUT);

  Serial.println("shouldn't be trash\n");

  //do the software buttons crap
  pinMode(rX_Ard, INPUT);
  pinMode(tX_Ard, OUTPUT);
  // mySerial.begin(9600);

  // 0 the sequence buffer
  memset(&sequence, 0, sizeof(sequence));


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


int parseInput() {
  while (true){
    //Serial.print("last pressed: ");Serial.println(dbLS);
    //Serial.print("counter: "); Serial.println(dbC);
    int bs1 = digitalRead(pBY);
    int bs2 = digitalRead(pBG);
    int bs3 = digitalRead(pBR);
    
    delay(2);
    if (bs1 == 1){
      //Serial.println("pressed yellow");
      if (dbLS == 1){
        dbC++;
        if (dbC == DEBOUNCE_CONSTANT){
          dbC = 0;
          return yellow;  
        }
      }
      else {
        dbLS = 1;
        dbC = 0;  
      }  
    }

    else if (bs2 == 1){
      //Serial.println("pressed green");
      if (dbLS == 2){
        dbC++;
        if (dbC == DEBOUNCE_CONSTANT){
          dbC = 0;
          return green;  
        }
      }
      else {
        dbLS = 2;
        dbC = 0;  
      }  
    }

    else if (bs3 == 1){
      //Serial.println("pressed red");
      if (dbLS == 3){
        dbC++;
        if (dbC == DEBOUNCE_CONSTANT){
          dbC = 0;
          return red;  
        }
      }
      else {
        dbLS = 3;
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
}

// returns true if the user inputs match the sequence
bool checkSequence(RX_Sequence inputSequence) {
  int lastState = 0;
  int currentState = 0;
  int i;

  for (int k = 0; inputSequence.sequence[k] != none && k < BUFFER_SIZE; k++)
    Serial.print(inputSequence.sequence[k]);
  Serial.println("");

  for (i = 0; i < BUFFER_SIZE && inputSequence.sequence[i] != none; i++) {
    // only breaks when there is a new input
    while (currentState == 0 || currentState == lastState) {
      currentState = parseInput();

      if (currentState == 0) {
        lastState = currentState;
        digitalWrite(13, LOW);
      } else {
        digitalWrite(13, HIGH);
      }
    }
    lastState = currentState;

    Serial.print("total length: "); Serial.println(i);
    if (currentState != inputSequence.sequence[i]) {
      return false;
    }
  }

  return true;
}

// the loop routine runs over and over again forever:
void loop() {

  // read the input pin:
  //int currentState = parseInput();
  // print out the state of the button:
  //Serial.print(digitalRead(pBY));


  radio.startListening();
  // will be high when waiting for a sequence
  digitalWrite(13, HIGH);
  sequence.length = 0;
  
  Serial.println("waiting for input");
  while (sequence.length == 0) {
    if (radio.available()) {
      radio.read(&sequence, sizeof(RX_Sequence));
    }
  }

  // will be low when waiting for a button press (but will turn on for registered button presses)
  digitalWrite(13, LOW);
  bool correctSequence = checkSequence(sequence);

  digitalWrite(13, HIGH);
  radio.stopListening();    
  radio.write(&correctSequence, sizeof(byte));
}
