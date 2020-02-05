#include<Wire.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24Network.h>

//Code for Node02 with Accelerometer GY-521

const int MPU = 0x68;
float AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;
String heaterLevel;
int ledStatus;

int led = 2;

RF24 radio(7, 8); // CE, CSN
RF24Network network(radio);

const uint16_t this_node = 01;   // Address of this node in Octal format ( 04,031, etc) as described in the hardware X paper
const uint16_t master00 = 00;

const unsigned long interval = 1000;  //ms  // How often to send data to the other unit
unsigned long last_sent;            // When did we last send?

void setup() {
  pinMode(led, OUTPUT);

  Serial.begin(9600);

  SPI.begin();
  radio.begin();
  network.begin(90, this_node);  //(channel, node address)
  radio.setDataRate(RF24_2MBPS);

  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}
void loop() {
  //Acceleration data correction
  int AcXoff = -950;
  int AcYoff = -300;
  int AcZoff = 0;

  char incomingData[32] = "";

  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 12, true);
  AcX = (Wire.read() << 8 | Wire.read()) + AcXoff;
  AcY = (Wire.read() << 8 | Wire.read()) + AcYoff;
  AcZ = (Wire.read() << 8 | Wire.read()) + AcZoff;
  GyX = Wire.read() << 8 | Wire.read();
  GyY = Wire.read() << 8 | Wire.read();
  GyZ = Wire.read() << 8 | Wire.read();

  float AcXG = AcX / 16384;
  float AcYG = AcY / 16384;
  float AcZG = AcZ / 16384;

  // use the following comment as debugging for calibration purposes
  
  /*
   *   Serial.print("Accelerometer: ");
  Serial.print("X = "); Serial.print(AcXG);
  Serial.print(" | Y = "); Serial.print(AcYG);
  Serial.print(" | Z = "); Serial.println(AcZG);*/

  //String accelerometer = String(AcXG, 2) + ',' + String(AcYG, 2);
  char sendData[50];
  //accelerometer.toCharArray(sendData, 50);


  // Here you can adjust the borders of each heater level. May you have to adjust them slightly
  if (-0.1 <= AcXG && AcXG <= 0.1 && 0.9 <= AcYG) {
    heaterLevel = "0";
  }

  if (0.1 <= AcXG && AcXG <= 0.85 && 0.15 <= AcYG && AcYG <= 0.9) {
    heaterLevel = "1";
  }

  if (0.85 <= AcXG && -0.65 <= AcYG && AcYG <= 0.15) {
    heaterLevel = "2";
  }

  if (0.15 <= AcXG && AcXG <= 0.85 && AcYG <= -0.65) {
    heaterLevel = "3";
  }

  if (-0.65 <= AcXG && AcXG <= 0.15 && -0 - 9 <= AcYG && AcYG <= -0.7) {
    heaterLevel = "4";
  }

  if (AcXG <= -0.65 && -0.7 <= AcYG && AcYG <= -0.25) {
    heaterLevel = "5";
  }
  heaterLevel.toCharArray(sendData, 50);
  //Serial.println(sendData);

  network.update();
  //===== Receiving =====//
  while ( network.available() ) {     // Is there any incoming data?
    RF24NetworkHeader header;

    network.read(header, &incomingData, sizeof(incomingData)); // Read the incoming data
    ledStatus = String(incomingData).toInt();

    Serial.print("Led Status: ");
    Serial.println(incomingData);
  }

  //===== Sending =====//

  /* Instead of sending in intervals, you can add a logic here to send only if the heater level changed.
   * This could help to save energy. Hint: See logic in the base unit with oldLedStatus and ledStatus.
  */ 

  
  unsigned long now = millis();
  if (now - last_sent >= interval) {   // If it's time to send a data, send it!
    last_sent = now;
    RF24NetworkHeader header(master00);   // (Address where the data is going)
    bool ok = network.write(header, &sendData, sizeof(sendData)); // Send the data
  }

  if (ledStatus == 1) {
    digitalWrite(led, HIGH);
  }
  else {
    digitalWrite(led, LOW);
  }
}
