

#define PERIOD 64800   // wait time in seconds
#define WORK 60         // work time in seconds

//#define PERIOD 10
//#define WORK 10

#define MOS 1           // mosfet pin

uint32_t mainTimer, myTimer;

boolean state = false;

#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#define adc_disable() (ADCSRA &= ~(1<<ADEN)) // disable ADC (before power-off)
#define adc_enable()  (ADCSRA |=  (1<<ADEN)) // re-enable ADC

void setup() {
  mainTimer = PERIOD;

  for (byte i = 0; i < 6; i++) {
    pinMode(i, INPUT);
  }
  adc_disable();         

  wdt_reset();            // watchdog init
  wdt_enable(WDTO_1S);    
  // 15MS, 30MS, 60MS, 120MS, 250MS, 500MS, 1S, 2S, 4S, 8S

  WDTCR |= _BV(WDIE);     // allowing interrupts
  sei();                  // allowing interrupts
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); 
}

void loop() {
  mainTimer++;

  if (!state) {                           
    if ((long)mainTimer - myTimer > PERIOD) {   
      myTimer = mainTimer;                
      state = true;                       
      pinMode(MOS, OUTPUT);               
      digitalWrite(MOS, HIGH);            
    }
  } else {                                
    if ((long)mainTimer - myTimer > WORK) {     
      myTimer = mainTimer;                
      state = false;                      
      digitalWrite(MOS, LOW);            
      pinMode(MOS, INPUT);                
    }
  }

  sleep_enable();   
  sleep_cpu();      
}

ISR (WDT_vect) {
  WDTCR |= _BV(WDIE); 
}
