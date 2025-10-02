#ifndef ST7735_H
#define ST7735_H


#include <avr/io.h>
#include <util/delay.h>
#include "spiAVR.h"

// A0 pin 8        | 0000 0001
// reset pin 12    | 0001 0000
// CS pin 10       | 0000 0100
// SDA pin 11      | 0000 1000
// SCK pin 13      | 0010 0000

#define playerY 113// Y position of the player sprite

// Define the control pins for the ST7735
#define SWRESET 0x01 // Software reset
#define SLPOUT 0x11 // sleep out
#define COLMOD 0x3A // color mode
#define DISPON 0x29 // display on
#define CASET 0x2A // column address
#define RASET 0x2B // row address
#define RAMWR 0x2C // mem write


const unsigned char characters[11] = 
    {0x70, 0x18, 0x7D, 0xB6, 0xBC, 0x3C, 0xBC, 0xB6, 0x7D, 0x18, 0x70}; // crab

const unsigned char coin[6]=
    { 0b001100,
      0b011110,
      0b110111,
      0b111011,
      0b011110,
      0b001100}; 
    
void Send_Command(unsigned char cmd) {
    PORTB &= 0xFB; // Set CS pin 0
    PORTB &= 0xFE; // A0 to command | A0 = 0
    SPI_SEND(cmd); // Send command
    PORTB |= 0x04; // Set CS pin 1
}

void Send_Data(unsigned char data) {
    PORTB &= 0xFB; // Set CS pin 0
    PORTB |= 0x01; // A0 to data | A0 = 1
    SPI_SEND(data); // Send data
    PORTB |= 0x04; // Set CS pin 1
}

void HardwareReset(void){
    PORTB &= 0xEF; // Set reset pin 0
    _delay_ms(200); 
    PORTB |= 0x10; // Set reset pin 1
    _delay_ms(200);
}

void ST7735_Init(void) {

    HardwareReset(); 
    Send_Command(SWRESET); 
    _delay_ms(150); 
    Send_Command(SLPOUT); 
    _delay_ms(200); 
    Send_Command(COLMOD); 
    Send_Data(0x05); // 16-bit color mode
    _delay_ms(10);
    Send_Command(DISPON);
    _delay_ms(200);

}

void setScreenArea(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1) {
    x0 += 2;
    x1 += 2;
    y0 += 1;
    y1 += 1;

    Send_Command(CASET); //column address 
    Send_Data(0x00);
    Send_Data(x0); 
    Send_Data(0x00);
    Send_Data(x1); 
    

    Send_Command(RASET); // Row address
    Send_Data(0x00);
    Send_Data(y0); 
    Send_Data(0x00);
    Send_Data(y1); 
    

    Send_Command(RAMWR); // save
    _delay_ms(2);
}

void sendColor565(unsigned char red, unsigned char green, unsigned char blue) {
    uint16_t color = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
    Send_Data(color >> 8);     // High byte
    Send_Data(color & 0xFF);   // Low byte
}

void fillScreen(unsigned char red, unsigned char green, unsigned char blue) {
    setScreenArea(0, 0, 127, 127); // full screen

    for (unsigned int i = 0; i < 128 * 128; i++) {
        sendColor565(blue, green, red);
    }
}

void drawPixel(unsigned char x, unsigned char y, unsigned char red, unsigned char green, unsigned char blue) {
    setScreenArea(x, y, x, y); // Set area to a single pixel
    sendColor565(blue, green, red); 
}

void drawRectangle(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char red, unsigned char green, unsigned char blue) {
    // Clamp bounds to stay within the screen
    if (x0 > 127) x0 = 127;
    if (x1 > 127) x1 = 127;
    if (y0 > 127) y0 = 127;
    if (y1 > 127) y1 = 127;

    // Ensure correct order
    if (x1 < x0) { unsigned char tmp = x0; x0 = x1; x1 = tmp; }
    if (y1 < y0) { unsigned char tmp = y0; y0 = y1; y1 = tmp; }

    setScreenArea(x0, y0, x1, y1);

    unsigned int totalPixels = (x1 - x0 + 1) * (y1 - y0 + 1);
    
    setScreenArea(x0, y0, x1, y1); // Set area for rectangle

    for (unsigned int i = 0; i < totalPixels; i++) {
        sendColor565(blue, green, red);
    }
}
void drawVerticalLine(unsigned char x, unsigned char y0, unsigned char y1, unsigned char r, unsigned char g, unsigned char b) {
    if (y1 < y0) { unsigned char tmp = y0; y0 = y1; y1 = tmp; }
    setScreenArea(x, y0, x, y1);
    for (unsigned char y = y0; y <= y1; y++) {
        sendColor565(b, g, r);
    }
}

void erasePlayer(unsigned char x) {
    drawRectangle(x - 10, playerY -2, x + 10, playerY + 10, 0x00, 0x00, 0x00);
}

void drawPlayer(unsigned char x) {
    unsigned char yPos = playerY + 10;
    for (unsigned char dx = 0; dx <= 10; dx++) {
        unsigned char height = 12 - dx;
        drawVerticalLine(x + dx, yPos - height, yPos, 0x00, 0xFF, 0x00);
        drawVerticalLine(x - dx, yPos - height, yPos, 0x00, 0xFF, 0x00);
    }
}

void drawCoin(unsigned char x) {
    for (unsigned char i = 0; i < 6; i++) {
        unsigned bit = coin[i];
        for (unsigned char j = 0; j < 6; j++) {
            if (bit & (1 << j)) { // Check if the bit is set
                drawPixel(x + i, playerY + j, 0xFF, 0xFF, 0x00); // Draw cyan pixel
            } 
        }
    }
}
void eraseCoin(unsigned char x) {
    for (unsigned char i = 0; i < 6; i++) {
        unsigned bit = coin[i];
        for (unsigned char j = 0; j < 6; j++) {
            if (bit & (1 << j)) { // Check if the bit is set
                drawPixel(x + i, playerY + j, 0x00, 0x00, 0x00); // Draw cyan pixel
            } 
        }
    }
}

void drawCrab(unsigned char x, unsigned char y) {
    for (unsigned char i = 0; i < 11; i++) {
        unsigned bit = characters[i];
        for (unsigned char j = 0; j < 8; j++) {
            if (bit & (1 << j)) { // Check if the bit is set
                drawPixel(x + i, y + j, 0x00, 0xFF, 0xFF); // Draw cyan pixel
            } 
        }
    }
}

void eraseCrab(unsigned char x, unsigned char y) {
    for (unsigned char i = 0; i < 11; i++) {
        unsigned bit = characters[i];
        for (unsigned char j = 0; j < 8; j++) {
            if (bit & (1 << j)) { // Check if the bit is set
                drawPixel(x + i, y + j, 0x00, 0x00, 0x00); // Draw cyan pixel
            } 
        }
    }
}

// void erasePlayer(unsigned char x) {
//     unsigned char yPos = playerY + 10;
//     for (unsigned char dx = 0; dx <= 10; dx++) {
//         unsigned char height = 12 - dx;
//         drawVerticalLine(x + dx, yPos - height, yPos, 0x00, 0x00, 0x00);
//         drawVerticalLine(x - dx, yPos - height, yPos, 0x00, 0x00, 0x00);
//     }
// }

// void drawPlayer(unsigned char x) {
//     unsigned char width;
//     unsigned char yPos = playerY+10; // Player Y position
//     for (unsigned char dx = 0; dx <= 10; dx++) {
//         width = 12 - dx; // Make center column tallest
//         drawRectangle(x + dx, yPos- width, x + dx, yPos , 0x00, 0xFF, 0x00);
//         drawRectangle(x - dx, yPos- width, x - dx, yPos , 0x00, 0xFF, 0x00);
//     }
// }

// void erasePlayer(unsigned char x) {
//     unsigned char width;
//     unsigned char yPos = playerY + 10; // Player Y position
//     for (unsigned char dx = 0; dx <= 10; dx++) {
//         width = 12 - dx; // Make center column tallest
//         drawRectangle(x + dx, yPos - width, x + dx, yPos, 0x00, 0x00, 0x00);
//         drawRectangle(x - dx, yPos - width, x - dx, yPos, 0x00, 0x00, 0x00);
//     }
// }




void shootBullet(unsigned char x) {
    unsigned char y = playerY-5;
    drawRectangle(x, y+1, x, y+3, 0xFF, 0x00, 0x00); // Draw bullet in red color

    while (y > 0) {
        drawRectangle(x, y+1, x, y+3, 0x00, 0x00, 0x00);// Clear previous bullet position
        y--;
        drawRectangle(x, y+1, x, y+3, 0xFF, 0x00, 0x00);// Draw new bullet position
        _delay_ms(20); // Delay for visibility
        
    }
    drawRectangle(x-1, y+1, x+1, y+3, 0x00, 0x00, 0x00);// Clear previous bullet position
}

void drawBullet(unsigned char x, unsigned char y) {
    drawRectangle(x, y + 1, x, y + 3, 0xFF, 0x00, 0x00); // Draw bullet in red color
}
void eraseBullet(unsigned char x, unsigned char y) {
    drawRectangle(x, y + 1, x, y + 3, 0x00, 0x00, 0x00); // Draw bullet in red color
}




#endif