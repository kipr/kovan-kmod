/*
 * kovan-regs.h
 *
 *  Created on: Nov 16, 2012
 *      Author: josh
 */

#ifndef KOVAN_REGS_H_
#define KOVAN_REGS_H_

#define DIG_IN 			1
#define AN_IN_0 		2
#define AN_IN_1 		3
#define AN_IN_2 		4
#define AN_IN_3 		5
#define AN_IN_4 		6
#define AN_IN_5 		7
#define AN_IN_6 		8
#define AN_IN_7 		9
#define AN_IN_8 		10
#define AN_IN_9 		11
#define AN_IN_10 		12
#define AN_IN_11 		13
#define AN_IN_12 		14
#define AN_IN_13 		15
#define AN_IN_14 		16
#define AN_IN_15 		17
#define AN_IN_16 		18
#define AC_CONNECTED		19

#define BEMF_0_LOW		20
#define BEMF_1_LOW		21
#define BEMF_2_LOW		22
#define BEMF_3_LOW		23

// Permanent Read/Write
#define SERVO_COMMAND_0 	25
#define SERVO_COMMAND_1 	26
#define SERVO_COMMAND_2 	27
#define SERVO_COMMAND_3 	28
#define DIG_OUT 		29
#define DIG_PULLUPS 		30
#define DIG_OUT_ENABLE 		31
#define AN_PULLUPS 		32
#define MOTOR_PWM_0 		33
#define MOTOR_PWM_1 		34
#define MOTOR_PWM_2 		35
#define MOTOR_PWM_3 		36
#define MOTOR_ALL_STOP 		40

#define BEMF_0_HIGH 		41
#define BEMF_1_HIGH 		42
#define BEMF_2_HIGH 		43
#define BEMF_3_HIGH 		44
#define SIDE_BUTTON 		45
#define MOT_BEMF_CLEAR 		46

#define GOAL_POS_0_LOW 		48
#define GOAL_POS_1_LOW 		49
#define GOAL_POS_2_LOW 		50
#define GOAL_POS_3_LOW 		51
#define GOAL_POS_0_HIGH 	52
#define GOAL_POS_1_HIGH 	53
#define GOAL_POS_2_HIGH 	54
#define GOAL_POS_3_HIGH		55

#define GOAL_SPEED_0_LOW 	56
#define GOAL_SPEED_1_LOW 	57
#define GOAL_SPEED_2_LOW 	58
#define GOAL_SPEED_3_LOW 	59
#define GOAL_SPEED_0_HIGH 	60
#define GOAL_SPEED_1_HIGH 	61
#define GOAL_SPEED_2_HIGH 	62
#define GOAL_SPEED_3_HIGH 	63

#define PID_MODES 		64
#define PID_STATUS 		65

#define PID_PN_0 		66
#define PID_PN_1 		67
#define PID_PN_2 		68
#define PID_PN_3 		69
#define PID_IN_0 		70
#define PID_IN_1 		71
#define PID_IN_2 		72
#define PID_IN_3 		73
#define PID_DN_0 		74
#define PID_DN_1 		75
#define PID_DN_2 		76
#define PID_DN_3 		77
#define PID_PD_0 		78
#define PID_PD_1 		79
#define PID_PD_2 		80
#define PID_PD_3 		81
#define PID_ID_0 		82
#define PID_ID_1 		83
#define PID_ID_2 		84
#define PID_ID_3 		85
#define PID_DD_0 		86
#define PID_DD_1 		87
#define PID_DD_2 		88
#define PID_DD_3 		89

#define MOTOR_DRIVE_CODE_T 	39

// Analog Voltage (3.3V or 5V, etc)
#define VANA			146

#endif /* KOVAN_REGS_H_ */
