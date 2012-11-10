/* Based on http://people.ee.ethz.ch/~arkeller/linux/kernel_user_space_howto.html#s3 */

#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include "../protocol.h"

#define BUFFSIZE 5096 

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
	char buffer[BUFFSIZE];
	memcpy(buffer, "Hello World", strlen("Hello World") + 1);
	int sendlen = strlen(buffer) + 1;
	
	if(sendto(sock, buffer, sendlen, 0, (struct sockaddr *) &sendsocket, sizeof(sendsocket)) != sendlen) {
		perror("sendto");
		return -1;
	}

	memset(buffer, 0, BUFFSIZE);
	int received = 0;
	if((received = recvfrom(sock, buffer, BUFFSIZE, 0, NULL, NULL)) < 0){
		perror("recvfrom");
		return -1;
	}
	buffer[BUFFSIZE - 1] = 0;
	
	printf("Message Received: %s\n", buffer);
	
	return 0;
}
