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
#define AN_IN_0 				2	// User ADC 0
#define AN_IN_1 				3	// User ADC 1
#define AN_IN_2 				4	// User ADC 2
#define AN_IN_3 				5	// User ADC 3
#define AN_IN_4 				6	// User ADC 4
#define AN_IN_5 				7	// User ADC 5
#define AN_IN_6 				8	// User ADC 6
#define AN_IN_7 				9	// User ADC 7
#define AN_IN_8 				10	// Motor ADC 0
#define AN_IN_9 				11	// Motor ADC 1
#define AN_IN_10 				12	// Motor ADC 2
#define AN_IN_11 				13	// Motor ADC 3
#define AN_IN_12 				14	// Motor ADC 4
#define AN_IN_13 				15	// Motor ADC 5
#define AN_IN_14 				16	// Motor ADC 6
#define AN_IN_15 				17	// Motor ADC 7
#define AN_IN_16 				18	// Battery ADC



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
