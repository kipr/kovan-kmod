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

#include "kovan-kmod-spi.h"


#define SERVER_PORT 5555

static struct socket *udpsocket=NULL;
static struct socket *clientsocket=NULL;


struct wq_wrapper
{
        struct work_struct worker;
	struct sock * sk;
};

struct wq_wrapper wq_data;



static DECLARE_COMPLETION( threadcomplete );
struct workqueue_struct *wq;



void cb_data(struct sock *sk, int bytes)
{
	printk("message received\n");

	spi_test();

	//wq_data.sk = sk;
	//queue_work(wq, &wq_data.worker);
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

	init_spi();
	spi_test();

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
	printk("EXIT MODULE\n");
}

module_init(server_init);
module_exit(server_exit);
MODULE_LICENSE("GPL");
