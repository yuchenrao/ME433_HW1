#include<xc.h> // processor SFR definitions

#include<sys/attribs.h> // __ISR macro
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

#define CS LATBbits.LATB8    // chip select pin

void initSPI1(){
TRISBbits.TRISB8 = 0;
CS = 1;
RPA1Rbits.RPA1R = 0b0011;   //OC1 to pin A1, output

SPI1CON = 0;
SPI1BUF;
SPI1BRG = 0x2;   //80000000/(2*desires)-1
SPI1STATbits.SPIROV = 0;
SPI1CONbits.CKE = 1;
SPI1CONbits.MSTEN = 1;  //master operation
SPI1CONbits.ON = 1;
}

unsigned char SPI1_IO(unsigned char write){
    SPI1BUF = write;
    while(!SPI1STATbits.SPIRBF){
        ;
    }
    return SPI1BUF;
}

void setVoltage(unsigned char channel, unsigned int voltage){
    unsigned char data1;
    unsigned char data2;
    //0b channel + 111 + voltage(H4)
    data1 = (channel << 7) | (0b01110000 | (voltage >> 4));
    //0b voltage(L4) + 0000
    data2 = voltage << 4;
    CS = 0;
    SPI1_IO(data1);
    SPI1_IO(data2);
    CS = 1;
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

initSPI1();   // init the SPI

unsigned int sinewave[50];
unsigned int triawave[100];
int i;
int counts = 1;
int countt = 1;

for (i = 0; i < 50; ++i){
    // create a sine wave
    sinewave[i] = 255.0/2.0 + (255.0/2.0)*sin(2*3.14*i/50.0);
}

for (i = 0; i < 100; ++i){
    // create a triangle wave
    triawave[i] = 255.0*i/100.0;
}

__builtin_enable_interrupts();

while(1) {
    _CP0_SET_COUNT(0);
    while(_CP0_GET_COUNT() < 24000){
        //update the value 1kHz
        ;
    }
    //send the value
    setVoltage(0, sinewave[counts]);
    _CP0_SET_COUNT(0);
    while(_CP0_GET_COUNT() < 24000){
        ;
    }
    setVoltage(1, triawave[countt]);
    counts++;
    countt++;
    if (counts == 50){
        counts = 0;
    }
    if (countt == 100){
        countt =0;
    }
    }
}



