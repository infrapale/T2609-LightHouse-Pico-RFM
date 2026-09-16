#include "main.h"
#include "io.h"
#include <Arduino.h>
#include "relay.h"
#include "atask.h"


typedef enum {
   RELAY_STATE_OFF = 0,
   RELAY_STATE_ON,
   RELAY_STATE_ON_TIMER,
   RELAY_STATE_NBR_OF
} relay_state_et;


typedef struct {
   relay_state_et state;
   uint16_t       timer_cntr;
   bool           updated;
   bool           is_on;
   uint8_t        cntr_10ms;
} relay_status_st;

typedef struct {
   uint8_t tick_cntr;
} relay_cntrl_st;

//atask_st rfm_receive_handle      = {"Receive <- RFM ", 100,0, 0, 255, 0, 1, rfm_receive_task};
atask_st relay_th                  = {"Relay 100ms    ", 100,0, 0, 255, 0, 1, relay_do_every_100ms};

relay_cntrl_st relay_ctrl = {0};

relay_status_st relay_status[MAX_RELAY]; 

uint8_t relay_off_on[MAX_RELAY][2]={
    {RELAY_1A,RELAY_1B},
    {RELAY_2A,RELAY_2B},      
};

void relay_initialize(void) {
   for (byte i=0;i < MAX_RELAY;i++){
      pinMode( relay_off_on[i][0], OUTPUT); 
      pinMode( relay_off_on[i][1], OUTPUT); 
      digitalWrite(relay_off_on[i][0],LOW);
      digitalWrite(relay_off_on[i][1],LOW);

      relay_status[i].is_on = false; 
      relay_status[i].cntr_10ms = 0; 
   }
    atask_add_new(&relay_th);

   //relay_test();
}

void relay_test(void)
{
   for (byte n=0; n<10; n++)
   {
      for (byte i=0;i < MAX_RELAY;i++)
      {
         for (byte j = 0; j<2; j++)
         {
            digitalWrite(relay_off_on[i][j],LOW);
            delay(50);
            digitalWrite(relay_off_on[i][j],HIGH);
            Serial.println(relay_off_on[i][j]);
            delay(1000);
            digitalWrite(relay_off_on[i][j],LOW);
            delay(1000);
            }
      }
   }
}

void  relay_toggle(byte relay_indx) {
   if( relay_status[relay_indx].state == RELAY_STATE_ON)
      relay_status[relay_indx].state = RELAY_STATE_OFF;
   else 
      relay_status[relay_indx].state = RELAY_STATE_OFF;
   relay_status[relay_indx].updated = true;
};

void relay_turn_on(byte relay_indx) {
      relay_status[relay_indx].state = RELAY_STATE_ON;
      relay_status[relay_indx].updated = true;

      // relay_status[relay_indx].is_on = true; 
      // relay_status[relay_indx].cntr_10ms = SWITCH_TIME_x10ms; 
};

void relay_turn_off(byte relay_indx) {
      relay_status[relay_indx].state = RELAY_STATE_OFF;
      relay_status[relay_indx].updated = true;

      // relay_status[relay_indx].is_on = false; 
      // relay_status[relay_indx].cntr_10ms = SWITCH_TIME_x10ms; 
};

void relay_on_of_timer(uint8_t relay_indx, uint16_t seconds)
{
   if(seconds == 0) relay_status[relay_indx].state = RELAY_STATE_OFF;
   else if (seconds == 1) relay_status[relay_indx].state = RELAY_STATE_ON;
   else {
      relay_status[relay_indx].timer_cntr = seconds;
      relay_status[relay_indx].state = RELAY_STATE_ON_TIMER;
   }
   relay_status[relay_indx].updated = true;
}

void relay_do_every_100ms(void){
   byte i;

   if(++relay_ctrl.tick_cntr > 9){
      relay_ctrl.tick_cntr = 0;
      for(i=0; i<MAX_RELAY; i++){
         if(relay_status[i].state == RELAY_STATE_ON_TIMER) {
            if(relay_status[i].timer_cntr > 0) {
               if(--relay_status[i].timer_cntr == 0) {
                  relay_status[i].state = RELAY_STATE_OFF;
                  relay_status[i].updated = true;
               }
            }
         }
      }
   }

   for(i=0; i<MAX_RELAY; i++){
      if(relay_status[i].updated){
         if((relay_status[i].state == RELAY_STATE_ON) || (relay_status[i].state == RELAY_STATE_ON_TIMER))
            digitalWrite(relay_off_on[i][1],HIGH);
         else 
            digitalWrite(relay_off_on[i][0],HIGH);
         relay_status[i].updated = false;
      }
      else {
         digitalWrite(relay_off_on[i][0],LOW);
         digitalWrite(relay_off_on[i][1],LOW);
      }
   }

   // for(i=0; i<MAX_RELAY; i++){
   //    if (relay_status[i].cntr_10ms > 0){
   //       relay_status[i].cntr_10ms--;
   //       if (relay_status[i].cntr_10ms > 0){
   //          if (relay_status[i].is_on) digitalWrite(relay_off_on[i][1],HIGH);
   //          else digitalWrite(relay_off_on[i][0],HIGH); 
   //       } 
   //       else {
   //          digitalWrite(relay_off_on[i][0],LOW);
   //          digitalWrite(relay_off_on[i][1],LOW);
   //       }
   //    }
   // } 
}
