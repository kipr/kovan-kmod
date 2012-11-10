#include <stdint.h>

//
// Structs
//

#define MAX_COMMAND_DATA_SIZE 256

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

//
// Two sample command functions
//

void motorCommand(struct MotorCommand *cmd)
{
	// Motor Command
}

void digitalCommand(struct DigitalCommand *cmd)
{
	// Digital Command
}

//
// The delegate function
//

uint8_t readPacket(uint8_t *data, const uint32_t size)
{
	if(size < sizeof(Packet)) {
		return FALSE; // Error: Packet is too small?
	}
	
	struct Packet *packet = (struct Packet *)data;
	
	for(uint16_t i = 0; i < packet->num; ++i) {
		struct Command cmd = packet->commands[i];
		
		switch(cmd.type) {
		case MotorCommandType:
			motorCommand((struct MotorCommand *)cmd.data);
			break;
		
		case DigitalCommandType:
			digitalCommand((struct DigitalCommand *)cmd.data);
			break;
		
		default: break;
		}
	}
	
	return TRUE;
}

