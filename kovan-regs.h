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
#define AN_IN_0 				2
#define AN_IN_1 				3
#define AN_IN_2 				4
#define AN_IN_3 				5
#define AN_IN_4 				6
#define AN_IN_5 				7
#define AN_IN_6 				8
#define AN_IN_7 				9
#define AN_IN_8 				10
#define AN_IN_9 				11
#define AN_IN_10 				12
#define AN_IN_11 				13
#define AN_IN_12 				14
#define AN_IN_13 				15
#define AN_IN_14 				16
#define AN_IN_15 				17


// Permanent Read/Write
#define SERVO_COMMAND_0			25
#define SERVO_COMMAND_1			26
#define SERVO_COMMAND_2			27
#define SERVO_COMMAND_3			28
#define DIG_DIRS				29  // Do we need it???
#define DIG_PULLUPS				30
#define	DIG_OUT_ENABLE			31
#define AN_PULLUPS				32
#define MOTOR_PWM_0				33
#define MOTOR_PWM_1				34
#define MOTOR_PWM_2				35
#define MOTOR_PWM_3				36


// Temporary Ready Only
#define ADC_IN_T 				2
#define ADC_VALID_T 			18
#define DIG_VALID_T 			19
#define DIG_BUSY_T 				20


// Temporary Read/Write
#define MOTOR_PWM_T 			33
#define SERVO_PWM_PERIOD_T		34
#define MOTOR_PWM_PERIOD_T		35
#define DIG_SAMPLE_T			37
#define	DIG_UPDATE_T			38
#define MOTOR_DRIVE_CODE_T		39
#define MOTOR_ALL_STOP_T		40
#define ADC_CHAN_T				41
#define ADC_GO_T				42


#endif /* KOVAN_REGS_H_ */
