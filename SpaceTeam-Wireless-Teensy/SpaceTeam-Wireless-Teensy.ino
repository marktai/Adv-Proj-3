 /*
TEENSY
*/

#include <SPI.h>
#include "RF24.h"


/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(9,10);
/**********************************************************/

byte addresses[][6] = {"1Node","2Node"};

#define BUFFER_SIZE 127

typedef enum color {
  none,
  yellow,
  green,
  red
};

typedef enum btnPrs {
  b_none,
  b_left,
  b_right
};

typedef struct TransmissionStruct {
  int button_press;
  double battery_voltage;
};


TransmissionStruct packet;


// digital pin 2 has a pushbutton attached to it. Give it a name:
int ledY = 19;
int ledG = 21;
int ledR = 22;

// if byte doesn't work, use byte
// 0 means end of array

typedef struct RX_Sequence {
  uint8_t len;
  byte sequence[BUFFER_SIZE];
};

RX_Sequence sequence;
bool starting = false;

int COLOR_DURATION = 500;
int OFF_DURATION = 500;


// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
//  Serial2.begin(9600);
  // make the pushbutton's pin an input:
  pinMode(ledY, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(13, OUTPUT);
  // 0 the sequence buffer
  memset(&sequence, 0, sizeof(sequence));
  sequence.len = 0;

  setColorHigh(ledR);

  Serial.print("before radio begin");
  
  radio.begin();
//  radio.setPALevel(RF24_PA_LOW);
  radio.setChannel(11);
  radio.printDetails();
  
  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1,addresses[0]);
  
  // Start the radio listening for data
  Serial.print("before radio startlistening");
  radio.startListening();
  delay(7000);
  radio.printDetails();


}

int addColor(RX_Sequence& sequence) {
  if (sequence.len > BUFFER_SIZE) {
    return 1; // cant hold the entire sequence in memory
  }

  sequence.sequence[sequence.len] = random(1, 4); // (yellow, green, red)

  sequence.len += 1;
  return 0;
}
void setColor(byte color, bool highOrLow) {
  switch (color) {
    case yellow:
      digitalWrite(ledY, highOrLow);
      break;
    case green:
      digitalWrite(ledG, highOrLow);
      break;
    case red:
      digitalWrite(ledR, highOrLow);
      break;
    case none:
    default:
      ;
  }
}


void setColorHigh(byte color) {
  setColor(color, HIGH);
}

void setColorLow(byte color) {
  setColor(color, LOW);
}

void setAllLow() {
  digitalWrite(ledY, LOW);
  digitalWrite(ledG, LOW);
  digitalWrite(ledR, LOW);
}


void displaySequence(RX_Sequence sequence) {
  setAllLow();
  int i;
  for (i = 0; i < sequence.len; i++) {
    setColorHigh(sequence.sequence[i]);
    delay(COLOR_DURATION);

    setColorLow(sequence.sequence[i]);
    delay(OFF_DURATION);
  }
}

void displayBatteryLevel(double battery_voltage) {
  setAllLow();
  color showColor;
  if (battery_voltage >= 3.9) {
    showColor = green;
  } else if (battery_voltage >= 3.7) {
    showColor = yellow;
  } else {
    showColor = red;
  }

  setColorHigh(showColor);
}


// the loop routine runs over and over again forever:
void loop() {
  
  // radio.stopListening();
  // Serial.println(" waiting to write");
  // radio.write(&sequence, sizeof(RX_Sequence));

  // will be high when waiting for a response
  radio.startListening();
  digitalWrite(13, HIGH);
  packet.button_press = 5;
  Serial.println("before loop");
  while (packet.button_press == 5) {
    if (radio.available()) {
      radio.read(&packet, sizeof(TransmissionStruct));
    }
  }


  digitalWrite(13, LOW);

  printf("Button press: %d, Battery Voltage: %f", packet.button_press, packet.battery_voltage);
  displayBatteryLevel(packet.battery_voltage);




  delay(1);        // delay in between reads for stability
}



