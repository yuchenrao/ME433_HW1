#include <xc.h> // processor SFR definitions
#include "i2c_master_noint.h"
#include <sys/attribs.h> // __ISR macro
#include <math.h>

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

// #define CS LATBbits.LATB8    // chip select pin

#define SLAVE_ADDR 0x20

void initExpander() {
    
    i2c_master_setup();                     // init I2C2, which we use as a master
    
    i2c_master_start();                     // Begin the start sequence
    i2c_master_send(SLAVE_ADDR << 1);       // send the slave address, left shifted by 1, 
                                            // which clears bit 0, indicating a write
    i2c_master_send(0x00);                  // IODIR address: 0x00
    i2c_master_send(0xF0);                  // set GP0-3 as outputs and GP4-7 as inputs
    i2c_master_stop();                      // send STOP:  end transmission, give up bus

}


void setExpander(char pin, char level) {
    
    i2c_master_start();                     // Begin the start sequence
    i2c_master_send(SLAVE_ADDR << 1);       // send the slave address, left shifted by 1, 
                                            // which clears bit 0, indicating a write
    i2c_master_send(0x0A);                  // OLAT address: 0x09
    i2c_master_send(level << pin);          // change the pin to the level(high/low)
    i2c_master_stop();                      // send STOP:  end transmission, give up bus
}


char getExpander() {
    
    char level;
    i2c_master_start();                     // Begin the start sequence
    i2c_master_send(SLAVE_ADDR << 1);       // send the slave address, left shifted by 1, 
                                            // which clears bit 0, indicating a write
    i2c_master_send(0x09);                  // GPIO address: 0x0A
    i2c_master_restart();                   // restart i2c
    i2c_master_send((SLAVE_ADDR << 1)|1);   // send the slave address, left shifted by 1, 
                                            // which clears bit 1, indicating a read
    level = i2c_master_recv();              // receive a byte from the slave
    i2c_master_ack(1);                      // sends NACK = 1 (no more bytes requested from slave)
    i2c_master_stop();                      // send STOP:  end transmission, give up bus
    return level;
            
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
   
    initExpander();
    
    __builtin_enable_interrupts();

    while(1) {
        setExpander(0,0);             // make LED(GP0) as low at the beginning
        while(!(getExpander()>>7)){  // when the button is pushed
            setExpander(0,1);         // make LED(GP0) as high 
        }
    }
    return 0;
}


