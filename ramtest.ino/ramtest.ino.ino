// Constants
const int bufferSize = 100; // Adjust size according to your needs

// Pin assignments
const int addressPins[] = {A1, A2, A3, A4};  // Address bits 0-3
const int dataPins[] = {2, 3, 4, 5};         // Data bits 0-3
const int CS1 = 6;                           // Chip Select 1 (Active Low)
const int CS2 = 7;                           // Chip Select 2 (Complement of CS1)
const int OE = 8;                            // Output Enable (Active Low)
const int WE = 9;                            // Write Enable (Active Low)

// State machine states
enum State {
  IDLE,
  WRITE_COMMAND,
  READ_COMMAND,
  TEST_COMMAND
};

// Variables to store address and value
int address = 0;
int value = 0;
int startAddress = 0;
int endAddress = 0;

// Current state of the state machine
State currentState = IDLE;

// Function to fill buffer until a '\r' character is received
void fillBuffer(char* buffer) {
  int bufferIndex = 0;

  while (true) {
    if (Serial.available()) {
      char receivedChar = Serial.read();

      // Check if the received character is a carriage return ('\r')
      if (receivedChar == '\r') {
        buffer[bufferIndex] = '\0'; // Null-terminate the string
        break; // Exit the loop after receiving '\r'
      } 
      else if (bufferIndex < bufferSize - 1) { // Ensure there's room in the buffer
        buffer[bufferIndex++] = receivedChar;  // Store the character in the buffer
      }
    }
  }
}

// Function to set the address on the address pins
void setAddress(int address) {
  for (int i = 0; i < 4; i++) {
    digitalWrite(addressPins[i], (address >> i) & 0x01);
  }
}

// Function to set the data pins for writing
void setData(int data) {
  for (int i = 0; i < 4; i++) {
    pinMode(dataPins[i], OUTPUT);            // Set data pins as output
    digitalWrite(dataPins[i], (data >> i) & 0x01);
  }
}

// Function to read data from the data pins
int readData() {
  int data = 0;
  for (int i = 0; i < 4; i++) {
    pinMode(dataPins[i], INPUT);             // Set data pins as input
    data |= (digitalRead(dataPins[i]) << i);
  }
  return data;
}

// Function to perform the write operation
void writeMemory(int address, int value) {
  setAddress(address);                      // Set the address
  setData(value);                           // Set the data

  digitalWrite(CS1, LOW);                   // Activate chip select
  digitalWrite(CS2, HIGH);                  // Complement of CS1
  digitalWrite(OE, HIGH);                   // Disable output enable
  digitalWrite(WE, LOW);                    // Activate write enable

  delayMicroseconds(1);                     // Ensure at least 150ns delay (~1µs)

  digitalWrite(WE, HIGH);                   // Deactivate write enable
  digitalWrite(CS1, HIGH);                  // Deactivate chip select
  digitalWrite(CS2, LOW);                   // Complement of CS1
}

// Function to perform the read operation
int readMemory(int address) {
  setAddress(address);                      // Set the address

  digitalWrite(CS1, LOW);                   // Activate chip select
  digitalWrite(CS2, HIGH);                  // Complement of CS1
  digitalWrite(OE, LOW);                    // Activate output enable
  digitalWrite(WE, HIGH);                   // Ensure write enable is inactive

  delayMicroseconds(1);                     // Ensure at least 150ns delay (~1µs)

  int data = readData();                    // Read the data

  digitalWrite(OE, HIGH);                   // Deactivate output enable
  digitalWrite(CS1, HIGH);                  // Deactivate chip select
  digitalWrite(CS2, LOW);                   // Complement of CS1

  return data;
}

// Function to perform the test operation with a random pattern
void testMemory(int startAddress, int endAddress) {
  const int regionSize = endAddress - startAddress + 1;
  int pattern[regionSize]; // Array to store the random pattern

  Serial.println("Testing Memory Region:");

  // Initialize random seed
  randomSeed(analogRead(0));

  // Generate and write the random pattern to memory
  for (int addr = 0; addr < regionSize; addr++) {
    pattern[addr] = random(0, 16); // Generate a random 4-bit value (0-15)
    writeMemory(startAddress + addr, pattern[addr]);
  }

  // Read back and compare
  Serial.println("Addr | Written | Read  | Match");
  Serial.println("------------------------------");

  for (int addr = 0; addr < regionSize; addr++) {
    int readValue = readMemory(startAddress + addr); // Read the value from memory
    bool match = (readValue == pattern[addr]);       // Compare with the written value

    // Print result in grid format
    Serial.print("0x");
    Serial.print(startAddress + addr, HEX);
    Serial.print("   |   0x");
    Serial.print(pattern[addr], HEX);
    Serial.print("   |  0x");
    Serial.print(readValue, HEX);
    Serial.print("   |  ");
    Serial.println(match ? "Yes" : "No");
  }
}


// Function to parse and execute the command
void processCommand(const char* buffer) {
  char command[10];
  int parsedItems;

  // Try to parse the buffer for a write command
  parsedItems = sscanf(buffer, "%s 0x%x 0x%x", command, &address, &value);
  if (parsedItems == 3 && strcmp(command, "write") == 0) {
    currentState = WRITE_COMMAND;
    Serial.print("Writing to Address = 0x");
    Serial.print(address, HEX);
    Serial.print(", Value = 0x");
    Serial.println(value, HEX);

    writeMemory(address, value);            // Perform the write operation
    return;
  }

  // Try to parse the buffer for a read command
  parsedItems = sscanf(buffer, "%s 0x%x", command, &address);
  if (parsedItems == 2 && strcmp(command, "read") == 0) {
    currentState = READ_COMMAND;
    Serial.print("Reading from Address = 0x");
    Serial.println(address, HEX);

    int readValue = readMemory(address);    // Perform the read operation
    Serial.print("Read Value = 0x");
    Serial.println(readValue, HEX);
    return;
  }

  // Try to parse the buffer for a test command
  parsedItems = sscanf(buffer, "%s 0x%x 0x%x", command, &startAddress, &endAddress);
  if (parsedItems == 3 && strcmp(command, "test") == 0) {
    currentState = TEST_COMMAND;
    Serial.print("Testing memory from 0x");
    Serial.print(startAddress, HEX);
    Serial.print(" to 0x");
    Serial.println(endAddress, HEX);

    testMemory(startAddress, endAddress);   // Perform the test operation
    return;
  }

  // If neither command is matched, print an error
  Serial.println("Invalid command!");
}

void setup() {
  // Set pin modes for address and control pins
  for (int i = 0; i < 4; i++) {
    pinMode(addressPins[i], OUTPUT);
    pinMode(dataPins[i], INPUT);            // Initialize data pins as input
  }
  pinMode(CS1, OUTPUT);
  pinMode(CS2, OUTPUT);
  pinMode(OE, OUTPUT);
  pinMode(WE, OUTPUT);

  // Initialize control pins to inactive states
  digitalWrite(CS1, HIGH);
  digitalWrite(CS2, LOW);
  digitalWrite(OE, HIGH);
  digitalWrite(WE, HIGH);

  Serial.begin(9600); // Start serial communication
  Serial.print("> "); // Print a prompt for user input
}

void loop() {
  char buffer[bufferSize]; // Declare buffer in the main loop

  fillBuffer(buffer);      // Call the function to fill the buffer
  processCommand(buffer);  // Process the received command

  Serial.print("> ");      // Print a prompt for the next user input
}
