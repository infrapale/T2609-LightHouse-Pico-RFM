#ifndef __RELAY_H__
#define __RELAY_H__

#define SWITCH_TIME_x10ms 5


void relay_initialize(void);

void relay_test(void);

void  relay_toggle(byte relay_indx);

void relay_turn_on(byte relay_indx); 

void relay_turn_off(byte relay_indx);

void relay_on_of_timer(uint8_t relay_indx, uint16_t seconds);

void relay_do_every_100ms(void);

#endif