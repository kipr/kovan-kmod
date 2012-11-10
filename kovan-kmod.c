/* kovan-kmod.c
 *
 * A kernel module to provide udp access to Kovan hardware
 *
 * Joshua Southerland and Braden McDorman
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

#include "kovan-kmod-spi.h"
#include "protocol.h"

#define SERVER_PORT 5555

static struct socket *udpsocket = NULL;
static struct socket *clientsocket = NULL;

struct wq_wrapper
{
        struct work_struct worker;
	struct sock *sk;
};

struct StateResponse
{
	unsigned char hasState : 1;
	struct State state;
};

struct wq_wrapper wq_data;

// static DECLARE_COMPLETION( threadcomplete );
struct workqueue_struct *wq;

void cb_data(struct sock *sk, int bytes)
{
	printk("Message Received\n");

	wq_data.sk = sk;
	queue_work(wq, &wq_data.worker);
}

struct StateResponse state()
{
	State ret;
	ret.t0 = 1;
	ret.t1 = 2;
	ret.t2 = 3;
	
	struct StateResponse respanse;
	response.hasState = 1;
	response.state = ret;
	return response;
}

void motor_command(struct MotorCommand *cmd)
{
	spi_test();
	
	return 0;
}

void digital_command(struct DigitalCommand *cmd)
{
	// Digital Command
	
	return 0;
}

unsigned char *do_packet(unsigned char *data, const unsigned int size)
{
	if(size != sizeof(struct Packet)) {
		return 0; // Error: Packet is too small?
	}
	
	struct Packet *packet = (struct Packet *)data;
	
	struct StateResponse response;
	memset(&response, 0, sizeof(StateResponse));
	
	for(unsigned short i = 0; i < packet->num; ++i) {
		struct Command cmd = packet->commands[i];
		
		switch(cmd.type) {
		case StateCommandType:
			ret = state();
			break;
		
		case MotorCommandType:
			motor_command((struct MotorCommand *)cmd.data);
			break;
		
		case DigitalCommandType:
			digital_command((struct DigitalCommand *)cmd.data);
			break;
		
		default: break;
		}
	}
	
	return ret;
}

#define UDP_HEADER_SIZE 8

void do_work(struct work_struct *data)
{
	struct wq_wrapper *foo = container_of(data, struct wq_wrapper, worker);
	
	int len = 0;
	while((len = skb_queue_len(&foo->sk->sk_receive_queue)) > 0) {
		struct sk_buff *skb = skb_dequeue(&foo->sk->sk_receive_queue);
		printk("Message Length: %i Message: %s\n", skb->len - UDP_HEADER_SIZE, skb->data + UDP_HEADER_SIZE);
		struct StateResponse response = do_packet(skb->data + UDP_HEADER_SIZE, skb->len - UDP_HEADER_SIZE);
		
		if(!response.hasState) continue;
		
		struct sockaddr_in to;
		memset(&to, 0, sizeof(to));
		to.sin_family = AF_INET;
		to.sin_addr.s_addr = skb->addr;
		to.sin_port = *((unsigned short *)skb->data);
		
		struct msghdr msg;
		memset(&msg, 0, sizeof(msg));
		msg.msg_name = &to;
		msg.msg_namelen = sizeof(to);
		
		struct iovec iov;
		iov.iov_base = &response->state;
		iov.iov_len  = sizeof(State);
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		
		/* adjust memory boundaries */
 		mm_segment_t oldfs = get_fs();
		set_fs(KERNEL_DS);
		
		sock_sendmsg(clientsocket, &msg, iov.iov_len);
		
		set_fs(oldfs);
		kfree_skb(skb);
		printk("Sent State Response\n");
	}
}

static int __init server_init(void)
{
	printk("Initing Kovan Module\n");

	init_spi();
	spi_test();

	/* socket to receive data */
	if(sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &udpsocket) < 0) {
		printk(KERN_ERR "server: Error creating udpsocket.n\n");
		return -EIO;
	}
	
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons((unsigned short)SERVER_PORT);
	
	if(udpsocket->ops->bind(udpsocket, (struct sockaddr *)&server, sizeof(server))) {
		sock_release(udpsocket);
		return -EIO;
	}
	udpsocket->sk->sk_data_ready = cb_data;

	/* create work queue */
	INIT_WORK(&wq_data.worker, do_work);
	wq = create_singlethread_workqueue("myworkqueue");
	if(!wq) {
		return -ENOMEM;
	}

	if(sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &clientsocket) < 0) {
		printk(KERN_ERR "server: Error creating clientsocket.n\n");
		return -EIO;
	}
	return 0;
}

static void __exit server_exit(void)
{
	if(udpsocket) sock_release(udpsocket);
	if(clientsocket) sock_release(clientsocket);

	if(wq) {
                flush_workqueue(wq);
                destroy_workqueue(wq);
	}
	
	printk("Exiting Kovan Module\n");
}

module_init(server_init);
module_exit(server_exit);
MODULE_LICENSE("GPL");
