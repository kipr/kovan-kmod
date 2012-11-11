/*
 * kovan-kmod-spi.h
 *
 *  Created on: Nov 10, 2012
 *      Author: josh
 */

#ifndef KOVAN_KMOD_SPI_H_
#define KOVAN_KMOD_SPI_H_

int kovan_write_u8(unsigned char data);

int kovan_write_u16(unsigned short data);

int rx_empty(void);

inline int spi_busy(void);

int kovan_flush_rx(void);

void print_spi_regs(void);

void init_spi(void);

void spi_test(void);


#endif /* KOVAN_KMOD_SPI_H_ */
