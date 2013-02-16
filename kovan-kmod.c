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
#include <linux/ip.h>

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>

#include "kovan-kmod-spi.h"
#include "kovan-regs.h"
#include "protocol.h"

#include "pwm_curve_5v.h"

#define SERVER_PORT 5555

#define WRITE_COMMAND_BUFF_SIZE NUM_RW_REGS

#define KOVAN_KMOD_DEBUG 0
#define KOVAN_KMOD_WARN 0
#define KOVAN_KMOD_ERROR 1

static struct timer_list pid_timer;

static struct socket *udpsocket = NULL;
static struct socket *clientsocket = NULL;

static unsigned short kovan_regs[TOTAL_REGS];

//////

#define MOVE_AT_SPEED 0
#define MOVE_TO_POSITION 1
#define MOVE_TO_POSITION_AT_SPEED 2

#define DRIVE_CODE_BRAKE 0x3
#define DRIVE_CODE_FORWARD 0x2
#define DRIVE_CODE_REVERSE 0x1
#define DRIVE_CODE_IDLE 0x0

typedef struct
{
	int desired_position;
	int desired_speed;

	int position;
	int position_prev;

	int speed_prev;
	int err_prev;
	int err_integr;

	char mode;
	char status;

	short Kp_n;
	short Kp_d;

	short Ki_n;
	short Ki_d;

	short Kv_n;
	short Kv_d;

	short alpha; // out of 100

	unsigned short pwm_out;
	char drive_code;
	int pid_term_prev;
} pid_state;

pid_state pid_states[4];

void init_pid_state(pid_state *state)
{
	state->Kp_n = 40;//5;
	state->Ki_n = 40;//5;
	state->Kv_n = 5;//10;
	state->Kp_d = 100;//10;
	state->Ki_d = 100;//10;
	state->Kv_d = 100;//15;

	state->mode = 0;
	state->status = 0;

	state->desired_position = 0;
	state->desired_speed = 0;

	state->position = 0;
	state->position_prev = 0;

	state->speed_prev = 0;
	state->err_integr = 0;
	state->err_prev = 0;

	state->alpha = 75;

	state->pwm_out = 0;
	state->drive_code = 0;
	state->pid_term_prev = 0;
}

void step_pid(pid_state *state)
{
	//  printk("step_pid:\n");
	static const int GOAL_EPSILON = 10;

	static const short RESISTANCE = 0;//400;
	static const short MAX_PWM = 2600 - RESISTANCE;

	static const int MAX_I_ERROR = 5500; // TODO: not ideal.... is based on Ki_d and Ki_n
	static const int MIN_I_ERROR = -MAX_I_ERROR;


	int Pterm=0, Iterm=0, Dterm=0, PIDterm;


	int pos_err = state->desired_position - state->position;

	int filtered_speed = 25 * (state->position - state->position_prev); ////state->alpha * 25 * (state->position - state->position_prev)
	//	+ (100 - state->alpha) * state->speed_prev;
	//filtered_speed = filtered_speed / 100;


	int speed_err = state->desired_speed - filtered_speed;

	int err = 0;

	// Handle the different operating modes
	switch(state->mode & 0x3) {
	case 0:
		state->status = 0;
		return;
	case 1:
		if ((pos_err > 0 && pos_err < GOAL_EPSILON) || (pos_err < 0 && pos_err > -GOAL_EPSILON)) {
			// at the goal
			state->pwm_out = 0;
			state->status = 0;
			state->drive_code = DRIVE_CODE_BRAKE;
			state->desired_speed = 0;
			state->err_prev = 0;
			state->err_integr = 0;
			return;
		} else {
			err = pos_err;
			state->status = 1;
		}
		break;
	case 2:
		state->status = 1;
		err = speed_err;
		break;
	case 3:
		if ((pos_err > 0 && pos_err < GOAL_EPSILON)
				|| (pos_err < 0 && pos_err > -GOAL_EPSILON)
				|| (pos_err < 0 && state->desired_speed > 0)
				|| (pos_err > 0 && state->desired_speed < 0)) {
			// at the goal
			state->pwm_out = 0;
			state->status = 0;
			state->drive_code = DRIVE_CODE_BRAKE;
			//state->mode = 0;
			state->err_prev = 0;
			state->err_integr = 0;
			//printk("hit mtp goal!\n");
			return;
		} else {
			err = speed_err;
			state->status = 1;
		}
		break;
	}



	// Proportional Term
	////////////////////////////////////////////////////////////////////////////////
	Pterm = (state->Kp_n * err)/state->Kp_d;

	// Integral Term
	////////////////////////////////////////////////////////////////////////////////
	state->err_integr += err;
	// is the error within the maximum or minimum motor power range?
	if(state->err_integr > MAX_I_ERROR) state->err_integr = MAX_I_ERROR;
	else if(state->err_integr < MIN_I_ERROR) state->err_integr = MIN_I_ERROR;
	Iterm = (state->Ki_n * state->err_integr )/state->Ki_d;

	// Derivative Term
	////////////////////////////////////////////////////////////////////////////////

	Dterm = (state->Kv_n * (err-state->err_prev))/state->Kv_d;
	state->position_prev = state->position;
	state->speed_prev = filtered_speed;

	// TODO: this is a hack
	PIDterm = Pterm + Iterm + Dterm;// + state->pid_term_prev;



	// TODO: convert PIDterm to 2600
	if (PIDterm > 0){

		if (PIDterm > MAX_PWM) {
			PIDterm = MAX_PWM;
			state->pwm_out = MAX_PWM;
		} else {
			state->pwm_out = PIDterm;
		}

		state->drive_code = DRIVE_CODE_FORWARD;

	}else{

		if (PIDterm < -MAX_PWM) {
			PIDterm = -MAX_PWM;
			state->pwm_out = MAX_PWM;
		} else {
			state->pwm_out = -PIDterm;
		}

		state->drive_code = DRIVE_CODE_REVERSE;
	}

	state->pid_term_prev = PIDterm;


	state->err_prev = err;

	if (state->mode > 0 && state->status == 0) state->pwm_out = 0;

}

struct wq_wrapper
{
	struct work_struct worker;
	struct sock *sk;
	unsigned char internal;
};

struct StateResponse
{
	unsigned char hasState : 1;
	struct State state;
};

struct wq_wrapper wq_data;

struct wq_wrapper wq_pid_data;

// static DECLARE_COMPLETION( threadcomplete );
struct workqueue_struct *wq;

void cb_data(struct sock *sk, int bytes)
{
	if(KOVAN_KMOD_DEBUG) printk("Message Received\n");

	wq_data.sk = sk;
	wq_data.internal = 0;
	queue_work(wq, &wq_data.worker);
}

struct StateResponse state(void)
{
	struct State ret;

	read_vals(ret.t, NUM_FPGA_REGS); // TODO: poor naming is poor

	struct StateResponse response;
	response.hasState = 1;
	response.state = ret;
	//printk("State called\n");
	return response;
}

void pid_timer_callback( unsigned long data )
{
	// printk( "pid_timer_callback called.\n");
	mod_timer(&pid_timer, jiffies + msecs_to_jiffies(40));

	// set up anything we need to set up in the pid packet
	for (unsigned int i = 0; i < 4; i++) {
		pid_states[i].position = ((int)kovan_regs[BEMF_0_HIGH+i] << 16) | kovan_regs[BEMF_0_LOW + i];

		pid_states[i].desired_position = ((int)kovan_regs[GOAL_POS_0_HIGH + i] << 16) | kovan_regs[GOAL_POS_0_LOW + i];
		pid_states[i].desired_speed = (((int)kovan_regs[GOAL_SPEED_0_HIGH + i] << 16) | kovan_regs[GOAL_SPEED_0_LOW + i]);

		pid_states[i].mode = (kovan_regs[PID_MODES] >> ((3 - i) << 1)) & 0x3;

		pid_states[i].Kp_n = kovan_regs[PID_PN_0 + i];
		if (kovan_regs[PID_PD_0+i] != 0) pid_states[i].Kp_d = kovan_regs[PID_PD_0 + i];
		pid_states[i].Ki_n = kovan_regs[PID_IN_0 + i];
		if (kovan_regs[PID_ID_0+i] != 0) pid_states[i].Ki_d = kovan_regs[PID_ID_0 + i];
		pid_states[i].Kv_n = kovan_regs[PID_DN_0 + i];
		if (kovan_regs[PID_DD_0+i] != 0) pid_states[i].Kv_d = kovan_regs[PID_DD_0 + i];
	}


	if (kovan_regs[PID_MODES] == 0){
		kovan_regs[PID_STATUS] = 0;
		return;
	}

	// add to the work queue
 	wq_pid_data.sk = 0;
	wq_pid_data.internal = 1;
	queue_work(wq, &wq_pid_data.worker);

}


struct StateResponse do_packet(unsigned char *data, const unsigned int size)
{
	int have_state_request = 0;

	int num_write_commands = 0;
	unsigned short write_addresses[WRITE_COMMAND_BUFF_SIZE];
	unsigned short write_values[WRITE_COMMAND_BUFF_SIZE];

	struct StateResponse response;
	memset(&response, 0, sizeof(struct StateResponse));
	
	if(size < sizeof(struct Packet)) {
		if(KOVAN_KMOD_WARN) printk("Packet was too small!! Not processing.\n");
		return response; // Error: Packet is too small?
	}
	
	struct Packet *packet = (struct Packet *)data;
	
	if(KOVAN_KMOD_DEBUG) printk("Received %u commands\n", packet->num);
	
	for(unsigned short i = 0; i < packet->num; ++i) {
		struct Command cmd = packet->commands[i];
		
		switch(cmd.type) {
		case StateCommandType:
			have_state_request = 1;
			break;
		
		case WriteCommandType:
		{
			struct WriteCommand *w_cmd = (struct WriteCommand *)&(cmd.data);

			// handle registers that don't go to the fpga
			if (w_cmd->addy > NUM_FPGA_REGS && w_cmd->addy < TOTAL_REGS) {
				kovan_regs[w_cmd->addy] = w_cmd->val;
				if(KOVAN_KMOD_DEBUG)printk("Writing Kovan Reg[%d] = %d\n", w_cmd->addy, w_cmd->val);
				continue;
			}

			if (num_write_commands < WRITE_COMMAND_BUFF_SIZE) {
				write_addresses[num_write_commands] = 	w_cmd->addy;
				if (w_cmd->addy >= MOTOR_PWM_0 && w_cmd->addy <= MOTOR_PWM_3){
					// correct PWM vals
					// TODO: move to FPGA
					const unsigned int percent = w_cmd->val / 26; // from 0 <--> 100
					write_values[num_write_commands] = pwm_outs[percent];
				}else{
					write_values[num_write_commands] = 	w_cmd->val;
				}
				if(KOVAN_KMOD_DEBUG) printk("r[%d]<=%d\n", w_cmd->addy, w_cmd->val);
				if (write_addresses[num_write_commands] < NUM_FPGA_REGS){
					num_write_commands += 1;
				}// otherwise ignore this out of range request
			}// no more room for write commands
		}
			break;
		
		default: break;
		}
	}
	
	if (num_write_commands > 0) {
		write_vals(write_addresses, write_values, num_write_commands);
	}

	if (have_state_request) {
		response = state();

		for (unsigned int i = NUM_FPGA_REGS; i < TOTAL_REGS; i++){
			response.state.t[i] = kovan_regs[i];
		}
	}

	for (unsigned int i = 0; i < NUM_FPGA_REGS; i++){
		kovan_regs[i] = response.state.t[i];
	}

	return response;
}

#define UDP_HEADER_SIZE 8

void do_work(struct work_struct *data)
{
	if(KOVAN_KMOD_DEBUG) printk("do work:\n");

	struct wq_wrapper *foo = container_of(data, struct wq_wrapper, worker);


	if(KOVAN_KMOD_DEBUG) printk("wq_wrapper foo = %p\n", foo);
	
	// TODO: this is so incredibly hacky
	if (foo->internal) {
		if(KOVAN_KMOD_DEBUG) printk("do_work internal call\n");

		unsigned char pwm_packet[sizeof(struct Packet) + 5 * sizeof(struct Command)];
		struct Packet *janky_pwm_packet = (struct Packet *)&pwm_packet;

		janky_pwm_packet->num = 6;

		for (unsigned int i = 0; i < 4; i++) step_pid(&pid_states[i]);

		// pwm
		struct WriteCommand *cmd = 0;
		for (unsigned int i = 0; i < 4; i++){
			cmd = (struct WriteCommand *) janky_pwm_packet->commands[i].data;
			if (pid_states[i].status){
				cmd->val = pid_states[i].pwm_out;
				cmd->addy = MOTOR_PWM_0 + i;
			}else{
				cmd->addy = 0;
			}
			janky_pwm_packet->commands[i].type = WriteCommandType;
		}


		// drive code
		unsigned short drive_code = 0;

		for (unsigned int i = 0; i < 4; i++){
			if (pid_states[i].mode){
				drive_code |= ((pid_states[i].drive_code & 0x3) << (6-(i<<1)));
			}else{
				drive_code |= (kovan_regs[MOTOR_DRIVE_CODE_T] & (0x3 << (6-(i<<1))));
			}
		}

		cmd = (struct WriteCommand *) janky_pwm_packet->commands[4].data;
		cmd->val =  drive_code;
		cmd->addy = MOTOR_DRIVE_CODE_T;
		janky_pwm_packet->commands[4].type = WriteCommandType;

		// state request
		janky_pwm_packet->commands[5].type = StateCommandType;

		do_packet(pwm_packet, sizeof(struct Packet));


		// set done register
		unsigned short pid_status =
				((pid_states[0].status & 0x1) << 3) |
				((pid_states[1].status & 0x1) << 2) |
				((pid_states[2].status & 0x1) << 1) |
				((pid_states[3].status & 0x1));

		kovan_regs[PID_STATUS] = pid_status;

		return;
	}

	int len = 0;
	while((len = skb_queue_len(&foo->sk->sk_receive_queue)) > 0) {
		struct sk_buff *skb = skb_dequeue(&foo->sk->sk_receive_queue);
		if(KOVAN_KMOD_DEBUG) printk("Message : %i Message: %s\n", skb->len - UDP_HEADER_SIZE, skb->data + UDP_HEADER_SIZE);
		struct StateResponse response = do_packet(skb->data + UDP_HEADER_SIZE, skb->len - UDP_HEADER_SIZE);
		
		if(!response.hasState) {
			if(KOVAN_KMOD_WARN)printk("No State to send back. Continuing...\n");
			continue;
		}
		if(KOVAN_KMOD_DEBUG) printk("Sending State Back...\n");
		
		struct sockaddr_in to;
		memset(&to, 0, sizeof(to));
		to.sin_family = AF_INET;
		struct iphdr *ipinf = (struct iphdr *)skb_network_header(skb);
		to.sin_addr.s_addr = ipinf->saddr;
		if(KOVAN_KMOD_DEBUG) printk(" @_SRC: %d.%d.%d.%d\n",NIPQUAD(ipinf->saddr));
		to.sin_port = *((unsigned short *)skb->data);
		
		struct msghdr msg;
		memset(&msg, 0, sizeof(msg));
		msg.msg_name = &to;
		msg.msg_namelen = sizeof(to);
		
		struct iovec iov;
		iov.iov_base = &response.state;
		iov.iov_len  = sizeof(struct State);
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		
		/* adjust memory boundaries */
 		mm_segment_t oldfs = get_fs();
		set_fs(KERNEL_DS);
		//printk("sock_sendmsg\n");
		sock_sendmsg(clientsocket, &msg, iov.iov_len);
		
		set_fs(oldfs);
		kfree_skb(skb);
		//printk("Sent State Response\n");
	}
}

static int __init server_init(void)
{
	printk("Initing Kovan Module\n");

	init_spi();

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
	INIT_WORK(&wq_pid_data.worker, do_work);
	wq = create_singlethread_workqueue("myworkqueue");
	if(!wq) {
		return -ENOMEM;
	}

	if(sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &clientsocket) < 0) {
		printk(KERN_ERR "server: Error creating clientsocket.\n");
		return -EIO;
	}
	
	printk("Initializing pid state structs\n");
	for (int i = 0; i < 4; ++i) {
		init_pid_state(&pid_states[i]);
	}

	memset(kovan_regs, 0, TOTAL_REGS*sizeof(kovan_regs[0]));

	// set up pid coeff registers
	for (unsigned int i = 0; i < 4; i++) {
		kovan_regs[PID_PN_0 + i] = pid_states[i].Kp_n;
		kovan_regs[PID_PD_0 + i] = pid_states[i].Kp_d;
		kovan_regs[PID_IN_0 + i] = pid_states[i].Ki_n;
		kovan_regs[PID_ID_0 + i] = pid_states[i].Ki_d;
		kovan_regs[PID_DN_0 + i] = pid_states[i].Kv_n;
		kovan_regs[PID_DD_0 + i] = pid_states[i].Kv_d;
	}






	printk("Setting up pid timer\n");
	setup_timer(&pid_timer, pid_timer_callback, 0);

	 printk("Starting pid timer\n");
	 mod_timer( &pid_timer, jiffies + msecs_to_jiffies(200) );

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
	
	del_timer(&pid_timer);

	printk("Exiting Kovan Module\n");
}

module_init(server_init);
module_exit(server_exit);
MODULE_LICENSE("GPL");
