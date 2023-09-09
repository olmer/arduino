/*
  Blink
*/
#define EEPROM_D0 2
#define EEPROM_D7 9

#define WRITE_ADDRESS 10
#define WRITE_MEMORY 11

#define CLOCK 12

#define SIGNAL_DELAY 100


//OPCODES
#define LDA 1
#define ADD 2
#define SUB 3
#define STA 4
#define LDI 5
#define JMP 6
#define JC  7
#define JZ  8
#define JEQ 9
#define OUT 14
#define HLT 15

// the setup function runs once when you press reset or power the board
void setup() {
    for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
        pinMode(pin, OUTPUT);
    }
    
    digitalWrite(CLOCK, LOW);
    
    digitalWrite(WRITE_ADDRESS, LOW);
    digitalWrite(WRITE_MEMORY, LOW);
    
    pinMode(CLOCK, OUTPUT);
    pinMode(WRITE_ADDRESS, OUTPUT);
    pinMode(WRITE_MEMORY, OUTPUT);
    
    delay(SIGNAL_DELAY);



    writeRAM(0, LDI);
    writeRAM(1, 6);      
    
    writeRAM(2, SUB);
    writeRAM(3, 17); 

    writeRAM(4, JZ);
    writeRAM(5, 8); 

    writeRAM(6, JMP);
    writeRAM(7, 2); 

    writeRAM(8, LDI);
    writeRAM(9, 250); 

    writeRAM(10, ADD);
    writeRAM(11, 16); 
    
    writeRAM(12, JC);
    writeRAM(13, 20);
    
    writeRAM(14, JMP);   
    writeRAM(15, 10);   
    
    writeRAM(16, 1);
    writeRAM(17, 2);
    writeRAM(20, HLT);
}


void writeRAM(int address, int data) {
    setAddress(address);
    delay(SIGNAL_DELAY);
    storeInMemory(data);
    delay(SIGNAL_DELAY);
}

void setAddress(int address) {
    setBusData(address);
    delay(SIGNAL_DELAY);
    writeSignal(WRITE_ADDRESS);
}

void storeInMemory(int data) {
    setBusData(data);
    delay(SIGNAL_DELAY);
    writeSignal(WRITE_MEMORY);
}

void setBusData(int data) {
    for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
        digitalWrite(pin, data & 1);
        data = data >> 1;
    }
}

void writeSignal(int signal) {
    digitalWrite(signal, HIGH);
    delay(SIGNAL_DELAY);
    pulseClock();
    digitalWrite(signal, LOW);
}

void pulseClock() {
    digitalWrite(CLOCK, HIGH);
    delay(SIGNAL_DELAY);
    digitalWrite(CLOCK, LOW);
}

// the loop function runs over and over again forever
void loop() {
    
}
