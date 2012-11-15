/*
 * kovan-kmod-spi.h
 *
 *  Created on: Nov 10, 2012
 *      Author: josh
 */

#ifndef KOVAN_KMOD_SPI_H_
#define KOVAN_KMOD_SPI_H_

#define NUM_FPGA_REGS 64
#define NUM_RW_REGS 40
#define RO_REG_OFFSET 0
#define RW_REG_OFFSET 24

int kovan_write_u8(unsigned char data);

int kovan_write_u16(unsigned short data);

int rx_empty(void);

int spi_busy(void);

int kovan_flush_rx(void);

unsigned int kovan_read_u32(void);

unsigned short kovan_read_u16(void);

void print_spi_regs(void);

void print_rx_buffer(unsigned short *buff_rx, unsigned short num_spi_regs);

void init_spi(void);

void read_vals(unsigned short *buff_rx, unsigned short num_vals_to_read);

void write_vals(unsigned short *addys, unsigned short *vals, unsigned short num_vals_to_send);

void write_test(void);

void spi_test(void);

#endif /* KOVAN_KMOD_SPI_H_ */
