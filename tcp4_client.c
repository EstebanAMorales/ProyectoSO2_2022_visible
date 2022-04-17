#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "clients_common.h"
#include "io_ops_common.h"

#define SA struct sockaddr
#define LOOPING_QUERY "SELECT * FROM Cars"
//#define PORT 7799
//#define INPUT_FILE_PATH "../fake180mbfile.fake"

int tcp4_client_sock_fd;
struct sockaddr_in tcp4_serv_addr;
int port;
char* backup_file_path;

int tcp4_init_client();
int tcp4_connect_to_server(char*  server_ip);

int main(int argc, const char **argv){
    setup_termination_signal_catcher();
    process_exec_arguments(argc, argv);
    tcp4_init_client();
    tcp4_connect_to_server("");
    while (keep_client_running){
        char* query_response;
        usleep(lag_between_sends_ms * 1000L);
        //send_file_over_socket(backup_file_path, tcp4_client_sock_fd);
        send_data(LOOPING_QUERY,strlen(LOOPING_QUERY),tcp4_client_sock_fd);
        recv_data(&query_response,tcp4_client_sock_fd);
        printf("QUERY RESULT:\n %s\n",query_response);

    }
    printf("Closed by User.\n");
    close(tcp4_client_sock_fd);
    return 0;
}


void process_exec_arguments(int arguments_count, const char **arguments) {
    if (arguments_count < 4){
        print_exec_help();
        exit(1);
    }
    //There are either 3 or more arguments
    const char* port_string_arg = arguments[1];
    printf("ARG2:%s",port_string_arg);
    port = atoi(port_string_arg);
    printf("PORT: %i\n",port);

    const char* file_path_string_arg = arguments[2];
    backup_file_path = malloc(strlen(file_path_string_arg) * sizeof(char ));
    printf("ARG2:%s\n",file_path_string_arg);
    strcpy(backup_file_path, file_path_string_arg);
    printf("FILE_PATH: %s\n", backup_file_path);

    process_lag_argument(arguments);

}

void print_exec_help() {
    printf("Usage: tcp4_client <PORT> <FILE_PATH> <LAG_MS>\n");
}

int tcp4_connect_to_server(char*  server_ip){
    bzero(&tcp4_serv_addr, sizeof(tcp4_serv_addr));
    // assign IP, PORT
    tcp4_serv_addr.sin_family = AF_INET;
    tcp4_serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    tcp4_serv_addr.sin_port = htons(port);

    // connect the client socket to server socket
    if (connect(tcp4_client_sock_fd, (SA*)&tcp4_serv_addr, sizeof(tcp4_serv_addr)) != 0) {
        printf("connection with the server failed...\n");
        return 1;
    } else {
        printf("connected to the server..\n");
        return 0;
    }
}
int tcp4_init_client(){
    // socket create and verification
    tcp4_client_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp4_client_sock_fd == -1) {
        printf("socket creation failed...\n");
        return 1;
    }
    else {
        printf("Socket successfully created..\n");
        return set_socket_timeouts(tcp4_client_sock_fd, 10);
        //return 0;
    }
}
