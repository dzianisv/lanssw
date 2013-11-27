#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <sys/select.h>

#include "common.h"

#define BUFSIZE 1024
#define RECV_STATUS_CYCLES 1024

int common_readed=0;
int sockfd;
int signal_handler ( int code );
int tcp_loop ( int sockfd );
int udp_loop ( int sockfd );

void help()
{
        fprintf ( stderr, "use: server -a <listen_ip> -p <port> [-t] [-u]\n" );
}


/* parse options */
static char opt_addr[17];
static char opt_port[6];
static int opt_via_tcp = 1;

void parse_options ( int argc, char** argv )
{
        if ( argc != 3 ) {
                help();
                exit ( EXIT_FAILURE );
        }

        int opt;
        while ( opt = getopt ( argc, argv, "aputh:" ) ) {
                switch ( opt ) {
                case 'a':
                        strcpy ( opt_addr, optarg );
                        break;

                case 'p':
                        strcpy ( opt_port, optarg );
                        break;

                case 'u':
                        opt_via_tcp = 0;
                        break;

                case 't':
                        opt_via_tcp = 1;
                        break;

                case 'h':
                        help();
                        exit ( EXIT_SUCCESS );
                        break;

                case '?':
                        help();
                        exit ( EXIT_FAILURE );
                        break;

                default:
                        help();
                        exit ( EXIT_FAILURE );
                        break;
                }
        }
}

int main ( int argc, char** argv )
{
        signal ( SIGURG, signal_handler );
        signal ( SIGPIPE, signal_handler );

        parse_options ( argc, argv );

        sockfd  = ( opt_via_tcp ) ?  tcpv4_bind ( opt_addr, opt_port ) : udpv4_bind ( opt_addr, opt_port ) ;

        if ( sockfd == -1 )  {
                perror ( "Can't init socket" );
                return 2;
        }

        int status;
        if ( opt_via_tcp ) {
                status = tcp_loop();
        } else {
                status = udp_loop();
        }

        return status;
}

int udp_loop ( int sockfd )
{
	fd_set fdsetp;
	FD_SETSIZE(&fdsetp);
	FD_SET(sockfd, &fdsetp);
	
	while( select(&fdsetp, NULL, NULL, NULL, 0) != -1)
	{
		if(FD_ISSET(&fdsetp) {
		}
	}
        return 0;
}

int tcp_loop ( int sockfd )
{
        if ( opt_via_tcp ) {
                if ( listen ( sockfd, 1 ) ) {
                        perror ( "can't start listening" );
                        exit ( EXIT_FAILURE );
                }
        }

        int client_sockfd;
        printf ( "Start listening at %s:%s\n", opt_addr, opt_port );

        char buffer[BUFSIZE];
        struct sockaddr_in client_addr;
        int size = sizeof ( client_addr );

        while ( ( client_sockfd = accept ( sockfd, &client_addr, &size ) ) != -1 ) {
                fcntl ( client_sockfd, F_SETOWN, getpid() );

                int opt=1;
                printf ( "Connection opened with %s\n", inet_ntoa ( client_addr.sin_addr ) );
                uint32_t flen, status;

                status  = recv ( client_sockfd, &flen, sizeof ( flen ), MSG_WAITALL );
                if ( status == -1 ) {
                        perror ( "Can't receive file name size" );
                        return 5;
                }

                char fname[flen+1];
                memset ( fname, 0x0, sizeof ( fname ) );
                status = recv ( client_sockfd, fname, flen, MSG_WAITALL );
                if ( status==-1 ) {
                        perror ( "Can't receive file name" );
                        continue;
                }

                int fd = open ( fname, O_WRONLY| O_APPEND | O_CREAT, 0755 );
                //send file open status
                status = errno;
                send ( client_sockfd, &status, sizeof ( status ), 0 );

                //send file offset
                {
                        uint32_t offset = file_size ( fd );
                        send ( client_sockfd, &offset, sizeof ( offset ), 0x0 );
                        printf ( "File name: %s, file size %u bytes\n", fname, offset );
                }

                //receiving data size
                uint32_t data_size, display_status;
                recv ( client_sockfd, &data_size, sizeof ( data_size ), 0x0 );

                while ( common_readed != data_size ) {
                        status = recv ( client_sockfd, buffer, sizeof ( buffer ), 0x0 );
                        common_readed +=  status;

                        if ( status == -1 ||  status == 0 )  {
                                perror ( "\nClient disconected" );
                                break;
                        } else if ( write ( fd, buffer,  status ) == -1 )  {
                                perror ( "\nCan't write to file" );
                                break;
                        }
                }

                if ( common_readed == data_size ) {
                        status=0;
                        send ( client_sockfd, &status, sizeof ( status ), 0x0 );
                        printf ( "\nAll data received\n" );
                } else {
                        fprintf ( stderr, "\nNot all data received from client\n" );
                }
                close ( fd );
                close ( client_sockfd );
                printf ( "\nConnection closed\n\n" );
        }

        perror ( "Socket accept error" );
        exit ( EXIT_FAILURE );
}

int signal_handler ( int code )
{
        printf ( "signal_handler code=%d\n", code );
        uint8_t buf;
        switch ( code ) {
        case SIGPIPE:
                fprintf ( stderr, "Pipe broken" );
                break;
        case SIGURG:
                recv ( sockfd, &buf, sizeof ( buf ), MSG_OOB );
                printf ( "Urgent data received\n" );
                printf ( "%10.2lf KB received\n",  common_readed/1024.0 );
                break;
        }
}