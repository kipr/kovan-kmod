/*
 * kovan-kmod-spi.c
 *
 *  Created on: Nov 10, 2012
 *      Author: josh
 */

#include "kovan-kmod-spi.h"
#include "protocol.h"

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/irq.h>
#include <linux/interrupt.h>

#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */
#include <asm/io.h>
#include <asm/irq.h>
//#include <mach/irqs.h>

#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <mach/mfp-pxa168.h>
#include <mach/regs-apbc.h>
#include <mach/gpio.h>
#include <plat/generic.h>
#include <mach/addr-map.h>
#include <asm/delay.h>
#include <mach/regs-ssp.h>


#define PXA_LOOPBACK_TEST 0

#ifndef SSCR0_MOD
#define SSCR0_MOD	(1 << 31)
#endif

#define ARRAY_AND_SIZE(x)       (x), ARRAY_SIZE(x)
#define REG32(x)                (*(volatile unsigned long *)(x))



#define SSP3_SSCR0		(APB_VIRT_BASE + 0x1f000)
#define SSP3_SSCR1		(APB_VIRT_BASE + 0x1f004)
#define SSP3_SSSR		(APB_VIRT_BASE + 0x1f008)
#define SSP3_SSPSP		(APB_VIRT_BASE + 0x1f02c)
#define SSP3_SSDR		(APB_VIRT_BASE + 0x1f010)
#define SSP3_SSTSA		(APB_VIRT_BASE + 0x1f030)
#define SSP3_SSRSA		(APB_VIRT_BASE + 0x1f034)

// The FPGA currently has 64 registers but can address up to 1024
// The 6 most significant bits in a message are reserved currently for command type
// This would allow for 32 different commands, however the fpga is currently
// pretending only the 2 most significant bits are important
// resulting in:
// 10 = read
// 01 = write
// 00 = start/undefined
// 11 = start/undefined
#define START_CMD 0x0000
#define WRITE_CMD 0x4000
#define READ_CMD 0x8000


int kovan_write_u8(unsigned char data)
{
	unsigned int sssr = __raw_readl(SSP3_SSSR);

	if ((sssr & 0x00000f00) == 0x00000f00){
		printk("Can't write %d\n",data);
		return 0;
	}

	__raw_writeb(data, SSP3_SSDR);
	return 1;
}


int kovan_write_u16(unsigned short data)
{
	unsigned int sssr = __raw_readl(SSP3_SSSR);

	if ((sssr & 0x00000f00) == 0x00000f00){
		printk("Can't write %d\n",data);
		return 0;
	}

	__raw_writel(data, SSP3_SSDR);
	return 1;
}


int rx_empty(void)
{

	unsigned int sssr = __raw_readl(SSP3_SSSR);

	if (sssr & SSSR_RNE){
		// not empty
		return 0;
	}else{
		// empty
		return 1;
	}
}


int spi_busy(void){

	if (__raw_readl(SSP3_SSSR) & SSSR_BSY){}; return 1;

	return 0;
}


int kovan_flush_rx(void)
{
	unsigned int ssp3_val = 0;
	unsigned int i;

	for (i = 0; i < 16; i++){
		ssp3_val = __raw_readl(SSP3_SSDR);
		udelay(1); // FIXME
	}

	return 1;
}


unsigned int kovan_read_u32(void)
{

	unsigned int data = 0;
	unsigned int ssp3_val = 0;

	do{
		ssp3_val = __raw_readl(SSP3_SSSR);
	}while(ssp3_val & SSSR_BSY);
	udelay(1); // FIXME
	data = __raw_readl(SSP3_SSDR); // play with the idea that this is getting optimized out

	return data;
}


unsigned short kovan_read_u16(void)
{
	unsigned short data = 0;
	unsigned int ssp3_val = 0;
	do{
		ssp3_val = __raw_readl(SSP3_SSSR);
	}while(ssp3_val & SSSR_BSY);
	udelay(1); // FIXME
	data = __raw_readl(SSP3_SSDR); // play with the idea that this is getting optimized out

	return data;
}


void print_spi_regs(void)
{
	unsigned int sssr = __raw_readl(SSP3_SSSR);
	unsigned int sscr0 = __raw_readl(SSP3_SSCR0);
	unsigned int sscr1 = __raw_readl(SSP3_SSCR1);
	unsigned int sspsp = __raw_readl(SSP3_SSPSP);
	printk("SPI Regs: \n\tSSSR=%x \n\tSSCR0=%x \n\tSSCR1=%x \n\tSSPSP=%x\n",
			sssr, sscr0, sscr1, sspsp);

}


void print_rx_buffer(unsigned short *buff_rx, unsigned short num_spi_regs)
{

	unsigned int i;

	for (i = 0; i < num_spi_regs; i+=8){
		printk("%d:%d [%d,%d,%d,%d,%d,%d,%d,%d]\n",i,i+7,
				 buff_rx[i], buff_rx[i+1], buff_rx[i+2], buff_rx[i+3],
				 buff_rx[i+4], buff_rx[i+5], buff_rx[i+6], buff_rx[i+7]);
	}
}


void init_spi(void)
{

	unsigned long pin_config[] = {
		/* FPGA PSP config */
		MFP_CFG(GPIO90, AF3), /* ssp3_clk */
		MFP_CFG(GPIO91, AF3), /* ssp3_frm */
		MFP_CFG(GPIO92, AF3), /* ssp3_miso */
		MFP_CFG(GPIO93, AF3), /* ssp3_mosi */
	};

	mfp_config(ARRAY_AND_SIZE(pin_config));

	printk("SSP3 clock enable (26 MHz)\n" );
	__raw_writel(0x23,(APB_VIRT_BASE + 0x1584c));
	__raw_writel(0x27,(APB_VIRT_BASE + 0x1584c)); // reset the unit
	__raw_writel(0x23,(APB_VIRT_BASE + 0x1584c));


	// frame width 1f   (0x7= 8 clocks)
	int sspsp = 0 | SSPSP_SFRMWDTH(0x15); // | SSPSP_SCMODE(3); // frame active low  (SSRP_SFRMP doesnt seem to work ,have to use SSCR1_IFR)

	int sscr1 = 0; // | SSCR1_SPH | SSCR1_SPO;  // mode 3
	if (PXA_LOOPBACK_TEST) sscr1 |= SSCR1_LBM;

	int sscr0 = 0 | SSCR0_MOD | SSCR0_Motorola | SSCR0_DataSize(16);

	__raw_writel(sspsp, SSP3_SSPSP); // continuous clock on SSP3
	__raw_writel(sscr1, SSP3_SSCR1); // this is actually a bizarre configuration
	__raw_writel(sscr0, SSP3_SSCR0); // but it works.
	__raw_writel(sscr0, SSP3_SSCR0); // but it works.

	__raw_writel(1, SSP3_SSTSA);
	__raw_writel(1, SSP3_SSRSA);

	// enable ssp
	sscr0 = sscr0 | SSCR0_SSE;
	__raw_writel(sscr0, SSP3_SSCR0);

}


void read_vals(unsigned short *buff_rx, unsigned short num_vals_to_read)
{
	int i;
	unsigned int ssp3_val = 0;


	//make sure we get back to a waiting state
	kovan_write_u16(START_CMD);
	udelay(1);
	kovan_write_u16(START_CMD);
	udelay(1);

	kovan_flush_rx();

	// init xfer
	kovan_write_u16(READ_CMD);
	buff_rx[0] = kovan_read_u16();

	// read vals
	for (i = 0; i < num_vals_to_read; i++){
		kovan_write_u16(READ_CMD);
		buff_rx[i] = kovan_read_u16();
	}
}

void write_vals(unsigned short *addys, unsigned short *vals, unsigned short num_vals_to_send)
{
	unsigned short i;
	unsigned short buff_rx_tmp[NUM_RW_REGS]; // 40 command registers
	unsigned int ssp3_val = 0;

	//make sure we get back to a waiting state
	kovan_write_u16(START_CMD);
	udelay(1);
	kovan_write_u16(START_CMD);
	udelay(1);

	kovan_flush_rx();

	for (i = 0; i < num_vals_to_send; i++){

		// init write + send register
		kovan_write_u16(WRITE_CMD + addys[i]);

		// send value, complete write
		kovan_write_u16(vals[i]);
		buff_rx_tmp[i] = kovan_read_u16();
	}

	//TODO: check return value from each write?
}



//  can write to registers 24-63
void write_test(void)
{
	unsigned short i;
	unsigned short first_reg = RW_REG_OFFSET;
	unsigned short last_reg = NUM_FPGA_REGS-1;

	unsigned short num_vals_to_send = NUM_RW_REGS;
	unsigned short vals[NUM_RW_REGS]; // 40 command registers
	unsigned short addys[NUM_RW_REGS]; // 40 command registers

	unsigned int addy_cnt = 0;

	for (i = first_reg; i <= last_reg; i++){
		vals[addy_cnt] = i;
		addys[addy_cnt] = i;
		addy_cnt+=1;
	}

	write_vals(addys, vals, num_vals_to_send);
}


void spi_test(void)
{

	int num_regs = NUM_RW_REGS;
	unsigned short rx_buff[NUM_FPGA_REGS];

	// check spi config
	print_spi_regs();

	printk("Reading...\n");
	read_vals(rx_buff, num_regs);
	print_rx_buffer(rx_buff, num_regs);


	printk("Writing...\n");
	write_test();


	printk("Reading...\n");
	read_vals(rx_buff, num_regs);
	print_rx_buffer(rx_buff, num_regs);

	// check spi config
	print_spi_regs();
}
