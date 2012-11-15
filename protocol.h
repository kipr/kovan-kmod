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
	WriteCommandType
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

struct WriteCommand
{
	unsigned short addy; // 0 - 40
	unsigned short val; // 0 - 0xFFFF
};

struct State
{
	unsigned short t[NUM_FPGA_REGS];
};

#ifdef __cplusplus
}
#endif

#endif
