/*

  Wiring:
    ESP32 <-- RO --  MAX485 module
    ESP32 --> DI  --> MAX485 module
    ESP32 --> DE+RE (tied together) --> one GPIO, HIGH = transmit
    MAX485 A/B     --> meter's RS-485 terminals

*/

#include <HardwareSerial.h>



#define RS485_RX_PIN     16   
#define RS485_TX_PIN     17   
#define RS485_DE_RE_PIN  4    

const char* METER_ADDRESS = "";

// OBIS code for total active energy (istrukcja PAFAL)
const char* TARGET_OBIS = "15.8.0";

HardwareSerial RS485(2); // ESP32 UART2

// Drive RS485
void setDE(bool transmit) {
  digitalWrite(RS485_DE_RE_PIN, transmit ? HIGH : LOW);
  delayMicroseconds(50);
}

// The meter's identification response encodes, as a single character,
// which baud rate it wants to switch to for the rest of the exchange.

long baudFromChar(char c) {
  switch (c) {
    case '0': return 300;
    case '1': return 600;
    case '2': return 1200;
    case '3': return 2400;
    case '4': return 4800;
    case '5': return 9600;
    case '6': return 19200;
    default:  return 300;
  }
}

// send the wake-up request at 300 baud 
String sendRequestAndGetId() {
  RS485.updateBaudRate(300);
  RS485.flush();

  String request = "/?";
  request += METER_ADDRESS;
  request += "!\r\n";

  setDE(true);
  RS485.print(request);
  RS485.flush();
  setDE(false);

  String id = "";
  unsigned long lastByte = millis();
  while (millis() - lastByte < 2000) {
    if (RS485.available()) {
      char c = RS485.read();
      id += c;
      lastByte = millis();
      if (c == '\n') break;
    }
  }
  return id;
}


void sendAck(char baudChar) {
  setDE(true);
  RS485.write(0x06);   // ACK
  RS485.write('0');    // protocol control: normal / data readout
  RS485.write(baudChar);
  RS485.write('0');    // mode control character
  RS485.print("\r\n");
  RS485.flush();
  setDE(false);
}


String readDataBlock(unsigned long timeoutMs = 3000) {
  String block = "";
  unsigned long lastByte = millis();
  bool started = false;

  while (millis() - lastByte < timeoutMs) {
    if (RS485.available()) {
      char c = RS485.read();
      lastByte = millis();
      if (!started) {
        if (c == 0x02) started = true; // wait for STX
        continue;
      }
      block += c;
      if (c == 0x03) break; // ETX ends the block
    }
  }
  return block;
}


float extractObisValue(const String& block, const String& obisCode) {
  int codePos = block.indexOf(obisCode + "(");
  if (codePos == -1) return NAN;

  int openParen = codePos + obisCode.length();
  int closeParen = block.indexOf(')', openParen);
  if (closeParen == -1) return NAN;

  String valueField = block.substring(openParen + 1, closeParen); // e.g. "0001234.56*kWh"
  int starPos = valueField.indexOf('*');
  String numberPart = (starPos == -1) ? valueField : valueField.substring(0, starPos);

  return numberPart.toFloat();
}

// Full read cycle. Returns NAN on any failure.
float readEnergyKWh() {
  String id = sendRequestAndGetId();
  if (id.length() < 5 || id[0] != '/') {
    Serial.println("No/garbled identification response from meter.");
    return NAN;
  }

  char baudChar = id[4]; // "/PAF" is 4 chars, baud char is the 5th
  long newBaud = baudFromChar(baudChar);

  sendAck(baudChar);
  delay(300); // let the meter switch baud rate on its side
  RS485.updateBaudRate(newBaud);

  String block = readDataBlock();
  if (block.length() == 0) {
    Serial.println("No data block received.");
    return NAN;
  }

  return extractObisValue(block, TARGET_OBIS);
}

void setup() {
  Serial.begin(115200);
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  setDE(false); // start in receive mode

  RS485.begin(300, SERIAL_7E1, RS485_RX_PIN, RS485_TX_PIN);
}

void loop() {
  float energy = readEnergyKWh();
  if (isnan(energy)) {
    Serial.println("Read failed.");
  } else {
    Serial.print("Total active energy: ");
    Serial.print(energy, 2);
    Serial.println(" kWh");
   
  }

  delay(10000); // poll 10s 
}
