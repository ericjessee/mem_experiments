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

  delayMicroseconds(1);          // Ensure data is stable
  digitalWrite(WE, LOW);         // Trigger write
  delayMicroseconds(1);          // Hold write enable
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

void processCommand(String command) {
  if (command.startsWith("read")) {
    uint16_t address = (uint16_t)strtol(&command[5], NULL, 16);
    uint8_t data = readData(address);
    Serial.print("Data at 0x");
    Serial.print(address, HEX);
    Serial.print(": 0x");
    Serial.println(data, HEX);
  } else if (command.startsWith("write")) {
    // Find the position of the first space after the command keyword
    int spaceIndex = command.indexOf(' ', 6); 
    if (spaceIndex > 0) {
      uint16_t address = (uint16_t)strtol(&command[6], NULL, 16); // Extract the address
      uint8_t data = (uint8_t)strtol(&command[spaceIndex + 1], NULL, 16); // Extract the data
      writeData(address, data);
      Serial.print("Written 0x");
      Serial.print(data, HEX);
      Serial.print(" to 0x");
      Serial.println(address, HEX);
    } else {
      Serial.println("Invalid write command format.");
    }
  } else {
    Serial.println("Invalid command.");
  }
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
  
  Serial.begin(9600);  // Start serial communication
}

// Main loop
void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    processCommand(command);
  }
}
