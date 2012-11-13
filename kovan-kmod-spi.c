/*
 * kovan-kmod-spi.c
 *
 *  Created on: Nov 10, 2012
 *      Author: josh
 */

#include "kovan-kmod-spi.h"




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
#include <mach/irqs.h>

#include <linux/platform_device.h>
#include <linux/mutex.h>
/* #include <linux/drivers/char/kovan_xilinx.h> */
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



//static int u8_writer(struct driver_data *drv_data)
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


//static int u8_writer(struct driver_data *drv_data)
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


int rx_empty(void){

	unsigned int sssr = __raw_readl(SSP3_SSSR);

	if (sssr & SSSR_RNE){
		// not empty
		return 0;
	}else{
		// empty
		return 1;
	}
}

inline int spi_busy(void){

	if (__raw_readl(SSP3_SSSR) & SSSR_BSY){}; return 1;

	return 0;
}


//static int u8_writer(struct driver_data *drv_data)
int kovan_flush_rx(void)
{
	char data = 0;

	while (!rx_empty()) {
		data = __raw_readl(SSP3_SSDR);
		printk("Read %d\n",data);
	}

	return 1;
}

//static int u8_writer(struct driver_data *drv_data)
int kovan_read_u32(void)
{
	char data = 0;
	data = __raw_readl(SSP3_SSDR);
	printk("Read %d\n",data);

	return 1;
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


void read_vals()
{
//	printk("Reading registers...\n");

	static unsigned int num_vals_to_send = 64;
	short buff_rx[num_vals_to_send];

	int i;

	//FIXME make sure we get back to a waiting state
	kovan_write_u16(0x0000);
	udelay(1);
	kovan_write_u16(0x0000);
	udelay(1);


	// FIXME:
	for (i = 0; i < 16; i++){
		__raw_readl(SSP3_SSDR);
		udelay(1);
	}


	// init xfer
	kovan_write_u16(0x8000);

	unsigned int ssp3_val = 0;
	do{
		ssp3_val == __raw_readl(SSP3_SSSR);
	}while(ssp3_val & SSSR_BSY);
	udelay(1); // FIXME
	__raw_readl(SSP3_SSDR);


	// read vals
	for (i = 0; i < num_vals_to_send; i++){

		kovan_write_u16(0x8000);

		do{
			ssp3_val = __raw_readl(SSP3_SSSR);
		}while(ssp3_val & SSSR_BSY);
		udelay(1); // FIXME
		buff_rx[i] = __raw_readl(SSP3_SSDR);
	}

//	for (i = 0; i < num_vals_to_send; i+=8){
//		printk("%d:%d [%d,%d,%d,%d,%d,%d,%d,%d]\n",i,i+7,
//				 buff_rx[i], buff_rx[i+1], buff_rx[i+2], buff_rx[i+3],
//				 buff_rx[i+4], buff_rx[i+5], buff_rx[i+6], buff_rx[i+7]);
//	}
}


void write_test()
{

	//  can write to registers 24-63


	unsigned short i;
	unsigned short first_reg = 24;
	unsigned short last_reg = 63;
	unsigned short buff_rx[40]; // 40 command registers
//	printk("Writing to registers...\n");


	//FIXME make sure we get back to a waiting state
	kovan_write_u16(0x0000);
	udelay(1);
	kovan_write_u16(0x0000);
	udelay(1);

	// FIXME: flushing
	for (i = 0; i < 16; i++){
	__raw_readl(SSP3_SSDR);
	udelay(1);
	}





	unsigned short matches = 0;
	unsigned int ssp3_val = 0;
	for (i = first_reg; i <= last_reg; i++){

		// init write + send register
		kovan_write_u16(0x4000 + i);

		do{
			ssp3_val = __raw_readl(SSP3_SSSR);
		}while(ssp3_val & SSSR_BSY);
		udelay(1); // FIXME
		__raw_readl(SSP3_SSDR);

		// send value, complete write
		kovan_write_u16(i);
		do{
			ssp3_val == __raw_readl(SSP3_SSSR);
		}while(ssp3_val & SSSR_BSY);
		udelay(1); // FIXME
		buff_rx[i-first_reg] = __raw_readl(SSP3_SSDR);

		if (buff_rx[i-first_reg] == i-1) matches += 1;
	}

//	printk("Writing complete.  %d/%d matched\n", matches, (last_reg-first_reg)+1);
}


void spi_test(void){
	// check spi config
	print_spi_regs();

	int i = 0;

	printk("start read...\n");
	for (i = 0; i < 100; i++){
		read_vals();
	}
	printk("done 100 reads, start write:...\n");
	for (i = 0; i < 100; i++){
		write_test();
	}
	printk("done 100 writes\n");




	// check spi config
	print_spi_regs();
}
