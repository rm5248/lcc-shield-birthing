#include <SPI.h>

static const byte EEPROM_CS = 10;
int found_eeprom = 0;

static const byte EEPROM_WRITE_ENABLE = 0x6;
static const byte EEPROM_READ_STATUS_REGISTER = 0x5;
static const byte EEPROM_WRITE_STATUS_REGISTER = 0x1;
static const byte EEPROM_READ_MEMORY_ARRAY = 0x3;
static const byte EEPROM_WRITE_MEMORY_ARRAY = 0x2;
static const byte EEPROM_WRITE_DISABLE = 0x4;

static const int LCC_UNIQUE_ID_ADDR = 0x6000;

// Memory information:
// page size: 64 bytes
// Proection can be: whole, half, upper quarter
//
// upper quarter(starting at 0x6000):
// 0x6000 - 0x6008 - LCC ID unique ID

void find_eeprom(){
  digitalWrite(EEPROM_CS, LOW);
  SPI.transfer(EEPROM_WRITE_ENABLE);
  digitalWrite(EEPROM_CS, HIGH);

  delay(5);

  digitalWrite(EEPROM_CS, LOW);
  SPI.transfer(EEPROM_READ_STATUS_REGISTER);
  int read_status_reg = SPI.transfer(0xFF);
  digitalWrite(EEPROM_CS, HIGH);

  delay(5);

  if(read_status_reg != 0xFF &&
     read_status_reg & (0x01 << 1)){
    // WEL bit is set, so we are talking with the EEPROM!
    // Let's go and disable it again
    found_eeprom = 1;
    digitalWrite(EEPROM_CS, LOW);
    SPI.transfer(EEPROM_WRITE_DISABLE);
    digitalWrite(EEPROM_CS, HIGH);
    Serial.println("Found EEPROM!");
  }
}

void eeprom_read_statusreg(){
  digitalWrite(EEPROM_CS, LOW);
  SPI.transfer(EEPROM_READ_STATUS_REGISTER);
  int read_status_reg = SPI.transfer(0xFF);
  digitalWrite(EEPROM_CS, HIGH);

  Serial.print("Status register: ");
  Serial.print(read_status_reg, HEX);
  Serial.println();
}

void eeprom_read(int offset, void* data, int numBytes){
  digitalWrite(EEPROM_CS, LOW);
  SPI.transfer(EEPROM_READ_MEMORY_ARRAY);

  SPI.transfer((offset & 0xFF00) >> 8);
  SPI.transfer((offset & 0x00FF) >> 0);

  uint8_t* u8_data = data;
  while(numBytes > 0){
    numBytes--;
    *u8_data = SPI.transfer(0xFF); // dummy byte
    u8_data++;
  }
  Serial.println();

  digitalWrite(EEPROM_CS, HIGH);
}

void eeprom_write(int offset, void* data, int numBytes){
  digitalWrite(EEPROM_CS, LOW);
  SPI.transfer(EEPROM_WRITE_ENABLE);
  digitalWrite(EEPROM_CS, HIGH);

  delay(5);

  eeprom_read_statusreg();

  digitalWrite(EEPROM_CS, LOW);
  SPI.transfer(EEPROM_WRITE_MEMORY_ARRAY);
  SPI.transfer((offset & 0xFF00) >> 8);
  SPI.transfer((offset & 0x00FF) >> 0);

  uint8_t* u8_data = data;
  while(numBytes > 0){
    numBytes--;
    SPI.transfer(*u8_data); // data byte
    u8_data++;
  }

  digitalWrite(EEPROM_CS, HIGH);
}

void print_node_id(uint64_t node_id){
    char buffer[3];
    sprintf(buffer, "%02X", (int)((node_id & 0xFF0000000000l) >> 40));
    Serial.print(buffer[0]);
    Serial.print(buffer[1]);
    sprintf(buffer, "%02X", (int)((node_id & 0x00FF00000000l) >> 32));
    Serial.print(buffer[0]);
    Serial.print(buffer[1]);
    sprintf(buffer, "%02X", (int)((node_id & 0x0000FF000000l) >> 24));
    Serial.print(buffer[0]);
    Serial.print(buffer[1]);
    sprintf(buffer, "%02X", (int)((node_id & 0x000000FF0000l) >> 16));
    Serial.print(buffer[0]);
    Serial.print(buffer[1]);
    sprintf(buffer, "%02X", (int)((node_id & 0x00000000FF00l) >> 8));
    Serial.print(buffer[0]);
    Serial.print(buffer[1]);
    sprintf(buffer, "%02X", (int)((node_id & 0x0000000000FFl) >> 0));
    Serial.print(buffer[0]);
    Serial.print(buffer[1]);
}

void setup() {
  Serial.begin (9600) ;
  while (!Serial) {
    delay (50) ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  }

  SPI.begin();

  find_eeprom();

  if(!found_eeprom){
    Serial.println(F("Unable to find EEPROM: PANIC!"));
    while(1){}
  }

  uint64_t node_id;
  eeprom_read(LCC_UNIQUE_ID_ADDR, &node_id, 8);

  Serial.print("LCC ID: ");
  print_node_id(node_id);
  Serial.println();

  if(node_id != 0l){
    Serial.println("Type 'c' within 5 seconds to change node ID");
    Serial.setTimeout(5000);
    String value = Serial.readStringUntil('\n');
    if(value.charAt(0) == 'c'){
      Serial.println("change!");
      node_id = 0l;
    }
  }

  if(node_id == 0l){
    Serial.setTimeout(0xFFFFFF);
    Serial.println("Node ID not set, please input new node ID followed by newline.");
    String nodeId = Serial.readStringUntil('\n');

    if(nodeId.length() != 12){
      Serial.println("Length not equal to 12, invalid");
      while(1){}
    }

    uint64_t new_node_id = 0l;
    // Convert the string to a uint64_t
    String upperBits = nodeId.substring(0, 4);
    String lowerBits = nodeId.substring(4, 12);
    new_node_id = strtol(upperBits.c_str(), 0, 16);// | strtol(lowerBits.c_str(), 0, 16);
    new_node_id = (new_node_id << 32l);
    new_node_id |= strtol(lowerBits.c_str(), 0, 16);

    Serial.print("New node id will be ");
    print_node_id(new_node_id);

    Serial.println();
    Serial.println("Type 'y' to accept");
    while(Serial.available() == 0){}
    int response = Serial.read();

    if(response == 'y'){
      eeprom_read_statusreg();
      eeprom_write(LCC_UNIQUE_ID_ADDR, &new_node_id, 8);
      eeprom_read_statusreg();
      delay(5);
      eeprom_read_statusreg();
      Serial.println("New node ID set");
    }else{
      Serial.println("Node ID not set");
    }
  }

  Serial.println("Birthing complete");
}

void loop() {
  // put your main code here, to run repeatedly:

}
