#include "RIMS.h"

volatile unsigned char TimerFlag = 0; 

void TimerISR() {
   TimerFlag = 1;
}

//task struct
typedef struct task {
   int state;                  // Task's current state
   unsigned long period;       // Task period
   unsigned long elapsedTime;  // Time elapsed since last task tick
   int (*TickFct)(int);        // Task tick function
} task;

const unsigned char NUM_TASKS = 4; 
const unsigned short random[256] = {
    52748, 43591, 58480, 12297, 32149, 22497, 28849, 48098,
    16986, 64663, 34111, 11733, 49331, 34032, 34945, 59443,
    60114, 25050, 47212, 20893, 33035, 53767, 41297, 11686,
    65113, 32857, 54890, 37807, 42962, 18831, 37457, 34540,
    55306, 29097, 63878, 62189, 40700, 14425, 29003, 60253,
    32884, 26308, 65187, 23388, 59376, 14280, 41310, 64999,
    15742, 60298, 25376, 56945, 37589, 45080, 23053, 61483,
    47921, 38152, 28452, 40891, 63398, 54234, 54036, 35816,
    21025, 20595, 44060, 65154, 54383, 37237, 50991, 50238,
    49858, 57471, 25550, 36436, 42001, 51260, 34503, 38492,
    51746, 39627, 44733, 51966, 36413, 61296, 56825, 42228,
    62646, 33489, 50227, 29308, 51149, 36960, 33608, 47290,
    36032, 46690, 39991, 59010, 39301, 44413, 20696, 56584,
    32804, 43455, 51138, 35459, 41607, 43406, 41623, 33690,
    64278, 50943, 56314, 30918, 37941, 58912, 55627, 61066,
    48102, 27549, 47857, 22030, 34170, 39731, 52970, 62986,
    45353, 61394, 32699, 63887, 54731, 43695, 57785, 51504,
    64902, 53957, 43855, 38481, 40662, 58992, 51962, 64550,
    37932, 39430, 31255, 37182, 53246, 26461, 40303, 48928,
    61145, 50876, 47801, 54352, 46301, 26795, 40247, 54826,
    54093, 45991, 34879, 36118, 61917, 56074, 33573, 63292,
    42825, 54321, 58360, 36310, 48311, 60788, 34770, 39393,
    48877, 46976, 24849, 38538, 44375, 26515, 37626, 43462,
    45947, 65092, 36833, 40493, 35491, 49703, 31361, 65011,
    42102, 32083, 34307, 51520, 32561, 59534, 63123, 43898,
    61173, 39506, 41473, 45234, 33226, 27180, 56484, 27450,
    38709, 52514, 51758, 52109, 54324, 58646, 57193, 26049,
    31498, 55560, 42872, 55672, 58985, 54241, 43756, 57938,
    52647, 58758, 52757, 40948, 32341, 55038, 64356, 56408,
    64538, 29550, 59138, 62937, 63128, 42901, 37694, 53856,
    58836, 40255, 37154, 49618, 43720, 31953, 36537, 43481,
    35504, 42091, 53085, 38849, 64245, 46536, 44125, 27890,
    36614, 27570, 55326, 27871, 43887, 39356, 63486, 49496
};

task tasks[NUM_TASKS];

//task periods
const unsigned long SYSCONTROL_PERIOD = 100;
const unsigned long RANDOMLED_PERIOD = 50;
const unsigned long PLAYERS_PERIOD = 100;
const unsigned long SCOREOUT_PERIOD = 200;
const unsigned long GCD_PERIOD = 50;

//define tasks
enum sysControlStates {SC_start, SC_off, SC_press, SC_press};
enum randomLEDStates { RLED_start, RLED_wait, RLED_flash, RLED_off };
enum playersStates { P_start, P_wait, P_flash, P_off };
enum scoreOutStates { SO_start, SO_off, SO_display, SO_OneOn, SO_TwoOn, SO_bothOff };
//global variables
unsigned char playerOneScore = 0;
unsigned char playerTwoScore = 0;
unsigned char gameEnd = 0;
unsigned char on = 0;
unsigned char LedState = 0;
unsigned char reset = 0;

// SysControl task
int SysControlTick(int state) {
    switch (state) {
        case SC_start:
            state = SC_off;
            break;
        case SC_off:
            if (A0 && !on) {
                state = SC_press;
            }
            break;
        case SC_press:
            if (!on && !A0) {
                state = SC_on;
            }else if (on && !A0) {
                state = SC_off;
            }
            break;
         case SC_on:
            if (A0 && on) {
                state = SC_press;
            } else if (!on) {
                state = SC_off;
            }
            break;
        default:
            state = SC_start;
            break;
    }
    
    switch (state) {
        case SC_start:
        case SC_off:
            on = 0; 
            B0 = 0; 
            break;
        case SC_press:
            break;
         case SC_on:
            B0 = 1; 
            if(gameEnd) {
                on = 0;
            }else {
                on = 1; 
            }
            break;
    }
    
    return state;
}

// RandomLED task
int RandomLEDTick(int state) {
    static unsigned char i = 0;
    static unsigned short j = 0;

    switch (state) {
        case RLED_start:
            state = RLED_off;
            break;
        case RLED_wait:
            if (j > random[i] && on) {
                state = RLED_flash;
            } else if (!on) {
                state = RLED_off;
            }
            break;
        case RLED_flash:
            if (!on) {
                state = RLED_off;
            }else if(reset) {
               state = RLED_wait;
               i++;
               j = 0;   
            }
            break;
        case RLED_off:
            if (on) {
                state = RLED_wait;
            }
            break;
        default:
            state = RLED_start;
            break;
    }

    switch (state) {
        case RLED_start:
        case RLED_wait:
            j++;
            B7 = 0;
            LedState = 0;
            break;
        case RLED_flash:
            B7 = 1;
            LedState = 1; 
            break;
        case RLED_off:
            i = 0;
            j = 0;
            break;
    }

    return state;
}

// Players task
int PlayersTick(int state) {
    switch (state) {
        case P_start:
            state = P_off;
            break;
        case P_wait:
            if (!on){
               state = P_off;
            }else if (on && LedState) {
                state = P_flash;
            }
            break;
        case P_flash:
            if (!on) {
                state = P_off;
            }else if (on && !LedState) {
                state = P_wait;
            }
            break;
        case P_off:
            if (on) {
                state = P_wait;
            }
            break;
        default:
            state = P_start;
            break;
    }

    switch (state) {
        case P_wait:
            if(A1 &&A2){
                reset = 1;
            }else if(A1) {
                playerTwoScore++;
                reset = 1;
            }else if(A2) {
               playerOneScore++;
               reset = 1;
            }else {
                reset = 0;
            }
            brea;
        case P_flash:
            if(A1 &&A2){
                reset = 1;
            }else if(A1) {
                playerOneScore++;
                reset = 1;
            }else if(A2) {
                playerTwoScore++;
                reset = 1;
            }else {
               reset = 0;
            }
            break;
        case P_off:
            PlayerOneScore = 0;
            PlayerTwoScore = 0;
            reset = 0;
            break;
    }

    return state;
}


// ScoreOut task
int ScoreOutTick(int state) {
   static unsigned char cnt;
    switch (state) {
        case SO_start:
            state = SO_off;
            break;
        case SO_off:
            if (on) {
                state = SO_display;
            }
            break;
        case SO_display:
            if (!on ) {
                state = SO_off;
            } else if (playerOneScore >= 7 ) {
                state = SO_OneOn;
                cnt = 0;
            } else if (playerTwoScore >= 7) {
                state = SO_TwoOn;
                cnt = 0;
            }
            break;
        case SO_OneOn:
            if (cnt <= 15 && on) {
                state = SO_bothOff;
            } else{
                state = SO_off;
                  gameEnd = 1;
            }
            break;
        case SO_TwoOn:
            if (cnt <= 15 && on) {
                state = SO_bothOff;
            } else{
                state = SO_off;
                gameEnd = 1;
            }
            break;
        case SO_bothOff:
            if (playerOneScore >= 7 ) {
                state = SO_OneOn;
            }else if (playerTwoScore >= 7) {
                state = SO_TwoOn;
            } else if (!on) {
                state = SO_off;
            } 
            break;
        default:
            state = SO_start;
            break;
    }

    switch (state) {
         case SO_start:
         case SO_off:
               B &= 0x81;
               break;
         case SO_display:
               B1 = playerOneScore & 0x01;
               B2 = playerTwoScore & 0x02;
               B3 = playerOneScore & 0x04;
               B4 = playerTwoScore & 0x01;
               B5 = playerOneScore & 0x02;
               B6 = playerTwoScore & 0x04;
               gameEnd = 0;
               break;
         case SO_OneOn:
               B1 = 1;
               B2 = 1;
               B3 = 1;
               B4 = 0;
               B5 = 0;
               B6 = 0;
               cnt++;
               break;
         case SO_TwoOn:
               B1 = 0;
               B2 = 0;
               B3 = 0;
               B4 = 1;
               B5 = 1;
               B6 = 1;
               cnt++;
               break;
         case SO_bothOff:
               B &= 0x81; 
               cnt++;
               break;
    }
    return state;
}

int main() {
  B = 0x00; // Initialize B to 0
  tasks[0].state = SC_start;
   tasks[0].period = SYSCONTROL_PERIOD;
   tasks[0].elapsedTime = 0;
   tasks[0].TickFct = &SysControlTick;
   tasks[1].state = RLED_start;
   tasks[1].period = RANDOMLED_PERIOD;
   tasks[1].elapsedTime = 0;
   tasks[1].TickFct = &RandomLEDTick;
   tasks[2].state = P_start;
   tasks[2].period = PLAYERS_PERIOD;
   tasks[2].elapsedTime = 0;
   tasks[2].TickFct = &PlayersTick;
   tasks[3].state = SO_start;
   tasks[3].period = SCOREOUT_PERIOD;
   tasks[3].elapsedTime = 0;
   tasks[3].TickFct = &ScoreOutTick;
   
   TimerSet(GCD_PERIOD);
   TimerOn();

}