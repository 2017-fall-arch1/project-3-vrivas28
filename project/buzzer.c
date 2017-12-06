#include <msp430.h>
#include "libTimer.h"
#include "buzzer.h"

static long counter = 0;
static long temp = 0;	

#define MIN_PERIOD 1000
#define MAX_PERIOD 4000

void buzzer_init()
{
    /* 
       Direct timer A output "TA0.1" to P2.6.  
        According to table 21 from data sheet:
          P2SEL2.6, P2SEL2.7, anmd P2SEL.7 must be zero
          P2SEL.6 must be 1
        Also: P2.6 direction must be output
    */
    timerAUpmode();		/* used to drive speaker */
    P2SEL2 &= ~(BIT6 | BIT7);
    P2SEL &= ~BIT7; 
    P2SEL |= BIT6;
    P2DIR = BIT6;		/* enable output to speaker (P2.6) */

}
void play(char choice)
{
    switch (choice)
    {
        case 1:
            playBn();
            break;
        case 2:
            playCF();
            break;
        case 3: 
            playE();
            break;
    }
}
void playBn()
{
    temp = 1400;
    buzzer_set_period(1975);
    while(++counter < temp){}
    counter = 0; 
}
void playCF()
{
    temp = 1400;
    buzzer_set_period(1108);
    while(++counter < temp){}
    counter = 0; 
}

void playE()
{
    temp = 1400;
    buzzer_set_period(1318);
    while(++counter < temp){}
    counter = 0;
}
void buzzer_set_period(short cycles)
{
  CCR0 = cycles; 
  CCR1 = cycles >> 1;		/* one half cycle */
}


    
    
  

