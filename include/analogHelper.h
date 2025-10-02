#ifndef ANALOG_H
#define ANALOG_H


#include <avr/io.h>

void ADC_init() {
  ADMUX = (1 << REFS0) ; // Set Vref to AVcc 
  ADCSRA =  (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
  ADCSRB = 0x00; 
}



unsigned int ADC_read(unsigned char chnl){
  ADMUX  = (1 << REFS0) | (chnl & 0x0F); /*set MUX3:0 bits without modifying any other bits*/
  ADCSRA |= (1 << ADSC);/*set the bit that starts conversion in free running mode without modifying any other bit*/
  while((ADCSRA >> 6)&0x01) {}
    

  uint8_t low, high;

  low = ADCL; /*what should this get assigned with?*/
  high = ADCH; /*what should this get assigned with?*/

  return ((high << 8) | low) ;
}

#endif