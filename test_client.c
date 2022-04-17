/*
	C ECHO client example using sockets
*/
#include <stdio.h>	//printf
#include <string.h>	//strlen
#include <sys/socket.h>	//socket
#include <arpa/inet.h>	//inet_addr
#include <unistd.h>
#include "io_ops_common.h"

int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char message[1000] , server_reply[2000];

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( 8888 );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

    puts("Connected\n");
    int bytes_sent, bytes_read;
    //keep communicating with server
    while(1)
    {
        printf("Enter message : ");
        scanf("%s" , message);

        //Send some data
        bytes_sent = send_data(message,strlen(message),sock);
        if( bytes_sent < 0)
        {
            puts("Send failed");
            return 1;
        }

        //Receive a reply from the server
        char* echo_resp;
        bytes_read = recv_data(&echo_resp,sock);
        if( bytes_read < 0)
        {
            puts("recv failed");
            break;
        }

        puts("Server reply :");
        puts(echo_resp);
    }

    close(sock);
    return 0;
}