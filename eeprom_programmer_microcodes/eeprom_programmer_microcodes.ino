/**
 * This sketch programs the microcode EEPROMs for the 8-bit breadboard computer
 * It includes support for a flags register with carry and zero flags
 * See this video for more: https://youtu.be/Zg1NdPKoosU
 */
#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define SHIFT_LATCH 4
#define EEPROM_D0 5
#define EEPROM_D7 12
#define WRITE_EN 13

/*
 * order of bits here is backwards to EEPROM pins. First half is left eeprom, second half is right
 */
#define HLTM 0b1000000000000000  // Halt clock                   // PIN 17 - IO7 - 1000_0000_0000_0000 - 80
#define MI   0b0100000000000000  // Memory address register in   // PIN 16 - IO6 - 0100_0000_0000_0000 - 40
#define RI   0b0010000000000000  // RAM data in                  // PIN 15 - IO5 - 0010_0000_0000_0000 - 20
#define RO   0b0001000000000000  // RAM data out                 // PIN 14 - IO4 - 0001_0000_0000_0000 - 10
#define NN   0b0000100000000000  // Not used                     // PIN 13 - IO3
#define II   0b0000010000000000  // Instruction register in      // PIN 11 - IO2 - 0000_0100_0000_0000 - 04
#define AI   0b0000001000000000  // A register in                // PIN 10 - IO1 - 0000_0010_0000_0000 - 02
#define AO   0b0000000100000000  // A register out               // PIN 9  - IO0 - 0000_0001_0000_0000 - 01 - next goes after address 080
#define EO   0b0000000010000000  // ALU out                      // PIN 17 - IO7 - 0000_0000_1000_0000 - 80
#define SU   0b0000000001000000  // ALU subtract                 // PIN 16 - IO6 - 0000_0000_0100_0000 - 40
#define BI   0b0000000000100000  // B register in                // PIN 15 - IO5 - 0000_0000_0010_0000 - 20
#define OI   0b0000000000010000  // Output register in           // PIN 14 - IO4 - 0000_0000_0001_0000 - 10
#define CE   0b0000000000001000  // Program counter enable       // PIN 13 - IO3 - 0000_0000_0000_1000 - 08
#define CO   0b0000000000000100  // Program counter out          // PIN 11 - IO2 - 0000_0000_0000_0100 - 04
#define J    0b0000000000000010  // Jump (program counter in)    // PIN 10 - IO1 - 0000_0000_0000_0010 - 02
#define RS   0b0000000000000001  // Microcode counter reset      // PIN 9  - IO0 - 0000_0000_0000_0001 - 01

/*
 * NOP                                          LDA
 * 000:  40 14    00 00 00 00 00 00             40 14    40 50    12    00    00 00
 * 080:  04 08    01 00 00 00 00 00             04 08    04 08    00    01    00 00
 *       MI RO|II                               MI RO|II MI MI|RO RO|AI 
 *       CO CE    RS                            CO CE    CO CE          RS
 *       
 *       
 * ADD                                          SUB
 * 010:  40 14    40 50    10 02 00 00          40 14    40 50    10    02    00 00
 * 090:  04 08    04 08    20 80 01 00          04 08    04 08    20    c0    01 00
 *       MI RO|II MI MI|RO RO AI                MI RO|II MI MI|RO RO    AI
 *       CO CE    CO CE    BI EO RS             CO CE    CO CE    BI    SU|EO RS
 *       
 *       
 * STA                                          LDI
 * 020:  40 14    40    50    21    00 00 00    40 14    40    12    00    00 00 00
 * 0a0:  04 08    04    08    00    01 00 00    04 08    04    08    01    00 00 00
 *       MI RO|II MI    RO|MI AO|RI             MI RO|II MI    RO|AI
 *       CO CE    CO    CE          RS          CO CE    CO    CE    RS
 *       
 *       
 * JMP                                          JC
 * 030:  40 14    40    10    00    00 00 00    40 14    00    00    00    00 00 00
 * 0b0:  04 08    04    02    01    00 00 00    04 08    08    01    00    00 00 00
 *       MI RO|II MI    RO                      MI RO|II
 *       CO CE    CO    J     RS                CO CE    CE    RS
 * 
 * 
 * JZ                                           JEQ
 * 040:  40 14    00    00    00    00 00 00    40 14     40    10    00    00 00 00
 * 0c0:  04 08    08    01    00    00 00 00    04 08     04    28    c0    08 01 00
 *                -                                       MI    RO
 *                CE    RS                                CO    BI|CE SU|EO CE RS
 * 
 * NOP
 * 050:  40 14 00 00 00 00 00 00   40 14 00 00 00 00 00 00
 * 0d0:  04 08 00 00 00 00 00 00   04 08 00 00 00 00 00 00
 * 
 * NOP
 * 060:  40 14 00 00 00 00 00 00   40 14 00 00 00 00 00 00
 * 0e0:  04 08 00 00 00 00 00 00   04 08 00 00 00 00 00 00
 * 
 * OUT                             HLT
 * 070:  40 14 01 00 00 00 00 00   40 14 80 00 00 00 00 00
 * 0f0:  04 08 10 01 00 00 00 00   04 08 00 00 00 00 00 00
 */

#define FLAGS_Z0C0 0
#define FLAGS_Z0C1 1
#define FLAGS_Z1C0 2
#define FLAGS_Z1C1 3

#define JC   0b0111
#define JZ   0b1000
#define JEQ  0b1001

const PROGMEM uint16_t UCODE_TEMPLATE[16][8] = {
  { CO|MI,  RO|II|CE,  RS,     0,        0,        0,        0,     0 },   // 0000 - NOP
  { CO|MI,  RO|II|CE,  CO|MI,  RO|MI|CE, RO|AI,    RS,       0,     0 },   // 0001 - LDA
  { CO|MI,  RO|II|CE,  CO|MI,  RO|MI|CE, RO|BI,    EO|AI,    RS,    0 },   // 0010 - ADD
  { CO|MI,  RO|II|CE,  CO|MI,  RO|MI|CE, RO|BI,    EO|AI|SU, RS,    0 },   // 0011 - SUB
  { CO|MI,  RO|II|CE,  CO|MI,  RO|MI|CE, AO|RI,    RS,       0,     0 },   // 0100 - STA
  { CO|MI,  RO|II|CE,  CO|MI,  RO|AI|CE, RS,       0,        0,     0 },   // 0101 - LDI
  { CO|MI,  RO|II|CE,  CO|MI,  RO|J,     RS,       0,        0,     0 },   // 0110 - JMP
  { CO|MI,  RO|II|CE,  CE,     RS,       0,        0,        0,     0 },   // 0111 - JC
  { CO|MI,  RO|II|CE,  CE,     RS,       0,        0,        0,     0 },   // 1000 - JZ
  { CO|MI,  RO|II|CE,  CO|MI,  RO|MI|CE, RO|BI,    EO|SU,    CE,    0 },   // 1001 - JEQ
  { CO|MI,  RO|II|CE,  CO|MI,  RO|BI|CE, EO|AI,    RS,       0,     0 },   // 1010 - ADI
  { CO|MI,  RO|II|CE,  0,      0,        0,        0,        0,     0 },   // 1011
  { CO|MI,  RO|II|CE,  0,      0,        0,        0,        0,     0 },   // 1100
  { CO|MI,  RO|II|CE,  0,      0,        0,        0,        0,     0 },   // 1101
  { CO|MI,  RO|II|CE,  AO|OI,  RS,       0,        0,        0,     0 },   // 1110 - OUT
  { CO|MI,  RO|II|CE,  HLTM,   0,        0,        0,        0,     0 },   // 1111 - HLT
};

uint16_t ucode[4][16][8];

/*
 * microcode fixes:
 * JC: remove CE in jump carry on CF=1
 * JEQ: move CE to prev micro instruction
 */

void initUCode() {
  // ZF = 0, CF = 0
  memcpy_P(ucode[FLAGS_Z0C0], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));

  // ZF = 0, CF = 1
  memcpy_P(ucode[FLAGS_Z0C1], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAGS_Z0C1][JC][2] = CO|MI;
  ucode[FLAGS_Z0C1][JC][3] = RO|J;
  ucode[FLAGS_Z0C1][JC][4] = RS;

  // ZF = 1, CF = 0
  memcpy_P(ucode[FLAGS_Z1C0], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAGS_Z1C0][JZ][2] = CO|MI;
  ucode[FLAGS_Z1C0][JZ][3] = RO|J;
  ucode[FLAGS_Z1C0][JZ][4] = RS;
  
  ucode[FLAGS_Z1C0][JEQ][6] = CO|MI;
  ucode[FLAGS_Z1C0][JEQ][7] = RO|J|CE;

  // ZF = 1, CF = 1
  memcpy_P(ucode[FLAGS_Z1C1], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAGS_Z1C1][JC][2] = CO|MI;
  ucode[FLAGS_Z1C1][JC][3] = RO|J;
  ucode[FLAGS_Z1C1][JC][4] = RS;
  
  ucode[FLAGS_Z1C1][JZ][2] = CO|MI;
  ucode[FLAGS_Z1C1][JZ][3] = RO|J;
  ucode[FLAGS_Z1C1][JZ][4] = RS;
  
  ucode[FLAGS_Z1C1][JEQ][6] = CO|MI;
  ucode[FLAGS_Z1C1][JEQ][7] = RO|J|CE;
}

/*
 * Output the address bits and outputEnable signal using shift registers.
 */
void setAddress(int address, bool outputEnable) {
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, (address >> 8) | (outputEnable ? 0x00 : 0x80));
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, address);

  digitalWrite(SHIFT_LATCH, LOW);
  digitalWrite(SHIFT_LATCH, HIGH);
  digitalWrite(SHIFT_LATCH, LOW);
}


/*
 * Read a byte from the EEPROM at the specified address.
 */
byte readEEPROM(int address) {
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    pinMode(pin, INPUT);
  }
  setAddress(address, /*outputEnable*/ true);

  byte data = 0;
  for (int pin = EEPROM_D7; pin >= EEPROM_D0; pin -= 1) {
    data = (data << 1) + digitalRead(pin);
  }
  return data;
}


/*
 * Write a byte to the EEPROM at the specified address.
 */
void writeEEPROM(int address, byte data) {
  byte origData = data;
  setAddress(address, /*outputEnable*/ false);
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    pinMode(pin, OUTPUT);
  }

  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    digitalWrite(pin, data & 1);
    data = data >> 1;
  }
  digitalWrite(WRITE_EN, LOW);
  delayMicroseconds(1);
  digitalWrite(WRITE_EN, HIGH);
  while (readEEPROM(address) != origData) {
    delay(1);
  }
}


/*
 * Read the contents of the EEPROM and print them to the serial monitor.
 */
void printContents(int start, int length) {
  for (int base = start; base < length; base += 16) {
    byte data[16];
    for (int offset = 0; offset <= 15; offset += 1) {
      data[offset] = readEEPROM(base + offset);
    }

    char buf[80];
    sprintf(buf, "%03x:  %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

    Serial.println(buf);
  }
}


void setup() {
  // put your setup code here, to run once:
  initUCode();

  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);
  digitalWrite(WRITE_EN, HIGH);
  pinMode(WRITE_EN, OUTPUT);
  Serial.begin(57600);

  // Program data bytes
  Serial.print("Programming EEPROM");

  // Program the 8 high-order bits of microcode into the first 128 bytes of EEPROM
  for (int address = 0; address < 1024; address += 1) {
    int flags       = (address & 0b1100000000) >> 8;
    int byte_sel    = (address & 0b0010000000) >> 7;
    int instruction = (address & 0b0001111000) >> 3;
    int step        = (address & 0b0000000111);

    if (byte_sel) {
//      writeEEPROM(address, ucode[flags][instruction][step]);
    } else {
//      writeEEPROM(address, ucode[flags][instruction][step] >> 8);
    }

    if (address % 64 == 0) {
      Serial.print(".");
    }
  }

  Serial.println(" done");


  // Read and print out the contents of the EERPROM
  Serial.println("Reading EEPROM");
  printContents(0, 1024);
}


void loop() {
  // put your main code here, to run repeatedly:

}
