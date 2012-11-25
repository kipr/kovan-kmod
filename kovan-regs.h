/*
 * kovan-regs.h
 *
 *  Created on: Nov 16, 2012
 *      Author: josh
 */

#ifndef KOVAN_REGS_H_
#define KOVAN_REGS_H_


// Permanent Read Only
#define DIG_IN 					1
#define AN_IN_0 				2	// Not currently supported
#define AN_IN_1 				3	// Not currently supported
#define AN_IN_2 				4	// Not currently supported
#define AN_IN_3 				5	// Not currently supported
#define AN_IN_4 				6	// Not currently supported
#define AN_IN_5 				7	// Not currently supported
#define AN_IN_6 				8	// Not currently supported
#define AN_IN_7 				9	// Not currently supported
#define AN_IN_8 				10	// Not currently supported
#define AN_IN_9 				11	// Not currently supported
#define AN_IN_10 				12	// Not currently supported
#define AN_IN_11 				13	// Not currently supported
#define AN_IN_12 				14	// Not currently supported
#define AN_IN_13 				15	// Not currently supported
#define AN_IN_14 				16	// Not currently supported
#define AN_IN_15 				17	// Not currently supported


// Permanent Read/Write
#define SERVO_COMMAND_0			25
#define SERVO_COMMAND_1			26
#define SERVO_COMMAND_2			27
#define SERVO_COMMAND_3			28
#define DIG_OUT					29
#define DIG_PULLUPS				30
#define	DIG_OUT_ENABLE			31
#define AN_PULLUPS				32
#define MOTOR_PWM_0				33
#define MOTOR_PWM_1				34
#define MOTOR_PWM_2				35
#define MOTOR_PWM_3				36


// Temporary Ready Only
#define DIG_VALID_T 			19
#define DIG_BUSY_T 				20


// Temporary Read/Write
#define DIG_SAMPLE_T			37
#define	DIG_UPDATE_T			38
#define MOTOR_DRIVE_CODE_T		39
#define MOTOR_ALL_STOP_T		40


#endif /* KOVAN_REGS_H_ */
