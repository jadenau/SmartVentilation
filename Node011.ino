/*
  Arduino Wireless Network - Multiple NRF24L01 Tutorial
            == Node 012 (child of Node 02)==    
*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24Network.h>

int sensor = 2;

int state;

RF24 radio(7, 8);               // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network

const uint16_t this_node = 011;  // Address of our node in Octal format ( 04,031, etc)
const uint16_t master00 = 00;    // Address of the other node in Octal format

const unsigned long interval = 1000;  //ms  // How often to send data to the other unit
unsigned long last_sent;            // When did we last send?

void setup() {
  Serial.begin(9600);

  SPI.begin();
  radio.begin();
  network.begin(90, this_node);  //(channel, node address)
  radio.setDataRate(RF24_2MBPS);

  pinMode(sensor, INPUT_PULLUP);
}

void loop() {
  state = digitalRead(sensor);

  Serial.print("State: ");
  Serial.println(state);
  network.update();
  
  //===== Sending =====//

  /* Instead of sending in intervals, you can add a logic here to send only if the windows state is changed.
   * This could help to save energy. Hint: See logic in the base unit with oldLedStatus and ledStatus.
  */ 

  
  unsigned long now = millis();
  if (now - last_sent >= interval) {   // If it's time to send a data, send it!
    last_sent = now;
    RF24NetworkHeader header(master00);
    bool ok = network.write(header, &state, sizeof(state)); // Send the data
  }
  
  delay(1000);
}
