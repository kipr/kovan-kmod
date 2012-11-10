/* kovan-kmod.c
 *
 * A kernel module to provide udp access to Kovan hardware
 *
 * Joshua Southerland
 *
 * Based on
 * http://people.ee.ethz.ch/~arkeller/linux/kernel_user_space_howto.html#s3
 *
 */
#include <linux/in.h>
#include <net/sock.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/inet.h>

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>

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

#ifndef SSCR0_MOD
#define SSCR0_MOD	(1 << 31)
#endif

#define ARRAY_AND_SIZE(x)       (x), ARRAY_SIZE(x)
#define REG32(x)                (*(volatile unsigned long *)(x))


#define SERVER_PORT 5555

#define SSP3_SSCR0		(APB_VIRT_BASE + 0x1f000)
#define SSP3_SSCR1		(APB_VIRT_BASE + 0x1f004)
#define SSP3_SSSR		(APB_VIRT_BASE + 0x1f008)
#define SSP3_SSPSP		(APB_VIRT_BASE + 0x1f02c)
#define SSP3_SSDR		(APB_VIRT_BASE + 0x1f010)
#define SSP3_SSTSA		(APB_VIRT_BASE + 0x1f030)
#define SSP3_SSRSA		(APB_VIRT_BASE + 0x1f034)


static struct socket *udpsocket=NULL;
static struct socket *clientsocket=NULL;

static DECLARE_COMPLETION( threadcomplete );
struct workqueue_struct *wq;

struct wq_wrapper
{
        struct work_struct worker;
	struct sock * sk;
};

struct wq_wrapper wq_data;


//static int u8_writer(struct driver_data *drv_data)
int kovan_write_u8(unsigned char data)
{

	unsigned int sssr = __raw_readl(SSP3_SSSR);

	if ((sssr & 0x00000f00) == 0x00000f00){
		printk("Can't write %d\n",data);
		return 0;
	}

	__raw_writeb(data, SSP3_SSDR);

	//printk("Succeeded in writing %d\n", data);

	return 1;
}


//static int u8_writer(struct driver_data *drv_data)
int kovan_write_u16(unsigned int data)
{

	unsigned int sssr = __raw_readl(SSP3_SSSR);

	if ((sssr & 0x00000f00) == 0x00000f00){
		printk("Can't write %d\n",data);
		return 0;
	}
	__raw_writel(data, SSP3_SSDR);

	//printk("Succeeded in writing %d\n", data);

	return 1;
}


static int rx_empty(){

	unsigned int sssr = __raw_readl(SSP3_SSSR);

	if (sssr & SSSR_RNE){
		return 0; // not empty
	}else{
		return 1; // empty
	}
}



//static int u8_writer(struct driver_data *drv_data)
int kovan_flush_rx()
{
	char data = 0;

	while (__raw_readl(SSP3_SSSR) & SSSR_RNE) {
		data = __raw_readl(SSP3_SSDR);
		printk("Read %d\n",data);
	}

	return 1;
}

void print_spi_regs()
{
	unsigned int sssr = __raw_readl(SSP3_SSSR);
	unsigned int sscr0 = __raw_readl(SSP3_SSCR0);
	unsigned int sscr1 = __raw_readl(SSP3_SSCR1);
	unsigned int sspsp = __raw_readl(SSP3_SSPSP);
	printk("SPI Regs: \n\tSSSR=%x \n\tSSCR0=%x \n\tSSCR1=%x \n\tSSPSP=%x\n",
			sssr, sscr0, sscr1, sspsp);

}


void cb_data(struct sock *sk, int bytes)
{
	printk("message received\n");

	  __raw_writel(0xdeadbeef, SSP3_SSDR);
	  __raw_writel(0xdeadbeef, SSP3_SSDR);


	wq_data.sk = sk;
	queue_work(wq, &wq_data.worker);
}

void send_answer(struct work_struct *data)
{
	struct  wq_wrapper * foo = container_of(data, struct  wq_wrapper, worker);
	int len = 0;
	/* as long as there are messages in the receive queue of this socket*/
	while((len = skb_queue_len(&foo->sk->sk_receive_queue)) > 0){
		struct sk_buff *skb = NULL;
		unsigned short * port;
		/* int len; */
		struct msghdr msg;
		struct iovec iov;
		mm_segment_t oldfs;
		struct sockaddr_in to;

		/* receive packet */
		skb = skb_dequeue(&foo->sk->sk_receive_queue);
		printk("message len: %i message: %s\n", skb->len - 8, skb->data+8); /*8 for udp header*/

		/* generate answer message */
		memset(&to,0, sizeof(to));
		to.sin_family = AF_INET;
		to.sin_addr.s_addr = in_aton("127.0.0.1");
		port = (unsigned short *)skb->data;
		to.sin_port = *port;
		memset(&msg,0,sizeof(msg));
		msg.msg_name = &to;
		msg.msg_namelen = sizeof(to);
		/* send the message back */
		iov.iov_base = skb->data+8;
		iov.iov_len  = skb->len-8;
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		/* adjust memory boundaries */
		oldfs = get_fs();
		set_fs(KERNEL_DS);
		/* len = sock_sendmsg(clientsocket, &msg, skb->len-8); */
		sock_sendmsg(clientsocket, &msg, skb->len-8);
		set_fs(oldfs);
		/* free the initial skb */
		kfree_skb(skb);
		printk("message sent\n");
	}
}

static int __init server_init( void )
{



	struct sockaddr_in server;
	int servererror;
	printk("INIT MODULE\n");


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
	int sspsp = 0 | SSPSP_SFRMWDTH(0x7);

	// SFRMDIR    1, slave mode, SSPx port received SSPx_FRM
	int sscr1 = 0 | SSCR1_LBM;// | SSCR1_SFRMDIR;

	//int sscr0 = 0 | SSCR0_EDSS | SSCR0_PSP | SSCR0_DataSize(8); // 8 bits
	int sscr0 = 0 | SSCR0_MOD | SSCR0_Motorola | SSCR0_DataSize(8); // 8 bits


	__raw_writel(sspsp, SSP3_SSPSP); // continuous clock on SSP3
	__raw_writel(sscr1, SSP3_SSCR1); // this is actually a bizarre configuration
	__raw_writel(sscr0, SSP3_SSCR0); // but it works.
	__raw_writel(sscr0, SSP3_SSCR0); // but it works.

	__raw_writel(1, SSP3_SSTSA);
	__raw_writel(1, SSP3_SSRSA);

	// enable ssp
	sscr0 = sscr0 | SSCR0_SSE;
	__raw_writel(sscr0, SSP3_SSCR0);

	// check spi config
	print_spi_regs();

	// flush rx buffer
	if (!rx_empty()) kovan_flush_rx();

	// check spi config
	print_spi_regs();

	// writing to spi
	printk("Writing to SPI...\n");

	int i;
	char buff_tx[100];
	char buff_rx[100];

	for (i = 0; i < 100; i++){
		buff_tx[i] = i;
		buff_rx[i] = 0;
	}

	for (i = 0; i < 100; i++){
		kovan_write_u8(0xAE);//buff_tx[i]);
		buff_rx[i] = __raw_readl(SSP3_SSDR);
	}
	for (i = 0; i < 100; i++){
		printk("Wrote %d Read %d\n",buff_tx[i], buff_rx[i]);
	}

	printk("Wrote %d times\n",i);

	// check spi config
	print_spi_regs();





	/* socket to receive data */
	if (sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &udpsocket) < 0) {
		printk( KERN_ERR "server: Error creating udpsocket.n" );
		return -EIO;
	}
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( (unsigned short)SERVER_PORT);
	servererror = udpsocket->ops->bind(udpsocket, (struct sockaddr *) &server, sizeof(server ));
	if (servererror) {
		sock_release(udpsocket);
		return -EIO;
	}
	udpsocket->sk->sk_data_ready = cb_data;

	/* create work queue */
	INIT_WORK(&wq_data.worker, send_answer);
	wq = create_singlethread_workqueue("myworkqueue");
	if (!wq){
		return -ENOMEM;
	}

	/* socket to send data */
	if (sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &clientsocket) < 0) {
		printk( KERN_ERR "server: Error creating clientsocket.n" );
		return -EIO;
	}
	return 0;
}

static void __exit server_exit( void )
{
	if (udpsocket)
		sock_release(udpsocket);
	if (clientsocket)
		sock_release(clientsocket);

	if (wq) {
                flush_workqueue(wq);
                destroy_workqueue(wq);
	}
	printk("EXIT MODULE");
}

module_init(server_init);
module_exit(server_exit);
MODULE_LICENSE("GPL");
