//for random number generation
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "LCD.h"
#include "timerISR.h"
#include "spiAVR.h"
#include "st7735.h"
#include "analogHelper.h"
#include "serialATmega.h"

//Number of tasks
#define NUM_TASKS 10
#define playerY 113



//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;

//struct for enemies
typedef struct _enemy {
    unsigned char x; // X position
    unsigned char y; // Y position
    unsigned char visible; // Visibility state (0: not visible, 1: visible)
} enemy;


//task periods
const unsigned long LCD_PERIOD = 500;
const unsigned long BUZZER_PERIOD = 100;
const unsigned long JOYSTICK_PERIOD = 100; 
const unsigned long PLAYER_PERIOD = 20; 
const unsigned long BULLERT_PERIOD = 20;
const unsigned long COIN_PERIOD = 500;
const unsigned long SCORETRACKER_PERIOD = 100;
const unsigned long ENEMIES_GENERATE_PERIOD = 2000; 
const unsigned long ENEMIES_PERIOD = 20;
const unsigned long ENEMY_MOVE_PERIOD = 20;
const unsigned long GCD_PERIOD = 20;

task tasks[NUM_TASKS];

void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                    // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {                 // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state);            // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                                    // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                          // Increment the elapsed time by GCD_PERIOD
	}
}




//define tasks
enum LCDStates {LCDStart, LCDWait, LCDUpdateDisplay, LCDUpdateLives, LCDWin, LCDLose};
enum BuzzerStates {BuzzerStart, BuzzerWait, CollectCoin, Win, Lose};
enum JoystickStates {JoystickStart, JSGameOff, JSWait, JSUpdate};
enum PlayerStates {PlayerStart, PlayerWait, PlayerMove, PlayerErase, PlayerDraw};
enum CoinStates {CoinStart, CoinOff, CoinWait, CoinGenerate, CoinPlace, CoinErase};
enum BulletStates {BulletStart, BulletWait, BulletMove, BulletShoot};
enum createEnemiesStates {CreateEnemiesStart, CreateEnemiesGenerate, CreateEnemiesWait};
enum EnemiesStates {EnemiesStart, EnemiesWait};
enum EnemyMoveStates {EnemyMoveStart, EnemyMoveOff, EnemyMoveWait, EnemyMoveSelect, EnemyMoveErase, EnemyAdvance, EnemyDraw};
enum ScoreTrackerStates {ScoreTrackerStart, ScoreTrackerWait, ScoreTrackerCoin, ScoreTrackerKillEnemy, ScoreTrackerOff};


//global varaibles 
unsigned char gameOn = 0x00; // Game state (0: not started, 1: started)

//to be used by LCD
unsigned short score = 0x00; //will be written by game task
char SCORE[] ="SCORE: ";
char score_str[5] = "0000"; 

//to be used by buzzer (written by score task)
unsigned char winGame = 0x00; 
unsigned char newCoin = 0x00; 
unsigned char killEnemy = 0x00; 
unsigned char killEnemyScore = 0x00; // Score for killing an enemy
unsigned char loseGame = 0x00; 

//written by player task
unsigned char playerX = 64; // Player X position
unsigned char shootX = 64; // Bullet X position
unsigned char lives = 3; 


//written by joystick task
unsigned char moveLeft = 0x00; 
unsigned char moveRight = 0x00;
unsigned char shoot = 0; // Bullet state (0: not shooting, 1: shooting)

//written by bullet task
unsigned char bulletY = 0;
unsigned char bulletVisible = 0; 

//written by coin task
unsigned char coinX = 240; 
unsigned char coinCollected = 0x00; // Coin collected state (0: not collected, 1: collected)
unsigned char coinVisible = 0x00; // Coin visibility state (0: not visible, 1: visible)
unsigned char coins =0;

//enemies positions
unsigned char enemyX[6] = {10, 30, 50, 70, 90, 110}; // Enemy X positions
unsigned char enemyY[4] = {10, 30, 50, 70}; // Enemy Y positions

enemy enemies[4][6]; // Array to store enemies

//custom character map (heart)
const unsigned char heart[8] = {
    0b00000,
    0b01010, 
    0b11111, 
    0b11111, 
    0b01110, 
    0b00100, 
    0b00000,
    0b00000
}; // Heart shape for custom character

// Helper Functions

// Convert an num to a array of chars
void int_to_str(unsigned short temp_score, char* score_str) {
    score_str[4] = '\0'; // Null terminator for 4-digit string

    for (int i = 3; i >= 0; i--) {
        score_str[i] = (temp_score % 10) + '0'; // Convert digit to character
        temp_score /= 10;
    }
}



// draw block of enemies & initialize an array to store them
void drawEnemyBlock() {
    for (unsigned char i = 0; i < 4; i++) {
        for (unsigned char j = 0; j < 6; j++) {
            drawCrab(enemyX[j], enemyY[i]);  // Note: X = columns, Y = rows
            enemies[i][j].x = enemyX[j]; // Set enemy X position
            enemies[i][j].y = enemyY[i]; // Set enemy Y position
            enemies[i][j].visible = 1; // Set enemy visibility to true
        }
    }
}

//check if all enemies are dead
unsigned char allEnemiesDead() {
    for (unsigned char i = 0; i < 4; i++) {
        for (unsigned char j = 0; j < 6; j++) {
            if (enemies[i][j].visible) { // If any enemy is still visible
                return 0; // Not all enemies are dead
            }
        }
    }
    return 1; // All enemies are dead
}

//check if row is dead
unsigned char rowDead(unsigned char row) {
    for (unsigned char j = 0; j < 6; j++) {
        if (enemies[row][j].visible) { // If any enemy in the row is still visible
            return 0; // Row is not dead
        }
    }
    return 1; // Row is dead
}

//check if enemy is hit by bullet
void enemyHit(unsigned char bulletX, unsigned char bulletY) {
    for (unsigned char i = 0; i < 4; i++) {
        for (unsigned char j = 0; j < 6; j++) {
            if (enemies[i][j].visible && 
                bulletX >= enemies[i][j].x && 
                bulletX <= enemies[i][j].x + 10 && 
                bulletY >= enemies[i][j].y && 
                bulletY <= enemies[i][j].y + 10 &&
                bulletVisible) {
                killEnemy = 0x01; // Set kill enemy flag
                killEnemyScore = 0x01;
                eraseCrab(enemies[i][j].x, enemies[i][j].y); // Erase the enemy
                enemies[i][j].visible = 0; // Set enemy visibility to false
            }
        }
    }
}

//check if player is hit by enemy  
unsigned char playerHit() {
    for (unsigned char i = 0; i < 4; i++) {
        for (unsigned char j = 0; j < 6; j++) {
            if (enemies[i][j].visible && 
                enemies[i][j].y + 10 >= playerY && 
                enemies[i][j].y <= playerY + 10 &&
                enemies[i][j].x + 10 >= playerX && 
                enemies[i][j].x <= playerX + 10) {
                return 1; // Player is hit by an enemy
            }
        }
    }
    return 0; // Player is not hit
}

//reset game
void resetGame() {

    // Player
    playerX = 64;
    shootX = 64;
    lives = 3;
    score = 0;
    bulletVisible = 0;
    shoot = 0;
    coins = 0;
    
    // Coin
    coinX = 240;
    coinCollected = 0;
    coinVisible = 0;

    // Enemies
    for (unsigned char i = 0; i < 4; i++) {
        for (unsigned char j = 0; j < 6; j++) {
            enemies[i][j].visible = 1;
            enemies[i][j].x = enemyX[j];
            enemies[i][j].y = enemyY[i];
        }
    }

    // Flags
    killEnemy = 0;
    killEnemyScore = 0;
    newCoin = 0;
    loseGame = 0;
    winGame = 0;
}



const char mario_prescalers[9] = {
    0x03, // 64
    0x03, // 64
    0x03, // 64
    0x04, // 256
    0x04, // 256
    0x05, // 128
    0x03, // 64
    0x04, // 256
    0x06  // 1024
};

const uint16_t mario_OCR1A[9] = {
    374, // E5
    312, // G5
    237, // C6
     78, // G5 low
     97, // E5 low
    140, // A5 mid
    252, // B5
     66, // A#5
     17  // A5 deep
};






//TICK FUNCTIONS
unsigned char k = 0; // Counter for win/lose messages
//LCD tick
int LCDTick(int state) {
    static unsigned char prev_score = 0x00;
    static unsigned char prev_lives = 3; 
    switch (state) {
        case LCDStart:
            lcd_clear(); 
            lcd_goto_xy(0, 0); // Set cursor to the first line
            lcd_write_str(SCORE);
            int_to_str(score, score_str);
            lcd_write_str(score_str);
            lcd_goto_xy(0, 13);
            lcd_write_character(0x00); 
            lcd_goto_xy(0, 14);
            lcd_write_character(0x00); // Draw heart character
            lcd_goto_xy(0, 15);
            lcd_write_character(0x00); 
            state = LCDWait;
            break;
        case LCDWait:
            if(winGame){
                k = 0;
                state = LCDWin; // Update display if winGame is set
            }else if (loseGame) {
                k = 0;
                state = LCDLose; // Update display if loseGame is set
            }else if(score != prev_score) {
                state = LCDUpdateDisplay;
            }else if(lives != prev_lives) {
                state = LCDUpdateLives; // Update display if lives changed
            }else if(!gameOn) {
                state = LCDStart; 
            }
            else {
                state = LCDWait;
            }
            break;
        case LCDUpdateDisplay:
            state = LCDWait;
            break;
        case LCDUpdateLives:
            state = LCDWait; 
            break;
        case LCDWin:
            if (k < 6) { // Wait for a while to show win message
                k++;
            } else {
                state = LCDStart; // Reset to start after showing win message
            }
            break;
        case LCDLose:
            if (k < 6) { // Wait for a while to show lose message
                k++;
            } else {
                state = LCDStart; // Reset to start after showing lose message
            }
            break;
        default:
            state = LCDStart;
            break;
    }

    switch (state) {
        case LCDUpdateDisplay:
            lcd_goto_xy(0, 7); // SCORE: _ _ 
            lcd_write_str("   "); 
            lcd_goto_xy(0, 7);
            int_to_str(score, score_str);
            lcd_write_str(score_str);
            break;
        case LCDUpdateLives:
            lcd_goto_xy(0, 13); // Position for lives
            lcd_write_str("   "); // Clear previous lives display
            lcd_goto_xy(0, 13);
            for (unsigned char i = 0; i < lives; i++) {
                lcd_write_character(0x00); // Draw heart character for each life
            }
            break;
        case LCDLose:
            lcd_clear(); // Clear the screen
            lcd_goto_xy(0, 0);
            lcd_write_str("GAME OVER :(");
            lcd_goto_xy(1, 0);
            lcd_write_str("SCORE: ");
            int_to_str(score, score_str);
            lcd_write_str(score_str);
            break;
        case LCDWin:
            lcd_clear(); // Clear the screen
            lcd_goto_xy(0, 0);
            lcd_write_str("YOU WIN!!! :)");
            lcd_goto_xy(1, 0);
            lcd_write_str("SCORE: ");
            int_to_str(score, score_str);
            lcd_write_str(score_str);
            break;
        default:
            break;
    }

    prev_score = score;
    return state;
}

// Active Buzzer 
int BuzzerTick(int state) {

  static unsigned char i = 0x00;
  static char note_idx = 0;
  static unsigned char j = 0x00;
    //state transition
    switch(state){
        case BuzzerStart:
            state = BuzzerWait;
            break;
        case BuzzerWait:
            if(winGame){ 
                i = 0x00;
                state = Win;
            }else if(loseGame){
                i = 0x00;
                state = Lose;
            }
            else if(newCoin){
                i = 0x00;
                state = CollectCoin;
            }
            break;
        case CollectCoin:
            if(winGame){ 
                i = 0x00;
                state = Win;
            }else if(loseGame){
                i = 0x00;
                state = Lose;
            } else{ 
                if(i > 12){
                  state = BuzzerWait;
                }
            }
            break;
        case Win:
            if(i > 30){
                i = 0x00;
                winGame = 0x00; 
                state = BuzzerWait;
            }
            break;
        case Lose:
            if(i > 30){
                i = 0x00;
                loseGame = 0x00;
                state = BuzzerWait;
            }
            break;
        default:
            state = BuzzerStart;
            break;
    }

    //state action
    switch(state){
        case BuzzerWait:
            OCR1A = 255; // Turn off buzzer
            break;
        case CollectCoin:
            switch (i%6){
              case 0:
                  TCCR1B = (TCCR1B & 0xF8) | 0x02;  
                  OCR1A = 128;
                  break;
              case 1:
                  TCCR1B = (TCCR1B & 0xF8) | 0x03;  
                  OCR1A = 128;
                  break;
              case 2:
                  TCCR1B = (TCCR1B & 0xF8) | 0x02;  
                  OCR1A = 128;
                  break;
              case 3:
                  TCCR1B = (TCCR1B & 0xF8) | 0x03;  
                  OCR1A = 128;
                  break;
              case 4:
                  TCCR1B = (TCCR1B & 0xF8) | 0x02;  
                  OCR1A = 128;
                  break;
              case 5:
                  TCCR1B = (TCCR1B & 0xF8) | 0x02;  
                  OCR1A = 128;
            break;
            }
            i++;
            break;
        case Win:
            if (note_idx < 9) {
            if (j == 0) {
                // Load new note
                TCCR1B = (TCCR1B & 0xF8) | mario_prescalers[note_idx];
                OCR1A = mario_OCR1A[note_idx];
            }

            j ++; // Assume 50ms task tick
            if (j >= 2) {
                j = 0;
                note_idx++;
            }
        } 
        i++;
        break;
        case Lose:
            if(i%10 < 6){
                TCCR1B = (TCCR1B & 0xF8) | 0x04; 
                OCR1A = 128; 
            }else{
                TCCR1B = (TCCR1B & 0xF8) | 0x06;
                OCR1A = 128; 
            }    
            i++;   
            break;
        default:
            break;
    }

    return state;
}

// Joystick tick
int JoystickTick(int state) {

    char btn = PINC & 0x01; 
    char resetBtn = PINC & 0x20;

    switch (state) {
        case JoystickStart:
            state = JSGameOff;
            break;
        case JSGameOff:
            if (!btn) {
                gameOn = 0x01; // Start the game
                resetGame(); // Reset game state
                state = JSUpdate;
            }

            break;
        case JSUpdate:
            if (!gameOn){
                state = JSGameOff; // Game is off, return to JSGameOff state
                moveLeft = 0x00;
                moveRight = 0x00;
            } 
            break;
        default:
            state = JoystickStart;
            break;
    }

    switch (state) {
        case JSUpdate:
            // Read the joystick position
            unsigned short XjoyStickValue = ADC_read(1); // Read X-axis
            unsigned short YjoystickValue = ADC_read(2); // Read Y-axis
            if(resetBtn){
                gameOn = 0x00; // Reset game state
                resetGame(); // Reset game state
            }else if (XjoyStickValue < 400) { // Move left
                moveLeft = 0x01;
                moveRight = 0x00;
            } else if (XjoyStickValue > 550) { // Move right
                moveLeft = 0x00;
                moveRight = 0x01;
            } else if(!shoot && YjoystickValue < 400) { // Shoot
                shoot = 0x01; 
                shootX = playerX; // Set bullet position to player position
             }else {
                moveLeft = 0x00;
                moveRight = 0x00;
            }

            break;
        default:
            state = JoystickStart;
            break;
    }

    return state;
}

// Player tick
int PlayerTick(int state) {
    static unsigned char lastPlayerX = 64; // Last player X position
    static unsigned char invincible = 0x00; // Invincibility state (0: not invincible, 1: invincible)
    static unsigned char hitCooldown = 0;
    switch (state) {
        case PlayerStart:
            state = PlayerWait;
            drawPlayer(playerX); // Draw player at initial position
            break;
        case PlayerWait:
            if (moveLeft || moveRight){
                state = PlayerErase; 
            } else {
                state = PlayerWait;
            }
            break;
        case PlayerMove:
            if(!(moveLeft || moveRight)) {
                state = PlayerWait; 
            } else  {
                state = PlayerErase; 
            }
            break;
        case PlayerErase:
            if (moveLeft || moveRight) {
                state = PlayerMove; // Continue moving
            }else {
                state = PlayerWait; // Return to waiting state
            }
            break;

        default:
            state = PlayerStart;
            break;
    }

    switch (state) {
        case PlayerWait:
        if(invincible) {
            hitCooldown--;
            if (hitCooldown == 0) {
                invincible = 0x00; // Reset invincibility after cooldown
            }
        }
            if(playerHit() && !invincible){
                lives--; // Decrease lives if player is hit
                invincible = 0x01; // Set invincibility flag
                hitCooldown = 25;
                if (lives == 0) {
                    fillScreen(0x00, 0x00, 0x00); // Clear the screen
                    loseGame = 0x01; // Set lose game flag
                    erasePlayer(playerX); // Erase player
                    gameOn = 0x00; // End the game
                    playerX = 64; // Reset player position
                    lastPlayerX = 64; // Reset last player position
                    drawPlayer(playerX);
                } else {
                    erasePlayer(playerX); 
                    playerX = 64; // Reset player position
                    lastPlayerX = 64; // Reset last player position
                    drawPlayer(playerX); // Redraw player at reset position
                }
            }
            break;
        case PlayerMove:
            if(invincible) {
                hitCooldown--;
                if (hitCooldown == 0) {
                    invincible = 0x00; // Reset invincibility after cooldown
                }
            }
            if(playerHit() & !invincible) {
                lives--; // Decrease lives if player is hit
                invincible = 0x01;
                hitCooldown = 25; // Set hit cooldown
                if (lives == 0) {
                    fillScreen(0x00, 0x00, 0x00); // Clear the screen
                    loseGame = 0x01; // Set lose game flag
                    erasePlayer(playerX); // Erase player
                    gameOn = 0x00; // End the game
                    playerX = 64; // Reset player position
                    lastPlayerX = 64; // Reset last player position
                    drawPlayer(playerX);
                    lives = 3;
                } else {
                    erasePlayer(playerX); 
                    playerX = 64; // Reset player position
                    lastPlayerX = 64; // Reset last player position
                    drawPlayer(playerX); // Redraw player at reset position
                }
            } else if(moveRight && playerX < 120) { // Prevent moving out of bounds
                lastPlayerX = playerX; 
                playerX += 5; 
            } else if (moveLeft && playerX > 10) { // Prevent moving out of bounds
                lastPlayerX = playerX;
                playerX -= 5; 
            }
            break;
        case PlayerErase:
            if (lastPlayerX != playerX) {
                erasePlayer(lastPlayerX);
                drawPlayer(playerX); 
            }

            break;
        default:
            break;
    }

    return state;
}

// Bullet tick
int bulletTick(int state) {
static unsigned char bulletY_prev; // Bullet Y position

    switch (state) {
        case BulletStart:
            state = BulletWait;
            break;
        case BulletWait:
            if (shoot) {
                bulletY = playerY - 12; // Set bullet Y position above the player
                bulletVisible = 0x01; // Set bullet visibility to true
                state = BulletMove; 
            } else {
                state = BulletWait;
            }
            break;
        case BulletMove:
            if (bulletY > 5) { // Check if the bullet is still on the screen
                state = BulletShoot;
            } else {
                state = BulletWait; 
                eraseBullet(shootX, bulletY_prev); 
                shoot = 0;// Reset to wait state when bullet goes off screen
                bulletVisible = 0x00; // Set bullet visibility to false
            }
            break;
        case BulletShoot:
            if(killEnemy){
                state = BulletWait; // Return to wait state after shooting
                killEnemy = 0x00; // Reset kill enemy flag
            } else {
                state = BulletMove; // Continue moving the bullet
            }
            break;
        default:
            state = BulletStart;
            break;
    }

    switch (state) {
        case BulletMove:
            bulletVisible = 0x01; // Set bullet visibility to true
            bulletY_prev = bulletY; // Store previous bullet Y position
            bulletY -= 5; // Move the bullet up
            break;
        case BulletShoot:
            if(killEnemy){
                eraseBullet(shootX, bulletY_prev); 
                bulletVisible = 0x00; 
                shoot = 0x00; 
            } else {
                bulletVisible = 0x01;
                drawBullet(shootX, bulletY);
                eraseBullet(shootX, bulletY_prev); 
            }
            
            break;
        default:
            break;
    }

    return state;
}

// coin tick 
int CoinTick(int state) {
    static unsigned char coinTime = 0; // Time for coin generation
    static unsigned char i = 0; // cointer for coin generation and erasure
    switch (state) {
        case CoinStart:
            state = CoinOff;
            break;
        case CoinOff:
            if (gameOn) {
                coinTime = rand() % 20 + 10; 
                state = CoinWait; 
            }
            break;
        case CoinWait:
            if(!gameOn){
                state = CoinOff; // Return to CoinOff state if game is off
            } else if (coinTime == i) {
                i = 0;
                state = CoinGenerate; 
            }
            break;
        case CoinGenerate:
            state = CoinPlace;
            break;
        case CoinPlace:
            if(i > 2) {
                state = CoinErase; // Erase the coin after it has been placed
            } 
            break;
        case CoinErase:
            i = 0;
            coinTime = rand() % 10 + 3; 
            state = CoinWait; 
            break;
        default:
            state = CoinStart;
            break;
    }

    switch (state) {
        case CoinWait:
            
            i++; 
            break;
        case CoinGenerate:
            coinX = rand() % 110 + 9;
            drawCoin(coinX); 
            coinVisible = 0x01; // Set coin visibility to true
            i = 0;
            break;
        case CoinPlace:
            i++; // Increment the counter for coin placement
            break;
        case CoinErase:
            if ((coinX + 5 < playerX) || (coinX - 5 > playerX)) {
                eraseCoin(coinX);
            }
            coinCollected = 0x00; 
            coinVisible = 0x00; // Set coin visibility to false
            coinX = 240;
            break;
        default:
            break;
    }

    return state;
}

//score tracker tick
int ScoreTrackerTick(int state) {
    switch (state) {
        case ScoreTrackerStart:
            state = ScoreTrackerWait;
            break;
        case ScoreTrackerOff:
            if (gameOn) {
                state = ScoreTrackerWait; // Wait for game to start
            }
            break;
        case ScoreTrackerWait:
            if(!gameOn){
                state = ScoreTrackerOff; // Return to ScoreTrackerOff state if game is off
            }else if (newCoin) {
                newCoin = 0x00;
                state = ScoreTrackerCoin;
            } else if (killEnemyScore) {
                killEnemyScore = 0x00;
                state = ScoreTrackerKillEnemy;
            }
            break;
        case ScoreTrackerCoin:
            state = ScoreTrackerWait; // Return to wait state after updating score
            break;
        case ScoreTrackerKillEnemy:
            state = ScoreTrackerWait; // Return to wait state after updating score
            break;
        default:
            state = ScoreTrackerStart;
            break;
    }

    switch (state) {

        case ScoreTrackerWait:
            if(playerX < coinX + 5 && playerX > coinX -5 && !coinCollected && coinVisible) {
                newCoin = 0x01; // Coin collected
                coinCollected = 0x01; 
            }
            break;
        case ScoreTrackerCoin:
            score += 100;
            coins++;
            serial_println("Coin collected!"); // Print message to serial
            serial_println(coins); // Print message to serial
            if (coins >= 3) {
                winGame = 0x01; // Set win game flag
                gameOn = 0x00; // End the game
            }
            
            break;
        case ScoreTrackerKillEnemy:
            score += 10; 
            break;
        default:
            break;
    }

    return state;
}

//create enemies tick
int CreateEnemiesTick(int state) {
    static unsigned char drawn = 0; 
    switch (state) {
        case CreateEnemiesStart:
            state = CreateEnemiesGenerate;
            break;
        case CreateEnemiesGenerate:
            state = CreateEnemiesWait; // Wait after generating enemies
            break;
        case CreateEnemiesWait:
            if (allEnemiesDead()) {
                state = CreateEnemiesGenerate; 
            } else if(!gameOn && !drawn) {
                state = CreateEnemiesStart; // Return to start state if game is off
            } else if(gameOn){
                drawn = 0;
            }
            break;
        default:
            state = CreateEnemiesStart;
            break;
    }

    switch (state) {
        case CreateEnemiesGenerate:
            fillScreen(0x00, 0x00, 0x00); // Clear the screen
            drawEnemyBlock(); // Draw the block of enemies
            drawn = 1; // Set drawn flag to true
            if(gameOn){
                drawPlayer(playerX); // Draw the player
            }else {
                drawPlayer(64); // Erase the player if game is off
            }
            break;
        case CreateEnemiesWait:
            break;
        default:
            break;
    }

    return state;
}
//enemies tick
int EnemiesTick(int state) {
    switch (state) {
        case EnemiesStart:
            state = EnemiesWait;
            break;

        case EnemiesWait:

            break;
        default:
            state = EnemiesStart;
            break;
    }

    switch (state) {
        case EnemiesWait:
            enemyHit(shootX, bulletY); 
            break;
        default:
            break;
    }

    return state;
}

// Enemy movement tick
int EnemyMoveTick(int state){
    static unsigned char enemyTime = 0; 
    static unsigned char j = 0; // counter for movement
    static unsigned char row = 3; // current row of enemies
    static unsigned char enemyIndex = 0; 
    static unsigned char selected = 0;

    switch(state){
        case EnemyMoveStart:
            state = EnemyMoveOff;
            break;
        case EnemyMoveOff:
            if (gameOn) {
                row = 3; // Reset to the last row
                j = 0; // Reset the counter for enemy movement
                selected = 0; // Reset selected flag
                enemyTime = 5; // Random time for enemy movement
                state = EnemyMoveWait; 
            }
            break;
        case EnemyMoveWait:
            if (!gameOn) {
                state = EnemyMoveOff; // Return to EnemyMoveOff state if game is off
            }
            else if (enemyTime == j) {
                j = 0;
                state = EnemyMoveSelect; 
            }
            break;
        case EnemyMoveSelect:
            if (!gameOn) {
                state = EnemyMoveOff; // Return to EnemyMoveOff state if game is off
            }else if (selected) {
                selected = 0; // Reset selected flag
                state = EnemyMoveErase; // Move the selected enemy
            } 
            break;
        case EnemyMoveErase:
            state = EnemyAdvance; 
            break;
        case EnemyAdvance:
            if(!gameOn){
                state = EnemyMoveOff;
            }
            else if(enemies[row][enemyIndex].y > 110){
                eraseCrab(enemies[row][enemyIndex].x, enemies[row][enemyIndex].y); 
                enemies[row][enemyIndex].visible = 0; // Set enemy visibility to false
                enemyTime = 5; 
                state = EnemyMoveWait;

            }else{
                state = EnemyDraw; 
            }
            break;
        case EnemyDraw:
            state = EnemyMoveWait;
            selected = 0; // Reset selected flag
            break;
        default:
            state = EnemyMoveStart;
            break;
    }

    switch(state){
        case EnemyMoveOff:
            // Do nothing, just wait for the game to start
            break;
        case EnemyMoveWait:
            j++; // Increment the counter for enemy movement
            break;
        case EnemyMoveSelect:
            // Select a random enemy from the current row to move
            if (rowDead(row)) {
                
                if (row == 0) {
                    row = 3; // Reset to the last row
                }
                else{
                    row--; // Move to the next row if the current row is dead
                }
            }
            enemyIndex = rand() % 6; 
            if (enemies[row][enemyIndex].visible) {
                selected = 1; 
            }else {
                selected = 0;
            }
            break;
        case EnemyMoveErase:
            if (enemies[row][enemyIndex].visible) {
                eraseCrab(enemies[row][enemyIndex].x, enemies[row][enemyIndex].y);
            }
            break;

        case EnemyAdvance:
            if (enemies[row][enemyIndex].visible) {
                if (enemies[row][enemyIndex].y < 110) {
                    enemies[row][enemyIndex].y += 5;
                } else {
                    eraseCrab(enemies[row][enemyIndex].x, enemies[row][enemyIndex].y);
                    enemies[row][enemyIndex].visible = 0;
                }
            }
            break;

        case EnemyDraw:
            if (enemies[row][enemyIndex].visible) {
                drawCrab(enemies[row][enemyIndex].x, enemies[row][enemyIndex].y);
                enemyHit(shootX, bulletY); 
            }
            
            break;
        default:
            break;
    }
    return state;
}




//main function
int main(void) {
    // initialize all inputs and ouputs

    DDRB = 0xFF; 
    PORTB = 0x00;

    DDRC = 0x00; 
    PORTC = 0x01; 

    DDRD = 0xFF;
    PORTD = 0x00;

    // Initialize the ADC
    ADC_init();
    // Initialize the serial communication
    serial_init(9600); // Initialize serial communication at 9600 baud rate

    // Initialize the LCD
    lcd_init();
    lcd_clear();
    lcdCustomChar(0, heart);

    // Initialize the buzzer timer/pwm
    TCCR1A |= (1 << COM1A1);              // Non-inverting mode on OC1A (PB1)
    TCCR1A |= (1 << WGM10);               // Fast PWM 8-bit: WGM10 + WGM12
    TCCR1B |= (1 << WGM12);              
    TCCR1B = (TCCR1B & 0xF8) | 0x02;      // Prescaler = 8 (0010)

    // initialize SPI for LCD
    SPI_INIT();
    // Initialize the LCD display
    ST7735_Init();
    // Fill the screen with black color
    fillScreen(0x00, 0x00, 0x00);

    
    srand(time(NULL)); // Seed the random number generator

    //task initialization
    tasks[0].state = LCDStart; // Task initial state.
    tasks[0].period = LCD_PERIOD; // Task Period.
    tasks[0].elapsedTime = 0; // Task current elapsed time.
    tasks[0].TickFct = &LCDTick; // Function pointer for the tick function.

    tasks[1].state = BuzzerStart; // Task initial state.
    tasks[1].period = BUZZER_PERIOD; // Task Period.
    tasks[1].elapsedTime = 0; // Task current elapsed time.
    tasks[1].TickFct = &BuzzerTick; // Function pointer for the tick function.

    tasks[2].state = JoystickStart; // Task initial state.
    tasks[2].period = JOYSTICK_PERIOD; // Task Period.
    tasks[2].elapsedTime = 0; // Task current elapsed time.
    tasks[2].TickFct = &JoystickTick; // Function pointer for the tick function.

    tasks[3].state = PlayerStart; // Task initial state.
    tasks[3].period = PLAYER_PERIOD; // Task Period.
    tasks[3].elapsedTime = 0; // Task current elapsed time.
    tasks[3].TickFct = &PlayerTick; // Function pointer for the tick function.

    tasks[4].state = BulletStart; // Task initial state.
    tasks[4].period = BULLERT_PERIOD; // Task Period.
    tasks[4].elapsedTime = 0; // Task current elapsed time.
    tasks[4].TickFct = &bulletTick; // Function pointer for the tick function.

    tasks[5].state = CoinStart; // Task initial state.
    tasks[5].period = COIN_PERIOD; // Task Period.
    tasks[5].elapsedTime = 0; // Task current elapsed time.
    tasks[5].TickFct = &CoinTick; // Function pointer for the tick function.

    tasks[6].state = ScoreTrackerStart; // Task initial state.
    tasks[6].period = SCORETRACKER_PERIOD; // Task Period.
    tasks[6].elapsedTime = 0; // Task current elapsed time.
    tasks[6].TickFct = &ScoreTrackerTick; // Function pointer for the tick function.

    tasks[7].state = CreateEnemiesStart; // Task initial state.
    tasks[7].period = ENEMIES_GENERATE_PERIOD; // Task Period.
    tasks[7].elapsedTime = ENEMIES_GENERATE_PERIOD; // Task current elapsed time.
    tasks[7].TickFct = &CreateEnemiesTick; // Function pointer for the tick function.

    tasks[8].state = EnemiesStart; // Task initial state.
    tasks[8].period = ENEMIES_PERIOD; // Task Period.
    tasks[8].elapsedTime = 0; // Task current elapsed time.
    tasks[8].TickFct = &EnemiesTick; // Function pointer for the tick function.

    tasks[9].state = EnemyMoveStart; // Task initial state.
    tasks[9].period = ENEMY_MOVE_PERIOD; // Task Period.
    tasks[9].elapsedTime = 0; // Task current elapsed time.
    tasks[9].TickFct = &EnemyMoveTick; // Function pointer for the tick function.


    
    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {}

    return 0;
}