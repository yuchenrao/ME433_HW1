#include <xc.h> // processor SFR definitions
#include "ILI9163C.h"
#include "i2c_master_noint.h"
#include "IMU.h"
#include <sys/attribs.h> // __ISR macro
#include <math.h>
#include <stdio.h>

// DEVCFG0

#pragma config DEBUG = OFF // no debugging

#pragma config JTAGEN = OFF // no jtag

#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1

#pragma config PWP = OFF // no write protect

#pragma config BWP = OFF // no boot write protect

#pragma config CP = OFF // no code protect

// DEVCFG1

#pragma config FNOSC = FRCPLL // use primary oscillator with pll

#pragma config FSOSCEN = OFF // turn off secondary oscillator

#pragma config IESO = OFF // no switching clocks

#pragma config POSCMOD = HS // high speed crystal mode

#pragma config OSCIOFNC = ON // free up secondary osc pins

#pragma config FPBDIV = DIV_1 // divide CPU freq by 1 for peripheral bus clock

#pragma config FCKSM = CSDCMD // do not enable clock switch

#pragma config WDTPS = PS1 // slowest wdt

#pragma config WINDIS = OFF // no wdt window

#pragma config FWDTEN = OFF // wdt off by default

#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the CPU clock to 48MHz

#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz

#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV

#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz

#pragma config UPLLIDIV = DIV_2 // divider for the 8MHz input clock, then multiply by 12 to get 48MHz for USB

#pragma config UPLLEN = ON // USB clock on

// DEVCFG3

#pragma config USERID = 0 // some 16bit userid, doesn't matter what

#pragma config PMDL1WAY = OFF // allow multiple reconfigurations

#pragma config IOL1WAY = OFF // allow multiple reconfigurations

#pragma config FUSBIDIO = ON // USB pins controlled by USB module

#pragma config FVBUSONIO = ON // USB BUSON controlled by USB module


void drawCharacter(char c, int x, int y, char color1, char color2){
    char d = c - 0x20;
    int i, j;
    for (i = 0; i < 5; i++){
        if (x+i < 128){
            for (j = 0; j < 8; j++){
                if (y + j < 128){
                    if (ASCII[d][i] >> j & 1 == 1){
                        LCD_drawPixel(x + i, y + j, color1);
                    }
                    else{
                        LCD_drawPixel(x + i, y + j, color2);
                    }
                }
            }
        }
    }
}


void drawString(char *msg, int x, int y, char color1, char color2){
    int i = 0;
    while(msg[i]){
        drawCharacter(msg[i], x+6*i, y, color1, color2);  // leave a blank for text
        i++;
    }
}


void drawBar(int x, int y, char color1, char color2, int len, int maxlen, int w){
    int i, j;
    int l = len;
    int l2 = maxlen;
    for (i = 0; i < l; i++){
        for (j = 0; j < w; j++){
            LCD_drawPixel(x + i, y + j, color1);
        }
    }
    for (i = l; i < l2; i++){
        for (j = 0; j < w; j++){
            LCD_drawPixel(x + i, y + j, color2);
        }
    }
}

void drawBar_x(int x, int y, char color1, char color2, signed char acc_x, int w){
    int i, j;
    if(acc_x > 0){
        for (i = 0; i < 50; i++){
            if (i < acc_x){
                for (j = 0; j < w; j++){
                    LCD_drawPixel(x + i, y + j, color1);
                }
            }
            else{
                for (j = 0; j < w; j++){
                    LCD_drawPixel(x + i, y + j, color2);
                }
            }
            for (j = 0; j< w; j++){
                LCD_drawPixel(x - i, y + j, color2);
            }
        }
    }
    else{
        acc_x = -acc_x;
        for (i = 0; i < 50; i++){
            if (i < acc_x){
                for (j = 0; j < w; j++){
                    LCD_drawPixel(x - i, y + j, color1);
                }
            }
            else{
                for (j = 0; j < w; j++){
                    LCD_drawPixel(x - i, y + j, color2);
                }
            }
            for (j = 0; j< w; j++){
                LCD_drawPixel(x + i, y + j, color2);
            }
        }
    }
    
}

void drawBar_y(int x, int y, char color1, char color2, signed char acc_y, int w){
    int i, j;
    if(acc_y > 0){
        for (i = 0; i < 50; i++){
            if (i < acc_y){
                for (j = 0; j < w; j++){
                    LCD_drawPixel(x + j, y + i, color1);
                }
            }
            else{
                for (j = 0; j < w; j++){
                    LCD_drawPixel(x + j, y + i, color2);
                }
            }
//            for (j = 0; j< w; j++){
//                LCD_drawPixel(x + j, y - i, color2);
//            }
        }
    }
    else{
        acc_y = -acc_y;
        for (i = 0; i < 50; i++){
            if (i < acc_y){
                for (j = 0; j < w; j++){
                    LCD_drawPixel(x + j, y - i, color1);
                }
            }
            else{
                for (j = 0; j < w; j++){
                    LCD_drawPixel(x + j, y - i, color2);
                }
            }
//            for (j = 0; j< w; j++){
//                LCD_drawPixel(x + j, y + i, color2);
//            }
        }
    }
    
}


int main() {
    
    __builtin_disable_interrupts();

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // do your TRIS and LAT commands here
    TRISAbits.TRISA4 = 0;  // LED as an output button
    LATAbits.LATA4 = 0;    // set high
    TRISBbits.TRISB4 = 1;  // button as an input button
    
    // turn odd analog B2, B3
    ANSELBbits.ANSB2 = 0;
    ANSELBbits.ANSB3 = 0;
    
    SPI1_init();
    LCD_init();
    LCD_clearScreen(BLACK);
    
    IMU_init();
    unsigned char imu_addr = 0;
    imu_addr = WhoAmI();
    
    __builtin_enable_interrupts();  
   
    while(1) {
        int num = 1;
        int i,j;
        for (num; num < 101; num++){
            unsigned char output[20];  
            unsigned char imu_data[14];
            float acc_x;
            float acc_y;
            float acc_z;
            I2C_read_multiple(IMU_ADDR, 0x20, imu_data, 14);
            sprintf(output, "Address is %d", imu_addr);
            drawString(output, 5, 5, WHITE, BLACK);

            sprintf(output, "x");
            drawString(output, 5, 61, WHITE, BLACK);
            sprintf(output, "y");
            drawString(output, 61, 115, WHITE, BLACK);
            
//            for(i = 0; i < 5; i++){
//                for (j = 0; j < 5; j++){
//                    LCD_drawPixel(62 + i, 62 + j, WHITE);
//                }
//            }

            acc_x = ((float)(get_acc_x(imu_data)))*0.0061*100;
            acc_y = ((float)(get_acc_y(imu_data)))*0.0061*100;
            acc_z = ((float)(get_acc_z(imu_data)))*0.0061*100;

            drawBar_y(62, 62, YELLOW, BLACK, ((signed char) (acc_z)), 5);
            drawBar_x(62, 62, BLUE, BLACK, ((signed char) (acc_y)), 5);
       
            while(_CP0_GET_COUNT() < 4800000){
                ;
            }
        }
        LCD_clearScreen(BLACK);
    }
    return 0;
}


