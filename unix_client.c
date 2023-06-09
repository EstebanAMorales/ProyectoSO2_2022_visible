#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/un.h>
#include <errno.h>

#include "clients_common.h"
#include "io_ops_common.h"

#define SA struct sockaddr
//#define INPUT_FILE_PATH "../fake180mbfile.fake"
//#define UNIX_SOCKET_PATH "./so2_2022_tp1.socket"

int unix_client_sock_fd;
struct sockaddr_un unix_serv_addr;
char* unix_socket_file_path;
char* backup_file_path;

int unix_init_client();
int unix_connect_to_server(char*  server_ip);


long get_database_backup(char *file_path, int client_fd);

int main(int argc, const char **argv){
    setup_termination_signal_catcher();
    process_exec_arguments(argc, argv);
    unix_init_client();
    unix_connect_to_server("");
    /*
    while (keep_client_running){
        usleep(lag_between_sends_ms*1000L);

    }
    */
    if (get_database_backup(backup_file_path,unix_client_sock_fd)<0L){
        printf("Failed to get file from server\n");
    }
    printf("Closed by User.\n");
    close(unix_client_sock_fd);
    return 0;
}

long get_database_backup(char *file_path, int client_fd) {
    FILE* file_ptr = fopen(file_path,"wb+");
    if (file_ptr==NULL){
        return -1;
    }
    char* entire_file_container;
    long total_read_size = recv_data(&entire_file_container,client_fd);
    printf("TOTAL DATA RECV: %i\n",total_read_size);
    if (total_read_size<0){
        fclose(file_ptr);
        free(entire_file_container);
        return -1;
    }
    if (fwrite(entire_file_container,1,total_read_size,file_ptr)==0){
        printf("Couldnt write on file. Error:%s",strerror(errno));
    }
    free(entire_file_container);
    fclose(file_ptr);
    return 0;
}


void process_exec_arguments(int arguments_count, const char **arguments) {
    if (arguments_count < 4){
        print_exec_help();
        exit(1);
    }
    //There are either 3 or more arguments

    const char* unix_path_string_arg = arguments[1];
    printf("ARG2:%s",unix_path_string_arg);
    unix_socket_file_path = malloc(strlen(unix_path_string_arg)*sizeof(char ));
    strcpy(unix_socket_file_path, unix_path_string_arg);
    printf("UNIX PATH: %s\n",unix_socket_file_path);

    const char* file_path_string_arg = arguments[2];
    backup_file_path = malloc(strlen(file_path_string_arg) * sizeof(char ));
    printf("ARG2:%s\n",file_path_string_arg);
    strcpy(backup_file_path, file_path_string_arg);
    printf("FILE_PATH: %s\n", backup_file_path);

    process_lag_argument(arguments);
}

void print_exec_help() {
    printf("Usage: unix_client <PORT> <FILE_PATH> <LAG MS>\n");
}

int unix_connect_to_server(char*  server_ip){
    bzero(&unix_serv_addr, sizeof(unix_serv_addr));
    // assign IP, PORT
    unix_serv_addr.sun_family = AF_UNIX;
    strcpy(unix_serv_addr.sun_path, unix_socket_file_path);

    // connect the client socket to server socket
    if (connect(unix_client_sock_fd, (SA*)&unix_serv_addr, sizeof(unix_serv_addr)) != 0) {
        printf("connection with the server failed...\n");
        return 1;
    } else {
        printf("connected to the server..\n");
        return 0;
    }
}
int unix_init_client(){
    // socket create and verification
    unix_client_sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (unix_client_sock_fd == -1) {
        printf("socket creation failed...\n");
        return 1;
    }
    else {
        printf("Socket successfully created..\n");
        return set_socket_timeouts(unix_client_sock_fd, 10);
        //return 0;
    }
}