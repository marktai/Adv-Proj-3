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

const double range = 32768.0;
const double init_a_x = 0.016479;
const double init_a_y = 0.003052;
const double init_a_z = 0.533203;

const int period = 12;
int count = 0;

typedef enum color {
  none,
  yellow,
  green,
  red
};

typedef enum Battery_Level {
  bt_low,
  bt_medium,
  bt_high
};

typedef enum btnPrs {
  b_none,
  b_left,
  b_right
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
  Mouse.screenSize(1366, 768);  // configure screen size
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

  
  radio.begin();
//  radio.setPALevel(RF24_PA_LOW);
  radio.setChannel(11);
  radio.printDetails();
  
  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1,addresses[0]);
  
  // Start the radio listening for data
  radio.startListening();
  delay(1000);
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


void displayBatteryLevel(char battery_level) {
  setAllLow();
  color showColor;
  if (battery_level == bt_high) {
    showColor = green;
  } else if (battery_level == bt_medium) {
    showColor = yellow;
  } else if (battery_level == bt_low) {
    showColor = red;
  }

  setColorHigh(showColor);
}

void printPacket(TransmissionStruct packet) {
  
  Serial.print("Button: ");
  Serial.print((int) packet.button_press);

  Serial.print("; Battery: ");
  Serial.print((int) packet.battery_level);

  /*Serial.print("; Accel: ");
  Serial.print(packet.a_x); 
  Serial.print(", ");
  Serial.print(packet.a_y); 
  Serial.print(", ");
  Serial.print(packet.a_z); */
//  printf("; Accel: %f, %f, %f,", packet.a_x/32768.0, packet.a_y/32768.0, packet.a_z/32768.0);

  /*
  Serial.print("; Gyro: ");
  Serial.print(packet.g_x); 
  Serial.print(", ");
  Serial.print(packet.g_y); 
  Serial.print(", ");
  Serial.print(packet.g_z); 
  */

  Serial.println("");
}

void moveMouse(TransmissionStruct packet) {
  
  double a_x = (packet.a_x - init_a_x)/range;
  double a_y = (packet.a_y - init_a_y)/range;
  double a_z = (packet.a_z - init_a_z)/range;

  printf("Accel: %f, %f, %f\n", a_x, a_y, a_z);

  Mouse.move((int) -70*a_y, (int) -70*a_x);
  Mouse.set_buttons(packet.button_press == b_left, 0, packet.button_press  == b_right);
  
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
  printf("Waiting for input\n");
  while (packet.button_press == 5) {
    if (radio.available()) {
      radio.read(&packet, sizeof(TransmissionStruct));
//        packet.button_press = 0;
//        packet.battery_level = 2;
//        packet.a_x = 15232;
//        packet.a_y = 1000;
//        packet.a_z = -20000;
    }
  }
  printPacket(packet);
  displayBatteryLevel(packet.battery_level);
  moveMouse(packet);

  digitalWrite(13, LOW);
//
//  if (count >= period) {
//    count = count % period;
//  }
//
//  if (0 <= count && count < period/4) {
//    Mouse.move(2,0);
//  } else if (count < period/2) {
//    Mouse.move(0,2);
//  } else if (count < 3* period/4) {
//    Mouse.move(-2,0);
//  } else {
//    Mouse.move(0,-2);
//  }
//
//  count += 1;
}



