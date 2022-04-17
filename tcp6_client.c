#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <uv.h>

#include "clients_common.h"
#include "io_ops_common.h"

#define SA struct sockaddr
#define STATEMENT_LIMIT 500
//#define PORT 7799
//#define INPUT_FILE_PATH "../fake180mbfile.fake"

int tcp6_client_sock_fd;
struct sockaddr_in6 tcp6_serv_addr;
int port;
char* backup_file_path;

int tcp6_init_client();
int tcp6_connect_to_server(char*  server_ip);
int prompt();

char stmt_line[STATEMENT_LIMIT];
int main(int argc, const char **argv){
    setup_termination_signal_catcher();
    process_exec_arguments(argc, argv);
    tcp6_init_client();
    tcp6_connect_to_server("");
    while (keep_client_running){
        //usleep(lag_between_sends_ms*1000L);
        //send_file_over_socket(backup_file_path, tcp6_client_sock_fd);
        prompt();
    }
    printf("Closed by User.\n");
    close(tcp6_client_sock_fd);
    return 0;
}

int prompt(){
    memset(stmt_line,0,STATEMENT_LIMIT);
    printf("remote_sqlite> ");
    if (fgets(stmt_line,STATEMENT_LIMIT,stdin)==NULL){
        printf("Input exceeds 500 character limit.\n");
        return -1;
    }
    printf("READ INPUT: %s",stmt_line);
    if (send_data(stmt_line,strlen(stmt_line),tcp6_client_sock_fd)<=0){//WARNING stmt_line should not contain \0
        return -2;
    }
    char* response;
    if (recv_data(&response, tcp6_client_sock_fd)<=0){
        return -3;
    }
    printf("\n%s",response);
    free(response);
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
    printf("Usage: tcp6_client <PORT> <FILE_PATH> <LAG>\n");
}

int tcp6_connect_to_server(char*  server_ip){
    bzero(&tcp6_serv_addr, sizeof(tcp6_serv_addr));
    // assign IP, PORT
    tcp6_serv_addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &tcp6_serv_addr.sin6_addr);
    tcp6_serv_addr.sin6_port = htons(port);

    // connect the client socket to server socket
    if (connect(tcp6_client_sock_fd, (SA*)&tcp6_serv_addr, sizeof(tcp6_serv_addr)) != 0) {
        printf("connection with the server failed...\n");
        perror("tcp6 connect: ");
        return 1;
    } else {
        printf("connected to the server..\n");
        return 0;
    }
}
int tcp6_init_client(){
    // socket create and verification
    tcp6_client_sock_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (tcp6_client_sock_fd == -1) {
        printf("socket creation failed...\n");
        return 1;
    }
    else {
        printf("Socket successfully created..\n");
        return set_socket_timeouts(tcp6_client_sock_fd, 100);
        //return 0;
    }
}
