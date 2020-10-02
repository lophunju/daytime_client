#include <sys/socket.h>	// sockaddr_in, socket(), connect()
#include <netinet/in.h>	// sockaddr_in
#include <netinet/ip.h>	// sockaddr_in
#include <sys/types.h>	// socket(), connect() (some historical implementations required this)
#include <strings.h>	// bzero()
#include <arpa/inet.h>	// htons(), inet_pton()
#include <unistd.h>		// read()
#include <stdio.h>		// fputs()
#include <stdlib.h>		// exit()


int main(int argc, char **argv){

	int sockfd, n;
	char recvline[MAXLINE + 1];	// +1 for set null terminate
	struct sockaddr_in servaddr;
	
	if (argc != 2)
		;//err_quit

	// AF_INET : communication domian for IPv4 Internet Protocols
	// SOCK_STREAM : communication type for TCP Connections
	// 0 : protocol setting. Only single protocol exists for tcp stream sockets, so can use 0 instead of IPPROTO_TCP.
	// socket() returns -1 on error
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
		;//err_sys

	bzero(&servaddr, sizeof(servaddr));
	
	// all address/port should be manipulated to  network byte order
	// htons() : host byte order to network byte order for unsigned long/unsigned short
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(13);	// 13 is reserved port for daytime protocol
	
	// inet_pton() returns 0 or -1 when address or address family is invalid
	if ( inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0 )	// how about using &servaddr.sin_addr.s_addr ?
		;//err_quit

	// sockaddr structure only used for pointer casting
	// connect() returns 0 on success, -1 on error
	if ( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0 )
		;//err_sys

	// read() returns -1 on error, the number of bytes on success
	while ( (n = read(sockfd, recvline, MAXLINE)) > 0 ){
		recvline[n] = 0;	// set null terminate
		if ( fputs(recvline, stdout) == EOF )
			;//err_sys
	}
	if (n<0)
		;//err_sys

	exit(0);	// terminates the program. cloes all open descriptors.
}
