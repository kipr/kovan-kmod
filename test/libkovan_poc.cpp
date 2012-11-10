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
	
	MotorCommand test;
	Command command;
	command.type = MotorCommandType;
	memcpy(command.data, &test, sizeof(MotorCommand));
	
	Command command1;
	command1.type = StateCommandType;
	
	CommandVector commands;
	commands.push_back(command);
	commands.push_back(command1);
	
	kovan.send(commands);
	
	State state;
	if(!kovan.recv(state)) {
		return 1;
	}
	
	std::cout << "state.t0 = " << state.t0 << "; t1 = " << state.t1 << "; t2 = " << state.t2 << std::endl;
	
	return 0;
}
