#ifndef IMU_H__
#define IMU_H__
// Header file for IMU.c
// implements high-level IMU functions using I2C

#define IMU_ADDR 0x6B // I2C hardware address 

void IMU_init(void);        // Init IMU
unsigned char WhoAmI(void); // use this to confirm communication with LSM6DS33 IMU
void I2C_read_multiple(unsigned char address, unsigned char reg, unsigned char * data, int length);
signed short get_ang_x(unsigned char * data);
signed short get_ang_y(unsigned char * data);
signed short get_ang_z(unsigned char * data);
signed short get_acc_x(unsigned char * data);
signed short get_acc_y(unsigned char * data);
signed short get_acc_z(unsigned char * data);

#endif
