/*
 * i2c.h
 *
 * Created: 10/30/2011 11:21:11 PM
 *  Author: steve
 */ 


#ifndef I2C_H_
#define I2C_H_

void i2cSetAddress(uint8_t _slaveAddr);
void i2cInit(void);
uint8_t i2cReadBytes(uint16_t addr, int len, uint8_t *buf);
uint8_t i2cWriteByte(uint16_t addr, uint8_t val);


#endif /* I2C_H_ */