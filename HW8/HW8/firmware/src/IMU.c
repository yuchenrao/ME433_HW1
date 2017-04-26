#include <xc.h>
#include "IMU.h"
#include "i2c_master_noint.h"

void IMU_init(void){
    
    i2c_master_setup();                     // init I2C2, which we use as a master  
    
    i2c_master_start();                     // Begin the start sequence
    i2c_master_send(IMU_ADDR << 1);         // send the slave address, left shifted by 1, 
    i2c_master_send(0x10);                  // CTRL1_XL address: 0x10
    i2c_master_send(0x82);                  // set 1.66 kHz, with 2g sensitivity, and 100 Hz filter
    i2c_master_stop();
     
    i2c_master_start();                     // Begin the start sequence
    i2c_master_send(IMU_ADDR << 1);         // send the slave address, left shifted by 1, 
    i2c_master_send(0x11);                  // CTRL2_G address: 0x11
    i2c_master_send(0x88);                  // Set the sample rate to 1.66 kHz, with 1000 dps sensitivity.
    i2c_master_stop();
     
    i2c_master_start();                     // Begin the start sequence
    i2c_master_send(IMU_ADDR << 1);
    i2c_master_send(0x12);                  // CTRL3_C address: 0x11
    i2c_master_send(0x04);                  // default
    i2c_master_stop();                      // send STOP:  end transmission, give up bus
}

unsigned char WhoAmI(void){
    unsigned char reply;
    
    i2c_master_start();                     // Begin the start sequence
    i2c_master_send(IMU_ADDR << 1);       // send the slave address, left shifted by 1,                                           // which clears bit 0, indicating a write
    i2c_master_send(0x0F);                  // WhoAmI address: 0x0F
    i2c_master_restart();                   // restart i2c
    i2c_master_send((IMU_ADDR << 1)|1);   // // which clears bit 1, indicating a read
    reply = i2c_master_recv();              // receive a byte from the slave
    i2c_master_ack(1);                      // sends NACK = 1 (no more bytes requested from slave)
    i2c_master_stop();                      // send STOP:  end transmission, give up bus
    return reply;
    
}

void I2C_read_multiple(unsigned char address, unsigned char reg, unsigned char * data, int length){
    int i;
    int l = length;
    i2c_master_start();                     // Begin the start sequence
    i2c_master_send(address << 1);         // send the slave address, left shifted by 1, 
    i2c_master_send(reg);                  // CTRL1_XL address: 0x10
    i2c_master_restart();                   // restart i2c
    i2c_master_send((address << 1)|1);   // // which clears bit 1, indicating a read
    for(i=0; i < l+1; i++){
        data[i] = i2c_master_recv();              // receive a byte from the slave
        if(i == l){
            i2c_master_ack(1);                      // sends NACK = 1 (no more bytes requested from slave)
        }
        else{
            i2c_master_ack(0);                      // sends NACK = 0 
        }
    }
    i2c_master_stop();                      // send STOP:  end transmission, give up bus
}

signed char get_ang_x(unsigned char * data){
    return data[4] << 8 | data[3];
}

signed char get_ang_y(unsigned char * data){
    return data[6] << 8 | data[5];
}

signed char get_ang_z(unsigned char * data){
    return data[8] << 8 | data[7];
}

signed char get_acc_x(unsigned char * data){
    return data[10] << 8 | data[9];
}

signed char get_acc_y(unsigned char * data){
    return data[12] << 8 | data[11];
}

signed char get_acc_z(unsigned char * data){
    return data[14] << 8 | data[13];
}