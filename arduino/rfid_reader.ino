/*
 BUZZCARD HID RFID Format: Standard corp 1k
   - http://www.midwestbas.com/store/media/pdf/infinias/customer_wiegand_card_formats.pdf

  PIN # decleration
*/
#define beeperPin 4
#define redPin 5 // Red LED
#define greenPin 6 // GREN LED
#define holdPin 7 //Stop the RFID Reader

/*
  RFID parsing information
*/
#define id_start_bit 14 //The starting bit of the Id stored in buzzcard
#define id_bit_length 20 //Bit Length of ID stored in buzzcard
#define BUZZCARD_BIT_LENGTH 35 //How many bits (total) are stored in the buzzcard
/*
  Static Globals
*/
static volatile byte RFID_Data[35]; //Have to store raw RFID data as array as its too long to fit in a natural arduino datatype
static volatile int RFID_bitsRead = 0; //How many bits have we read so far?
static int hold = false;

#include <SPI.h>
#include <boards.h>
#include <ble_shield.h>
#include <services.h> 

void setup()
{
  ble_begin();
  Serial.begin(57600);

  // Attach pin change interrupt service routines from the Wiegand RFID readers
  attachInterrupt(0, dataZero_High, RISING);//DATA0 to pin 2 - data
  attachInterrupt(4, dataOne_High, RISING); //DATA1 to pin 19 - clock
  delay(10);
  
  /* Beeper Setup */
  pinMode(beeperPin, OUTPUT);
  digitalWrite(beeperPin, HIGH);
  /* Red LED Setup */
  pinMode(redPin, OUTPUT);
  digitalWrite(redPin, HIGH);
  /* Green LED Setup */
  pinMode(greenPin, OUTPUT);
  digitalWrite(greenPin, HIGH);
  /* RFID Hold Setup */
  pinMode(holdPin, OUTPUT);
  digitalWrite(holdPin, HIGH);

  // put the reader input variables to zero
  for(int i = 0; i < 35; i++) {
    RFID_Data[i] = 0x00;
  }
  RFID_bitsRead = 0;
}

void loop() {
  if(RFID_bitsRead == BUZZCARD_BIT_LENGTH) {
    RFID_do_events();
  /*
    If we read more than BUZZCARD_BIT_LENGTH,
    then we've had an error somewhere.  
    Therefore we pause and reset
  */
  } else if(RFID_bitsRead > BUZZCARD_BIT_LENGTH) { 
    delay(3500); //STOP Executing for ~3.5 secs..aka reset
    RFID_reset();
  }

  /*
    When not being interrupted by RFID Reader, 
    and done with parsing of RFID codes....
    we do our ble events (advertising, reading, writing)
  */
  if(!hold) {
    ble_do_events();
  }
}


/*
  HARDWARE INTERRUPTS
 */
void dataZero_High(void) { //Binary0
  RFID_Data[RFID_bitsRead] = 0;
  ++RFID_bitsRead;
  //Put on a hold
  if(!hold) {
    RFID_lock();
  }
}

void dataOne_High(void) { //Binary1
  RFID_Data[RFID_bitsRead] = 1;
  ++RFID_bitsRead;
  //Put on a hold
  if(!hold) {
    RFID_lock();
  }
}
/*
  Functions
*/
void RFID_do_events() {
  if(RFID_bitsRead >= 35) { 
    unsigned long Card_Data = parseId();
    Serial.println(Card_Data);

    sendId(Card_Data);
    RFID_reset();
  }
}

void RFID_lock() {
  Serial.println("Hold engaged");
  hold = true;
  digitalWrite(holdPin, LOW);
  digitalWrite(greenPin, LOW);
  digitalWrite(redPin, HIGH);
}

void RFID_reset() {
  RFID_bitsRead = 0;
  for(int i = 0; i < 35; ++i) {
    RFID_Data[i] = 0;
  }
  RFID_unlock(); //Resume RFID hardware interrupts will resume
}

void RFID_unlock() {
  Serial.println("Hold dis-engaged");
  hold = false;
  digitalWrite(holdPin, HIGH);
  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, HIGH);
}




unsigned long parseId() {
  unsigned long parsedId = 0;
  for(int i = id_start_bit; i < (id_start_bit + id_bit_length); i++) {
      parsedId <<= 1;
      parsedId |= RFID_Data[i] & 0x1; //RFID data stored in first bit
  }
  return parsedId;
}
/*
  Convert buzzcard ID to a string, with a newline terminator
*/
int id2str(char * destination, unsigned long id) {
 int length = sprintf(destination, "%lu\n", id);
 return length;
}

/*
  Send the buzzcard ID through Bluetooth LE
*/
void sendId(unsigned long id) {
  char buffer[11]; //max unsinged long = '4294967295\n'
  int length = id2str(buffer, id);
  for(int i = 0; i < length; ++i) {
    ble_write(buffer[i]);
  }
}

