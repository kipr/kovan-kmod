#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_COMMAND_DATA_SIZE 16

enum CommandType
{
	NilType = 0,
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
	struct Command *commands;
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

#ifdef __cplusplus
}
#endif

#endif
