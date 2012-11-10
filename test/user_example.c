/* Based on http://people.ee.ethz.ch/~arkeller/linux/kernel_user_space_howto.html#s3 */

#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include "../protocol.h"

#define BUFFSIZE 5096 

struct Packet *create_packet(const unsigned short num, unsigned int *packet_size)
{
	*packet_size = sizeof(struct Packet) + (sizeof(struct Command) * (num - 1));
	struct Packet *packet = (struct Packet *)malloc(*packet_size);
	packet->num = num;
	return packet;
}

int main(int argc, char *argv[]) {
	
	int sock = 0;
	if((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("socket");
		return -1;
	}

	/* my address */
	struct sockaddr_in receivesocket;
	memset(&receivesocket, 0, sizeof(receivesocket));  
	receivesocket.sin_family = AF_INET; 
	receivesocket.sin_addr.s_addr = htonl(INADDR_ANY);
	receivesocket.sin_port = htons(9999);

	if(bind(sock, (struct sockaddr *) &receivesocket, sizeof(receivesocket)) < 0) {
		perror("bind");
		return -1;
	}

	/* kernel address */
	struct sockaddr_in sendsocket;
	memset(&sendsocket, 0, sizeof(sendsocket));
	sendsocket.sin_family = AF_INET;
	sendsocket.sin_addr.s_addr = inet_addr("127.0.0.1");
	sendsocket.sin_port = htons(5555);

	/* Send message to the server */
	struct MotorCommand test;
	
	struct Command command;
	command.type = MotorCommandType;
	memcpy(command.data, &test, sizeof(struct MotorCommand));
	
	unsigned int packet_size = 0;
	struct Packet *packet = create_packet(1, &packet_size);
	memcpy(packet->commands, &command, sizeof(struct Command));
	
	if(sendto(sock, packet, packet_size, 0, (struct sockaddr *)&sendsocket, sizeof(sendsocket)) != packet_size) {
		perror("sendto");
		return -1;
	}
	
	free(packet);
	
	return 0;
}
