#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_COMMAND_DATA_SIZE 16
#define NUM_FPGA_REGS 64

enum CommandType
{
	NilType = 0,
	StateCommandType,
	MotorCommandType,
	DigitalCommandType
};

struct Command
{
	enum CommandType type;
	unsigned char data[MAX_COMMAND_DATA_SIZE];
};

struct Packet
{
	unsigned short num;
	struct Command commands[1];
};

struct MotorCommand
{
	unsigned char port; // 0 - 3
	unsigned char power; // 0 - 255
};

struct DigitalCommand
{
	unsigned char port; // 0 - 7
	unsigned char on : 1;
};

struct State
{
	unsigned short t[NUM_FPGA_REGS];
};

#ifdef __cplusplus
}
#endif

#endif
