/*
  Arduino Wireless Network - Multiple NRF24L01 Tutorial
          == Base/ Master Node 00==
  by Dejan, www.HowToMechatronics.com
  Libraries:
  nRF24/RF24, https://github.com/nRF24/RF24
  nRF24/RF24Network, https://github.com/nRF24/RF24Network

*/

#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <nRF24L01.h>

#include <WiFiNINA.h>

#include "arduino_secrets.h"

WiFiSSLClient client;  //Instantiate WiFi object, can scope from here or Globally

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the Wifi radio's status

const char* host = "script.google.com";
const int httpsPort = 443;

const char* fingerprint = "46 B2 C3 44 9C 59 09 8B 01 B6 F8 BD 4C FB 00 74 91 2F EF F6"; // ignore this
String GAS_ID = "";   // Enter your Google Apps Script service id

int buzzer = 2;

// List for each unit one variable
int stateWindow011;
int stateWindow021;
int stateWindow012;

int stateHeater1;
int stateHeater2;

// List the correct addresses of all units
RF24 radio(7, 6);               // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 00;   // Address of this node in Octal format ( 04,031, etc)
const uint16_t node01 = 01;      // Address of the other node in Octal format
const uint16_t node02 = 02;
const uint16_t node011 = 011;
const uint16_t node021 = 021;
const uint16_t node012 = 012;

const unsigned long interval = 1000;  //ms  // How often to send data to the other unit
unsigned long last_sent; // When did we last send?

// Declare how often you want to send your data to the google sheet
const unsigned long interval_google = 60000;
unsigned long last_sent_google;

unsigned long misconductTimer;
int misconductSeconds;


void setup() {
  Serial.begin(9600);

  Serial.println("Connect to Wifi");

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    //Connect to WPA/WPA2 network.Change this line if using open/WEP network
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  Serial.println("Connected to wifi");
  printWifiStatus();

  SPI.begin();
  radio.begin();
  network.begin(90, this_node);  //(channel, node address)
  radio.setDataRate(RF24_2MBPS);
}

void loop() {
  char incomingData[32] = "";
  char sendData1[50];
  char sendData2[50];

  // Declare one led status for each heater (old and new one)
  String ledStatus1;
  String ledStatus2;
  String oldLedStatus1;
  String oldLedStatus2;

  unsigned long now = millis();

  network.update();
  //===== Receiving =====//
  while ( network.available() ) {     // Is there any incoming data?
    RF24NetworkHeader header;

    network.read(header, &incomingData, sizeof(incomingData)); // Read the incoming data

    // Use this pattern to receive the data of the corresponding units and assign it to the variables initiated above
    if (header.from_node == 01) {
      stateHeater1 = String(incomingData).toInt();

      Serial.print("Heater Level 1: ");
      Serial.println(stateHeater1);
    }

    if (header.from_node == 02) {
      stateHeater2 = String(incomingData).toInt();

      Serial.print("Heater Level 2: ");
      Serial.println(stateHeater2);
    }

    if (header.from_node == 011) {
      stateWindow011 = incomingData[0];
      Serial.print("State window 1 from Node 01: ");
      Serial.println(stateWindow011);
    }

    if (header.from_node == 021) {
      stateWindow021 = incomingData[0];
      Serial.print("State window 2 from Node 01: ");
      Serial.println(stateWindow021);
    }


    if (header.from_node == 012) {
      stateWindow012 = incomingData[0];
      Serial.print("State window 1 from Node 02: ");
      Serial.println(stateWindow012);
    }
  }

  // The pattern to send led signal to the heaters. Please add all of your windows in the inner brackets
  //===== Sending Node01 =====//
  if (stateHeater1 > 0 && (stateWindow011 == 1 || stateWindow021 == 1 || stateWindow012 == 1)) {
    ledStatus1 = "1";
    ledStatus1.toCharArray(sendData1, 50);
  }
  
  else {
    ledStatus1 = "0";
    ledStatus1.toCharArray(sendData1, 50);
  }

  if (oldLedStatus1 != ledStatus1) {   // If it's time to send a data, send it!
    RF24NetworkHeader header(node01);   // (Address where the data is going)
    bool ok = network.write(header, &sendData1, sizeof(sendData1)); // Send the data
  }

  //===== Sending Node 02 =====//
  if (stateHeater2 > 0 && (stateWindow011 == 1 || stateWindow021 == 1 || stateWindow012 == 1)) {
    ledStatus2 = "1";    
    ledStatus2.toCharArray(sendData2, 50);
  }
  else {
    ledStatus2 = "0";    
    ledStatus2.toCharArray(sendData2, 50);
  }

  if (oldLedStatus2 != ledStatus2) {   // If it's time to send a data, send it!
    last_sent = now;
    RF24NetworkHeader header(node02);   // (Address where the data is going)
    bool ok = network.write(header, &sendData2, sizeof(sendData2)); // Send the data
  }

  
  // controls if there is any misconduct behavior. Add all your windows in the left brackets and the heaters ain the right ones
  if ((stateWindow021 == 1 || stateWindow012 == 1 || stateWindow011 == 1) && (stateHeater2 > 0 || stateHeater1 > 0)) {
    tone(buzzer, 250);

    if (misconductTimer == 0) {
      misconductTimer = now;
    }
  }

  else {
    noTone(buzzer);
    if (misconductTimer > 0) {
      misconductSeconds += now - misconductTimer;
    }
    misconductTimer = 0;
  }

  if (now - last_sent_google >= interval_google) {
    last_sent_google = now;

    if (misconductTimer > 0) {
      misconductSeconds = now - misconductTimer;
    }

    misconductSeconds = misconductSeconds / 1000;

    // Add all data to the requesting method. If you have more or less units, add them in the method as well!
    sendData(stateHeater1, stateHeater2, stateWindow011, stateWindow021, stateWindow012, misconductSeconds);

    if ((stateWindow021 == 1 || stateWindow012 == 1 || stateWindow011 == 1) && (stateHeater2 > 0 || stateHeater1 > 0)) {
      misconductTimer = last_sent_google;
    }

    else {
      misconductTimer = 0;
    }
    misconductSeconds = 0;
  }

  // state all led stati here
  oldLedStatus1 = ledStatus1;
  oldLedStatus2 = ledStatus2;
}

//Send data into Google Spreadsheet, don't forget to declare all variables
void sendData(int stateHeater1, int stateHeater2, int stateWindow011, int stateWindow021, int stateWindow012, int misconductSeconds)
{
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  // the variables have to be converted to strings before sending them to the google sheet
  String string_heater1 =  String(stateHeater1);
  String string_heater2 =  String(stateHeater2);
  String string_window011 =  String(stateWindow011);
  String string_window021 =  String(stateWindow021);
  String string_window012 =  String(stateWindow012);
  String string_misconductSeconds = String(misconductSeconds);

  // Mention all of your data in this url! Check if they will be assigned correct in the google script as well
  String url = "/macros/s/" + GAS_ID + "/exec?heater1=" + string_heater1 + "&heater2=" + string_heater2 + "&window011=" + string_window011 + "&window021=" + string_window021  + "&window012=" + string_window012 + "&misconductSeconds=" + string_misconductSeconds;
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266/Arduino CI successfull!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
