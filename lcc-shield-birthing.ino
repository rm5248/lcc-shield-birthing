#include <M95_EEPROM.h>

#include <SPI.h>

// Older boards
//static const byte EEPROM_CS = 10;
//M95_EEPROM eeprom(&SPI, EEPROM_CS, 64);

// Nov 2023 boards
static const byte EEPROM_CS = 7;
M95_EEPROM eeprom(SPI, EEPROM_CS, 256, 3, true);

#define ID_VERSION 1
#define MANUFACTURER "Snowball Creek Electronics"
#define PART_NUMBER "SCE-230900"
#define HARDWARE_REV "Rev.3"

struct id_page{
  uint64_t node_id;
  uint16_t id_version;
  char manufacturer[32];
  char part_number[21];
  char hw_version[12];
};

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
  struct id_page id;

  Serial.begin (9600) ;
  while (!Serial) {
    delay (50) ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  }

  pinMode(EEPROM_CS, OUTPUT);

  SPI.begin();
  eeprom.begin();

  if(!eeprom.exists()){
    Serial.println(F("Unable to find EEPROM: PANIC!"));
    while(1){}
  }

  Serial.print("Status register: ");
  Serial.println(eeprom.status_register(), HEX);

  Serial.print("ID page locked? ");
  Serial.println(eeprom.id_page_locked());

  uint64_t node_id;
  eeprom.read_id_page(sizeof(id), &id);
  node_id = id.node_id;

  Serial.print("LCC ID: ");
  print_node_id(node_id);
  Serial.println();
  Serial.print("ID page version: ");
  Serial.println(id.id_version);
  Serial.print("Manufacturer: ");
  Serial.println(id.manufacturer);
  Serial.print("Part number: ");
  Serial.println(id.part_number);
  Serial.print("HW Version: ");
  Serial.println(id.hw_version);

  if(eeprom.id_page_locked()){
    Serial.println("ID page locked, cannot change");
    return;
  }

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
      memset(&id, 0, sizeof(id));
      id.node_id = new_node_id;
      id.id_version = ID_VERSION;
      memcpy(id.manufacturer, MANUFACTURER, strlen(MANUFACTURER));
      memcpy(id.part_number, PART_NUMBER, strlen(PART_NUMBER));
      memcpy(id.hw_version, HARDWARE_REV, strlen(HARDWARE_REV));
      eeprom.write_id_page(sizeof(id), &id);
      Serial.println("New node ID set");

      while(Serial.available()){
        Serial.read();
      }

      Serial.println();
      Serial.println("Type 'y' to lock the ID page");
      while(Serial.available() == 0){}
      response = Serial.read();
      if(response =='y'){
        eeprom.lock_id_page();
        Serial.println("ID page locked!");
      }else{
        Serial.println("ID page not locked");
      }

    }else{
      Serial.println("Node ID not set");
    }
  }

  Serial.println("Birthing complete");
}

void loop() {
  // put your main code here, to run repeatedly:

}
