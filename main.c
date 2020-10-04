#include <sys/socket.h>	// sockaddr_in, socket(), connect()
#include <netinet/in.h>	// sockaddr_in
#include <netinet/ip.h>	// sockaddr_in
#include <sys/types.h>	// socket(), connect() (some historical implementations required this)
#include <strings.h>	// bzero()
#include <arpa/inet.h>	// htons(), inet_pton()
#include <unistd.h>		// read()
#include <stdio.h>		// fputs()
#include <stdlib.h>		// exit()

#define MAXLINE 1024

/* headers for error handling */
#include <stdarg.h>	// varying arguments (va_~~~), vsnprintf()
#include <errno.h>	// errno
//#include <stdlib.h> // exit() (already included)
//#include <stdio.h> // vsnprintf(), snprintf(), fflush() (already included)
#include <string.h>	// strlen(), strerror(), strcat()
#include <syslog.h>	// syslog(), LOG_ERR

#define MAXLINE_E 100


/**
 * for error handling 
 */
int daemon_proc;	// set nonzero by daemon_init(), only works when the process is daemonized

static void err_doit(int, int, const char*, va_list);

/* Fatal error related to system call
 * Print message, dump core, and terminate */
void err_sys(const char *fmt, ...){
	
	va_list ap;	// pointer indicating the first of variable arguments
	
	va_start(ap, fmt);	// make ap indicate after the last fixed argument
	// could simply get variable arguments like 'var = va_arg(ap, type)'
	err_doit(1, LOG_ERR, fmt, ap);	// 1/0: print errno or not, LOG_ERR: level-error conditions

	va_end(ap);	// the end of using variable arguments
	exit(1);	// abnormal termination
}

/* Fatal error unrelated to system call
 * Print message and terminate */
void err_quit(const char *fmt, ...){
	// similar to err_sys

	va_list ap;

	va_start(ap, fmt);
	// could simply get variable arguments like 'var = va_arg(ap, type)'
	err_doit(0, LOG_ERR, fmt, ap);

	va_end(ap);
	exit(1);
}

/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */
static void err_doit(int errnoflag, int level, const char* fmt, va_list ap){

	int errno_save, n;
	char buf[MAXLINE_E + 1];

	// errno: integer variable, set by system calls and some library functions on error
	// to use, errno should be saved to prevent from being changed by other calls
	errno_save = errno;

	vsnprintf(buf, MAXLINE_E, fmt, ap);	// vsnprintf() invokes va_arg macro
	// could use vsprintf when vsnprintf doesn't work, but not safe

	n = strlen(buf);
	if (errnoflag)
		snprintf(buf+n, MAXLINE_E-n, ": %s", strerror(errno_save));	// strerror() returns the string describes the errno_save
	strcat(buf, "\n");

	if (daemon_proc){	// only works when the process is daemonized
		syslog(level, "%s", buf);	// format string should be comes to 2nd argument
	} else {
		fflush(stdout);		// in case stdout and stderr are the same
		fputs(buf, stderr);
		fflush(stderr);
	}
	
	return;
}




/**
 * wrapper functions
 * Socket, Connect, Read, Fputs, Inet_pton
 */
int Socket(int domain, int type, int protocol){
	
	int n;
	if ( (n = socket(domain, type, protocol)) < 0 )
		err_sys("socket error");

	return n;	// sockfd integer number on success
}

int Connect(int sockfd, struct sockaddr *addr, socklen_t addrlen){

	int n;
	if ( (n = connect(sockfd, addr, addrlen)) < 0 )
		err_sys("connect error");

	return n;	// 0 on success
}

ssize_t Read(int fd, void* buf, size_t count){

	int n;
	if ( (n = read(fd, buf, count)) < 0 )
		err_sys("read error");

	return n;	// the number of bytes read on success
}

int Fputs(const char* s, FILE* stream){

	int n;
	if ( (n = fputs(s, stream)) == EOF )
		err_sys("fputs error");

	return n;	// nonnegative number on success
}

int Inet_pton(int af, const char* src, void* dst){

	int n;
	// inet_pton() returns 0 or -1 when address or address family is invalid
	if ( (n = inet_pton(af, src, dst)) <= 0 )
		err_quit("inet_pton error for %s", src);

	return n;	// 1 on success
}





int main(int argc, char **argv){

	int sockfd, n;
	char recvline[MAXLINE + 1];	// +1 for set null terminate
	struct sockaddr_in servaddr;
	
	if (argc != 2)
		err_quit("usage: a.out <IPaddress>");

	// AF_INET : communication domian for IPv4 Internet Protocols
	// SOCK_STREAM : communication type for TCP Connections
	// 0 : protocol setting. Only single protocol exists for tcp stream sockets, so can use 0 instead of IPPROTO_TCP.
	sockfd = Socket(AF_INET, SOCK_STREAM, 0);
	
	bzero(&servaddr, sizeof(servaddr));
	
	// all address/port should be manipulated to  network byte order
	// htons() : host byte order to network byte order for unsigned long/unsigned short
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(13);	// 13 is reserved port for daytime protocol
	
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	// sockaddr structure only used for pointer casting
	Connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

	// read() returns -1 on error, the number of bytes on success
	// Read() checks for error by itself. it is double checking
	while ( (n = Read(sockfd, recvline, MAXLINE)) > 0 ){
		recvline[n] = 0;	// set null terminate
		Fputs(recvline, stdout);
	}
	if (n<0)
		err_sys("read error");

	exit(0);	// terminates the program. cloes all open descriptors.
}
