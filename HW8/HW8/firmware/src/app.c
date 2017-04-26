#include "app.h"
#include "ILI9163C.h"
#include "i2c_master_noint.h"
#include "IMU.h"
#include <sys/attribs.h> // __ISR macro
#include <math.h>
#include <stdio.h>

APP_DATA appData;

void APP_Initialize(void) {
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;

    // do your TRIS and LAT commands here
    TRISAbits.TRISA4 = 0; // LED as an output button
    LATAbits.LATA4 = 0; // set high
    TRISBbits.TRISB4 = 1; // button as an input button

    // turn odd analog B2, B3
    ANSELBbits.ANSB2 = 0;
    ANSELBbits.ANSB3 = 0;

    SPI1_init();
    LCD_init();
    LCD_clearScreen(BLACK);

    IMU_init();

}

void drawCharacter(char c, int x, int y, char color1, char color2) {
    char d = c - 0x20;
    int i, j;
    for (i = 0; i < 5; i++) {
        if (x + i < 128) {
            for (j = 0; j < 8; j++) {
                if (y + j < 128) {
                    if (ASCII[d][i] >> j & 1 == 1) {
                        LCD_drawPixel(x + i, y + j, color1);
                    } else {
                        LCD_drawPixel(x + i, y + j, color2);
                    }
                }
            }
        }
    }
}

void drawString(char *msg, int x, int y, char color1, char color2) {
    int i = 0;
    while (msg[i]) {
        drawCharacter(msg[i], x + 6 * i, y, color1, color2); // leave a blank for text
        i++;
    }
}

void drawBar(int x, int y, char color1, char color2, int len, int maxlen, int w) {
    int i, j;
    int l = len;
    int l2 = maxlen;
    for (i = 0; i < l; i++) {
        for (j = 0; j < w; j++) {
            LCD_drawPixel(x + i, y + j, color1);
        }
    }
    for (i = l; i < l2; i++) {
        for (j = 0; j < w; j++) {
            LCD_drawPixel(x + i, y + j, color2);
        }
    }
}

void drawBar_x(int x, int y, char color1, char color2, signed char acc_x, int w) {
    int i, j;
    if (acc_x > 0) {
        for (i = 0; i < 50; i++) {
            if (i < acc_x) {
                for (j = 0; j < w; j++) {
                    LCD_drawPixel(x + i, y + j, color1);
                }
            } else {
                for (j = 0; j < w; j++) {
                    LCD_drawPixel(x + i, y + j, color2);
                }
            }
        }
    } else {
        acc_x = -acc_x;
        for (i = 0; i < 50; i++) {
            if (i < acc_x) {
                for (j = 0; j < w; j++) {
                    LCD_drawPixel(x - i, y + j, color1);
                }
            } else {
                for (j = 0; j < w; j++) {
                    LCD_drawPixel(x - i, y + j, color2);
                }
            }
        }
    }

}

void drawBar_y(int x, int y, char color1, char color2, signed char acc_y, int w) {
    int i, j;
    if (acc_y > 0) {
        for (i = 0; i < 50; i++) {
            if (i < acc_y) {
                for (j = 0; j < w; j++) {
                    LCD_drawPixel(x + j, y + i, color1);
                }
            } else {
                for (j = 0; j < w; j++) {
                    LCD_drawPixel(x + j, y + i, color2);
                }
            }
        }
    } else {
        acc_y = -acc_y;
        for (i = 0; i < 50; i++) {
            if (i < acc_y) {
                for (j = 0; j < w; j++) {
                    LCD_drawPixel(x + j, y - i, color1);
                }
            } else {
                for (j = 0; j < w; j++) {
                    LCD_drawPixel(x + j, y - i, color2);
                }
            }
        }
    }

}

void APP_Tasks(void) {

    /* Check the application's current state. */
    switch (appData.state) {
            /* Application's initial state. */
        case APP_STATE_INIT:
        {
            bool appInitialized = true;


            if (appInitialized) {

                appData.state = APP_STATE_SERVICE_TASKS;
            }
            break;
        }

        case APP_STATE_SERVICE_TASKS:
        {
            // LED 

            LATAbits.LATA4 = 0; // turn off the LED
            _CP0_SET_COUNT(0); // set core time to 0
            while (_CP0_GET_COUNT() < 24000) {
                while (PORTBbits.RB4 == 0) {
                    LATAbits.LATA4 = 0; // turn off the LED
                }
            }

            LATAbits.LATA4 = 1; // turn on the LED
            _CP0_SET_COUNT(0); // set core time to 0
            while (_CP0_GET_COUNT() < 24000) {
                while (PORTBbits.RB4 == 0) {
                    LATAbits.LATA4 = 0; // turn off the LED
                }
            }


            // LCD part
            unsigned char imu_addr = 0;
            imu_addr = WhoAmI();

            int num = 1;
            int i, j;
            unsigned char output[20];
            unsigned char imu_data[14];
            float acc_x;
            float acc_y;
            I2C_read_multiple(IMU_ADDR, 0x20, imu_data, 14);
            sprintf(output, "Address is %d", imu_addr);
            drawString(output, 5, 5, WHITE, BLACK);

            sprintf(output, "x");
            drawString(output, 5, 61, WHITE, BLACK);
            sprintf(output, "y");
            drawString(output, 61, 115, WHITE, BLACK);


            acc_x = ((float) (get_acc_x(imu_data)))*0.0061 * 100;
            acc_y = ((float) (get_acc_y(imu_data)))*0.0061 * 100;

            drawBar_y(62, 62, YELLOW, BLACK, ((signed char) (acc_y)), 5);
            drawBar_x(62, 62, BLUE, BLACK, ((signed char) (acc_x)), 5);

            while (_CP0_GET_COUNT() < 4800000) {
                ;
            } 
           break;
        }

            /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}

