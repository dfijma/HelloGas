
#include <PowerNodeV11.h> // -- this is an olimex board.
#include <ACNode.h>
#include "MachineState.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Adafruit_MCP23X17.h>

#include "Button.h" // Custom Button class (to support MCP)

#define MACHINE "gas"

#ifndef OTA_PASSWD
#define OTA_PASSWD "Foo"
#warning "Setting easy to guess/hardcoded OTA password."
#endif

// I2C 
const uint8_t I2C_SDA_PIN = 13; // i2c SDA Pin, ext 2, pin 10
const uint8_t I2C_SCL_PIN = 16; // i2c SCL Pin, ext 2, pin 7
const uint32_t I2C_FREQ = 100000U; 

const int I2C_POWER_PIN = 15;

// MCP I/O-extender, with buttons
// NodeStandard buttons 1..3 are on MCP GPA0..GPA2, see https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library
const uint8_t MCP_I2C_ADDR = 0x20;

// Button2 via MCP (pin GPA1)
const int SWITCH2 = 1; // GPA1

// The ACNode "machine"
ACNode node = ACNode(MACHINE);
OTA ota = OTA(OTA_PASSWD); // TODO: find out how this actually works
MqttLogStream mqttlogStream = MqttLogStream();

MachineState machinestate = MachineState();

TelnetSerialStream telnetSerialStream = TelnetSerialStream();

// MCP object with buttons wired to it, with callback
Adafruit_MCP23X17 mcp;

// Button 
void onButtonPressed(int pin, int state);

Button button(&mcp, SWITCH2, onButtonPressed);

// The 'application state'

unsigned long now;


void onButtonPressed(int pin, int state) {
   if (state == HIGH) {
     // active Low
     return; 
   }
    Log.printf("button pressed pin %d\n", pin);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n\n");
  Serial.println("Booted: " __FILE__ " " __DATE__ " " __TIME__ );

  now = millis();

  // TODO: find out more on this MQTT-stuff
  node.set_mqtt_prefix("test");
  node.set_master("master");

  // i2C Setup
  pinMode(I2C_POWER_PIN, OUTPUT);
  digitalWrite(I2C_POWER_PIN, LOW);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);

  if (!mcp.begin_I2C(MCP_I2C_ADDR, &Wire)) {
    Log.println("cannot initialize MCP I/O-extender");
  }
  
  // begin buttons, now that i2c and mcp are started
  button.begin();

  node.onConnect([]() {
    Log.println("Connected");
    machinestate = MachineState::WAITINGFORCARD;
  });

  node.onDisconnect([]() {
    Log.println("Disconnected");
    machinestate = MachineState::NOCONN;
  });

  node.onError([](acnode_error_t err) {
    Log.printf("Error %d\n", err);
    machinestate = MachineState::WAITINGFORCARD;
  });

  node.addHandler(&ota);
  node.addHandler(&machinestate);

  Log.addPrintStream(std::make_shared<MqttLogStream>(mqttlogStream));
  auto t = std::make_shared<TelnetSerialStream>(telnetSerialStream);
  Log.addPrintStream(t);
  Debug.addPrintStream(t);

  node.begin(BOARD_OLIMEX); // OLIMEX  
  Log.println("Booted: " __FILE__ " " __DATE__ " " __TIME__ );
}

void loop() {
  node.loop();
  now = millis();
  button.check(); // This could probably be integrated into node.loop() 
  switch (machinestate.state()) {
    case MachineState::WAITINGFORCARD:
      break;
    default:
      break;
  }

}
