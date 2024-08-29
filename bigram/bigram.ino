#include <stdlib.h> // Required for random number generation

// Define maximum memory size for the test buffer
#define MAX_TEST_SIZE 512
#define RAM_START_ADDRESS 0x0000
#define RAM_END_ADDRESS 0x39FF

#define TSETTLE 1

uint8_t testPattern[MAX_TEST_SIZE]; // Buffer to store the test pattern

// Static array to be loaded into memory
static const unsigned char pgm[] = {
    0x0E, 0x00, 0x04, 0x3A, 0x16, 0x00, 0xB8, 0xC2, 0x02, 0x00, 0x0C, 0x2A, 
    0x14, 0x00, 0x71, 0x06, 0x00, 0xC3, 0x02, 0x00, 0x78, 0x00, 0x04, 
};
const uint16_t pgmSize = sizeof(pgm);

// Pin Assignments
const int SER_ADDR = 2;   // Serial data input for address
const int RCLK_ADDR = 3;  // Storage register clock (latch) for address
const int SRCLK_ADDR = 4; // Shift register clock for address
const int OE_ADDR = 5;    // Output enable for address shift registers

const int SER_DOUT = A4;  // Serial data input for data output
const int RCLK_DOUT = A3; // Storage register clock (latch) for data output
const int SRCLK_DOUT = A2;// Shift register clock for data output
const int OE_DOUT = A1;   // Output enable for data output shift register

const int SH_LD_DIN = 7;  // Shift/!Load for data input
const int SER_DIN = 6;    // Serial data output from data input shift register !!USE QH, NOT SER
const int CLK_DIN = 8;    // Clock for data input shift register

const int WE = 9;         // Write Enable for SRAM
const int OE_SRAM = 10;   // Output Enable for SRAM

void testMemoryRegion(uint16_t startAddress, uint16_t endAddress) {
  uint16_t rangeSize = endAddress - startAddress + 1;

  // Generate and write random pattern
  Serial.print("Writing test pattern to range 0x");
  Serial.print(startAddress, HEX);
  Serial.print(" to 0x");
  Serial.println(endAddress, HEX);

  for (uint16_t i = 0; i < rangeSize; i++) {
    uint8_t randomValue = (uint8_t)random(0, 256);  // Generate random value
    testPattern[i] = randomValue;                   // Store it locally
    writeData(startAddress + i, randomValue);       // Write to memory
  }

  // Read back and verify
  Serial.println("Verifying test pattern...");
  bool testPassed = true;
  for (uint16_t i = 0; i < rangeSize; i++) {
    uint8_t readValue = readData(startAddress + i);
    if (readValue != testPattern[i]) {
      testPassed = false;
      Serial.print("Mismatch at 0x");
      Serial.print(startAddress + i, HEX);
      Serial.print(": expected 0x");
      Serial.print(testPattern[i], HEX);
      Serial.print(", got 0x");
      Serial.println(readValue, HEX);
    }
  }

  if (testPassed) {
    Serial.println("Test passed: no errors found.");
  } else {
    Serial.println("Test failed: errors detected.");
  }
}

// Shift out data function
void shiftOutData(int dataPin, int clockPin, int bitOrder, uint8_t val) {
  for (int i = 0; i < 8; i++)  {
    digitalWrite(dataPin, !!(val & (bitOrder == LSBFIRST ? 1 << i : 1 << (7 - i))));
    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW);
  }
}

// Set address function
void setAddress(uint16_t address) {
  digitalWrite(OE_ADDR, HIGH);  // Disable output
  shiftOutData(SER_ADDR, SRCLK_ADDR, MSBFIRST, (address >> 8) & 0xFF);  // Upper byte
  shiftOutData(SER_ADDR, SRCLK_ADDR, MSBFIRST, address & 0xFF);         // Lower byte
  digitalWrite(RCLK_ADDR, HIGH);  // Latch address
  digitalWrite(RCLK_ADDR, LOW);
  digitalWrite(OE_ADDR, LOW);    // Enable output
}

// Write data function
void writeData(uint16_t address, uint8_t data) {
  setAddress(address);
  digitalWrite(OE_SRAM, HIGH);   // Disable SRAM output
  digitalWrite(WE, HIGH);        // Prepare to write

  digitalWrite(OE_DOUT, HIGH);   // Disable data output
  digitalWrite(RCLK_DOUT, LOW); // Latch data
  shiftOutData(SER_DOUT, SRCLK_DOUT, MSBFIRST, data);  // Output data
  digitalWrite(RCLK_DOUT, HIGH);  // Latch data
  digitalWrite(OE_DOUT, LOW);    // Enable data output

  delayMicroseconds(TSETTLE);          // Ensure data is stable
  digitalWrite(WE, LOW);         // Trigger write
  delayMicroseconds(TSETTLE);          // Hold write enable
  digitalWrite(WE, HIGH);        // End write

  digitalWrite(OE_DOUT, HIGH);   // Disable data output
  digitalWrite(OE_ADDR, HIGH);    // Disable address output
}

// Read data function
uint8_t readData(uint16_t address) {
  setAddress(address);
  digitalWrite(OE_SRAM, LOW);    // Enable SRAM output
  digitalWrite(SH_LD_DIN, LOW);  // Load parallel data
  digitalWrite(SH_LD_DIN, HIGH); // Shift mode
  
  uint8_t data = 0;
  for (int i = 0; i < 8; i++) {
    data |= digitalRead(SER_DIN) << (7 - i);
    digitalWrite(CLK_DIN, HIGH);
    digitalWrite(CLK_DIN, LOW);
  }
  
  digitalWrite(OE_SRAM, HIGH);   // Disable SRAM output
  digitalWrite(OE_ADDR, HIGH);    // Disable address output
  return data;
}

void loadpgm() {
  Serial.println("Loading static array into memory...");
  for (uint16_t i = 0; i < pgmSize; i++) {
    writeData(RAM_START_ADDRESS + i, pgm[i]);
  }
  Serial.println("Static array loaded.");
}

void verifypgm(uint16_t startAddress) {
  Serial.println("Verifying static array...");
  bool verificationPassed = true;
  
  for (uint16_t i = 0; i < pgmSize; i++) {
    uint8_t readValue = readData(startAddress + i);
    if (readValue != pgm[i]) {
      verificationPassed = false;
      Serial.print("Mismatch at 0x");
      Serial.print(startAddress + i, HEX);
      Serial.print(": expected 0x");
      Serial.print(pgm[i], HEX);
      Serial.print(", got 0x");
      Serial.println(readValue, HEX);
    }
  }

  if (verificationPassed) {
    Serial.println("Verification passed: no errors found.");
  } else {
    Serial.println("Verification failed: errors detected.");
  }
}

void processCommand(String command) {
  if (command.startsWith("read")) {
    uint16_t address = (uint16_t)strtol(&command[5], NULL, 16);
    uint8_t data = readData(address);
    Serial.print("Data at 0x");
    Serial.print(address, HEX);
    Serial.print(": 0x");
    Serial.println(data, HEX);
  } else if (command.startsWith("write")) {
    int spaceIndex = command.indexOf(' ', 6); 
    if (spaceIndex > 0) {
      uint16_t address = (uint16_t)strtol(&command[6], NULL, 16);
      uint8_t data = (uint8_t)strtol(&command[spaceIndex + 1], NULL, 16);
      writeData(address, data);
      Serial.print("Written 0x");
      Serial.print(data, HEX);
      Serial.print(" to 0x");
      Serial.println(address, HEX);
    } else {
      Serial.println("Invalid write command format.");
    }
  } else if (command.startsWith("test")) {
    int spaceIndex = command.indexOf(' ', 5);
    if (spaceIndex > 0) {
      uint16_t startAddress = (uint16_t)strtol(&command[5], NULL, 16);
      uint16_t endAddress = (uint16_t)strtol(&command[spaceIndex + 1], NULL, 16);
      
      if (startAddress > endAddress) {
        Serial.println("Invalid address range.");
        return;
      }

      if ((endAddress - startAddress + 1) > MAX_TEST_SIZE) {
        Serial.println("Test range too large.");
        return;
      }

      // Call the helper function
      testMemoryRegion(startAddress, endAddress);
    }
  } else if (command.startsWith("alltest")) {
    uint16_t totalStart = 0x0000;
    uint16_t totalEnd = 0x39FF;
    uint16_t chunkSize = MAX_TEST_SIZE;
    Serial.println("test");
    for (uint16_t startAddress = totalStart; startAddress <= totalEnd; startAddress += chunkSize) {
      uint16_t endAddress = startAddress + chunkSize - 1;
      if (endAddress > totalEnd) {
        endAddress = totalEnd;
      }

      // Call the helper function for each chunk
      testMemoryRegion(startAddress, endAddress);
    }

    Serial.println("Full RAM test complete.");
  } else if (command.startsWith("load")) {
    loadpgm();
  } else if (command.startsWith("verify")) {
    uint16_t startAddress = (uint16_t)strtol(&command[7], NULL, 16);
    verifypgm(startAddress);
  } else {
    Serial.println("Invalid command.");
  }
}


void clearMemory() {
  Serial.println("Clearing memory...");
  for (uint16_t address = RAM_START_ADDRESS; address <= RAM_END_ADDRESS; address++) {
    writeData(address, 0x00);
  }
  Serial.println("Memory clear complete.");
}

// Setup function
void setup() {
  // Initialize pins
  pinMode(SER_ADDR, OUTPUT);
  pinMode(RCLK_ADDR, OUTPUT);
  pinMode(SRCLK_ADDR, OUTPUT);
  pinMode(OE_ADDR, OUTPUT);
  pinMode(SER_DOUT, OUTPUT);
  pinMode(RCLK_DOUT, OUTPUT);
  pinMode(SRCLK_DOUT, OUTPUT);
  pinMode(OE_DOUT, OUTPUT);
  pinMode(SH_LD_DIN, OUTPUT);
  pinMode(SER_DIN, INPUT);
  pinMode(CLK_DIN, OUTPUT);
  pinMode(WE, OUTPUT);
  pinMode(OE_SRAM, OUTPUT);
  
  Serial.begin(115200);  // Start serial communication
  randomSeed(analogRead(0)); // Seed the random number generator

  // Clear memory on startup
  clearMemory();
}

// Main loop
void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    processCommand(command);
  }
}
