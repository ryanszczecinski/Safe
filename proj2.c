#ifndef F_CPU
#define F_CPU 1000000UL
#endif

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define SPEED 0x02
#define LED_PIN0 (1 << PB0)
#define LED_PIN1 (1 << PB1)
#define LED_PIN2 (1 << PB2)
#define KEY0 (1<<PB4)
#define KEY1 (1<<PB3)

uint8_t count = 0;
uint8_t led_mask = 0b0000;
typedef enum {RELEASED =0, PRESSED} key;
key key0 = RELEASED;
key key1 = RELEASED;
key key0Prev = RELEASED;
key key1Prev = RELEASED;
typedef enum {PROGRAM, LOCKED} states;
states state = RELEASED;
uint8_t savedState = 0;
uint8_t input = 0;
uint16_t sinceLastButtonPress =0;
int inputCount = 0;


ISR(TIMER0_COMPA_vect){
  //will alternate the leds which need to be turned on at a fast pace
  if(count%4 ==0 && (led_mask & 0b1000) != 0){
    //yellow
    PORTB = 0b110;
  }
  else if(count%4 ==1 && (led_mask & 0b0100) != 0){
    //green
    PORTB = 0b001;
  }
  else if(count%4 ==2 && (led_mask & 0b0010) != 0){
    //blue
    PORTB = 0b011;
  }
  else if(count%4 ==3 && (led_mask & 0b0001) != 0){
    //red
    PORTB = 0b100;
  }
  count +=1;
  sinceLastButtonPress +=1;
}
//blink the led three times
void flashYellow(){
  //be careful, the code will be stuck here for 1.5 seconds
  //and will not update anything else
  led_mask |= 0b1000;
  _delay_ms(300);
  led_mask &= 0b0111;
  _delay_ms(200);

  led_mask |= 0b1000;
  _delay_ms(300);
  led_mask &=0b0111;
  _delay_ms(200);

  led_mask |= 0b1000;
  _delay_ms(300);
  led_mask &=0b0111;
  _delay_ms(200);
}


//state changing logic
//assume that there will not be a case where both are pressed at once
void setState(){
  //will trigger on a button press, not a button hold
  if(key0Prev == RELEASED && key0 == PRESSED){
      input = input << 1;
      inputCount +=1;
  }
  key0Prev = key0; //remember the previous value of key0
  if(key1Prev == RELEASED && key1 == PRESSED){
      input = input << 1;
      input |= 0b1;
      inputCount +=1;
  }
  key1Prev = key1;
  //has timed out BONUS wooo ...EZ
  if(sinceLastButtonPress>1000 && inputCount>0){
    input = 0;
    inputCount = 0;
    sinceLastButtonPress = 0;
    flashYellow();
  }

  //this means we have 6 inputs
  //state only changes at the end of 6 bits being enterd
  if(inputCount != 0 && inputCount % 6 ==0 ){
    if(state == PROGRAM){
      savedState = input;
      input = 0;
      state = LOCKED;
      inputCount = 0;
    }
    else{
      if (input == savedState){
        savedState = 0;
        input = 0;
        state = PROGRAM;
      }
      else{
         input = 0;
         flashYellow();
      }
      inputCount =0;
    }
  }
}
//adjusts the led mask depending on state and input pins
void adjustMask(){
  if(state == PROGRAM){
    led_mask = 0b1100;
  }
  else{
     led_mask = 0b0001;
  }
  if(key0 == PRESSED || key1 == PRESSED){
    led_mask |= 0b0010;
  }
  else led_mask &= 1101;

}


int main(){
  DDRB = LED_PIN0 | LED_PIN1 | LED_PIN2;
  PORTB = 0;
  led_mask = 0b1100; //initial program state display
  OCR0A = SPEED;
  TCCR0A = 0b00000010; //the last two bits make the timer reset everytime it hits OCR0A
  TCCR0B = 0b0101; //1024
  TIMSK = (1<<OCIE0A);
  uint8_t historyK0 = 0;
  uint8_t historyK1 = 0;
  sei();
  sinceLastButtonPress = 0;
  while(1){
    historyK0 = historyK0 <<1;
    if((PINB & KEY0) == 0) historyK0 = historyK0 | 0x1;
    if ((historyK0 & 0b111111) == 0b111111){
       key0 = PRESSED;
       sinceLastButtonPress = 0;
    }
    else key0 = RELEASED;

    historyK1 = historyK1 <<1;
    if((PINB & KEY1) == 0) historyK1 = historyK1 | 0x1;
    if ((historyK1 & 0b111111) == 0b111111){
       key1 = PRESSED;
       sinceLastButtonPress = 0;
    }
    else key1 = RELEASED;
    setState();
    adjustMask();


  }


}
