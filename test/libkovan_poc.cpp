#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <iostream>
#include <unistd.h>
#include "../protocol.h"
#include "../kovan-regs.h"


typedef std::vector<Command> CommandVector;

class KovanModule
{
public:
	KovanModule(const uint64_t& moduleAddress, const uint16_t& modulePort);
	~KovanModule();
	
	bool init();
	bool bind(const uint64_t& address, const uint16_t& port);
	void close();
	
	uint64_t moduleAddress() const;
	uint16_t modulePort() const;
	
	bool send(const Command& command);
	bool send(const CommandVector& commands);
	
	bool recv(State& state);
	
	Command createWriteCommand(unsigned short address, unsigned short value);

	void turnMotorsOn(unsigned short speed = 0xFFFF);

	void turnMotorsOff();

	int getState(State &state);

	void displayState(const State state);



private:
	int m_sock;
	sockaddr_in m_out;
	
	static Packet *createPacket(const uint16_t& num, uint32_t& packet_size);
};

KovanModule::KovanModule(const uint64_t& moduleAddress, const uint16_t& modulePort)
	: m_sock(-1)
{
	memset(&m_out, 0, sizeof(m_out));
	m_out.sin_family = AF_INET;
	m_out.sin_addr.s_addr = moduleAddress;
	m_out.sin_port = modulePort;
}

KovanModule::~KovanModule()
{
	close();
}

bool KovanModule::init()
{
	// Socket was already inited
	if(m_sock >= 0) return true;
	
	m_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(m_sock < 0) {
		perror("socket");
		return false;
	}
	
	return true;
}

bool KovanModule::bind(const uint64_t& address, const uint16_t& port)
{
	if(m_sock < 0) return false;
	
	sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET; 
	sa.sin_addr.s_addr = address;
	sa.sin_port = port;

	if(::bind(m_sock, (sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("bind");
		return false;
	}
	
	return true;
}

void KovanModule::close()
{
	if(m_sock < 0) return;
	
	::close(m_sock);
}

uint64_t KovanModule::moduleAddress() const
{
	return m_out.sin_addr.s_addr;
}

uint16_t KovanModule::modulePort() const
{
	return m_out.sin_port;
}

bool KovanModule::send(const Command& command)
{
	return send(CommandVector(1, command));
}

bool KovanModule::send(const CommandVector& commands)
{
	uint32_t packetSize = 0;
	Packet *packet = createPacket(commands.size(), packetSize);
	memcpy(packet->commands, &commands[0], commands.size() * sizeof(Command));
	
	bool ret = true;
	if(sendto(m_sock, packet, packetSize, 0, (sockaddr *)&m_out, sizeof(m_out)) != packetSize) {
		perror("sendto");
		ret = false;
	}
	
	free(packet);
	return ret;
}

bool KovanModule::recv(State& state)
{
	memset(&state, 0, sizeof(State));
	if(recvfrom(m_sock, &state, sizeof(State), 0, NULL, NULL) != sizeof(State)){
		perror("recvfrom");
		return false;
	}
	return true;
}


Packet *KovanModule::createPacket(const uint16_t& num, uint32_t& packet_size)
{
	packet_size = sizeof(Packet) + sizeof(Command) * (num - 1);
	Packet *packet = reinterpret_cast<Packet *>(malloc(packet_size));
	packet->num = num;
	return packet;
}


Command KovanModule::createWriteCommand(unsigned short address, unsigned short value)
{
		WriteCommand wc;
		wc.addy = address;
		wc.val = value;
		Command c0;
		c0.type = WriteCommandType;
		memcpy(c0.data, &wc, sizeof(WriteCommand));

		return c0;
}


// test to turn all 4 motors on
//
// pwm_div is probably constant
//
// pwm_val depends on the speed  (shared for all motors currently)
// 0xFF seems to be relatively quick
//
// drive_code specifies  forward/reverse/idle/brake
// Forward = 10, Reverse = 01, Brake = 11, Idle = 00
void KovanModule::turnMotorsOn(unsigned short speed)
{

	// this seems to be the pwm div bunnie uses (10kHz?)
	Command c0 = createWriteCommand(MOTOR_PWM_PERIOD_T, 3);

	// relatively fast
	Command c1 = createWriteCommand(MOTOR_PWM_T, speed);

	// drive code (all forward)
	Command c2 = createWriteCommand(MOTOR_DRIVE_CODE_T, 0xAA);

	CommandVector commands;
	commands.push_back(c0);
	commands.push_back(c1);
	commands.push_back(c2);

	send(commands);
}

void KovanModule::turnMotorsOff()
{

	// this seems to be the pwm div bunnie uses (10kHz?)
	Command c0 = createWriteCommand(MOTOR_PWM_PERIOD_T, 3);

	// relatively fast
	Command c1 = createWriteCommand(MOTOR_PWM_T, 0);

	// drive code (F,R,F,R)
	Command c2 = createWriteCommand(MOTOR_DRIVE_CODE_T, 0);

	CommandVector commands;
	commands.push_back(c0);
	commands.push_back(c1);
	commands.push_back(c2);

	send(commands);

}

int KovanModule::getState(State &state)
{

	Command c0;
	c0.type = StateCommandType;

	CommandVector commands;
	commands.push_back(c0);

	send(commands);


	if(!recv(state)) {
		std::cout << "Error: didn't get state back!" << std::endl;
		return -1;
	}

	return 0;
}


void KovanModule::displayState(const State state)
{

	int i;
	std::cout << "State: " << std::endl;
	for (i = 0; i < NUM_FPGA_REGS; i+=4){
		std::cout << "\t"
				<< state.t[i]   << ", "
				<< state.t[i+1] << ", "
				<< state.t[i+2] << ", "
				<< state.t[i+3] << std::endl;
	}
}


int main(int argc, char *argv[])
{
	KovanModule kovan(inet_addr("127.0.0.1"), htons(5555));
	
	// Create the socket descriptor for communication
	if(!kovan.init()) {
		return 1;
	}
	
	// Bind out client to an address
	if(!kovan.bind(htonl(INADDR_ANY), htons(9999))) {
		return 1;
	}
	
	// Turn motors on for some time
	kovan.turnMotorsOn();
	sleep(3);

	// Turn the motors off
	kovan.turnMotorsOff();
	sleep(1);

	// Update and display the state
	State state;
	kovan.getState(state);
	kovan.displayState(state);

	return 0;
}
