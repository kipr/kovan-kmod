#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>
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
	
	const uint64_t& moduleAddress() const;
	const uint16_t& modulePort() const;
	
	bool send(const Command& command);
	bool send(const CommandVector& commands);
	
private:
	int m_sock;
	sockaddr_in m_out;
	
	static Packet *createPacket(const uint16_t& num, uint32_t& packet_size);
};

KovanModule::KovanModule(const uint64_t& moduleAddress, const uint16_t& modulePort)
	: m_sock(-1)
{
	memset(&m_out, 0, sizeof(sendsocket));
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

	if(bind(m_sock, (sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("bind");
		return false;
	}
	
	return true;
}

void KovanModule::close()
{
	if(m_sock < 0) return;
	
	close(m_sock);
}

const uint64_t& KovanModule::moduleAddress() const
{
	return m_out.sin_addr.s_addr;
}

const uint16_t& KovanModule::modulePort() const
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
	
	bool ret = true;
	if(sendto(sock, packet, packetSize, 0, (sockaddr *)&m_out, sizeof(m_out)) != packetSize) {
		perror("sendto");
		ret = false;
	}
	
	free(packet);
	return ret;
}

Packet *KovanModule::createPacket(const uint16_t& num, uint32_t& packet_size)
{
	packet_size = sizeof(Packet) + sizeof(Command) * (num - 1);
	Packet *packet = malloc(packet_size);
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
	
	kovan.send(command);
	
	return 0;
}
