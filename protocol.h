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
	uint8_t data[MAX_COMMAND_DATA_SIZE];
};

struct Packet
{
	uint16_t num;
	struct Command *commands;
};

struct MotorCommand
{
	uint8_t port; // 0 - 3
	uint8_t power; // 0 - 255
};

struct DigitalCommand
{
	uint8_t port; // 0 - 7
	uint8_t on : 1;
};

#ifdef __cplusplus
}
#endif

#endif
