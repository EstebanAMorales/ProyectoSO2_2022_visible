#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <strings.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/un.h>
#include <sys/select.h>

#include "server_tp2.h"
#include "io_ops_common.h"

int tpc4_server_port;
int tcp6_server_port;
char* unix_socket_path;
char* log_file_path;

static volatile int keep_running = 1;
//mutexes used to write on the shared counters.
pthread_mutex_t tcp4_mutex, tcp6_mutex, unixconn_mutex;
long tcp4_byte_count, tcp6_byte_count, unixconn_byte_count;

//used for creating new threads as detachable.
pthread_attr_t create_detached_attr;

FILE *bw_usage_log_file;
//TCPv4 setup variables
int tcp4_server_socket_fd, addr_size;
struct sockaddr_in tcp4_server_addr_in, tcp4_client_addr_in;
//TCPv6 setup variables
int tcp6_listener_socket_fd = -1;
struct sockaddr_in6 tcp6_client_addr;
socklen_t tcp6_client_addr_len;
char str_addr[INET6_ADDRSTRLEN];
//Connected UNIX setup variables
int unix_listener_socket_fd = 0;
struct sockaddr_un unix_client_addr;

int main(int argc, const char **argv) {

    process_exec_arguments(argc,argv);
    setup_log_file();
    setup_ctrlc_signal_catcher();
    setup_tcp4_socket();
    setup_tcp6_socket();
    setup_unix_socket();

    pthread_mutex_init(&tcp4_mutex,NULL);
    pthread_mutex_init(&tcp6_mutex,NULL);
    pthread_mutex_init(&unixconn_mutex,NULL);

    pthread_attr_init(&create_detached_attr);
    pthread_attr_setdetachstate(&create_detached_attr, PTHREAD_CREATE_DETACHED);

    init_bandwidth_monitor();
    init_connections_listeners();

    while (keep_running){
        sleep(1);
    }

    printf("Exited by user! Bye.\n");
    pthread_mutex_destroy(&tcp6_mutex);
    pthread_mutex_destroy(&tcp4_mutex);
    pthread_mutex_destroy(&unixconn_mutex);
    fclose(bw_usage_log_file);
    return 0;
}

void process_exec_arguments(int arguments_count, const char **arguments) {
    if (arguments_count < 5){
        print_exec_help();
        exit(1);
    }
    /*
     * int tpc4_server_port;
int tcp6_server_port;
char* unix_socket_path;
     */
    //There are either 3 or more arguments
    const char* tcp4_port_string_arg = arguments[1];
    printf("ARG2:%s", tcp4_port_string_arg);
    tpc4_server_port = atoi(tcp4_port_string_arg);
    printf("PORT: %i\n",tpc4_server_port);

    const char* tcp6_port_string_arg = arguments[2];
    printf("ARG3:%s", tcp6_port_string_arg);
    tcp6_server_port = atoi(tcp6_port_string_arg);
    printf("PORT: %i\n",tcp6_server_port);

    const char* socket_path_string_arg = arguments[3];
    unix_socket_path = malloc(strlen(socket_path_string_arg) * sizeof(char ));
    printf("ARG4:%s\n", socket_path_string_arg);
    strcpy(unix_socket_path, socket_path_string_arg);
    printf("SOCKET_PATH: %s\n",unix_socket_path);

    //log_file_path
    const char* log_file_path_string_arg = arguments[4];
    log_file_path = malloc(strlen(log_file_path_string_arg) * sizeof(char ));
    printf("ARG5:%s\n", log_file_path_string_arg);
    strcpy(log_file_path, log_file_path_string_arg);
    printf("FILE_PATH: %s\n",log_file_path);
}

void print_exec_help(){
    printf("Usage: server_multithread <IPv4_PORT> <IPv6_PORT> <SOCKET_FILE_PATH> <LOG_FILE_PATH>\n");
}

static void sig_handler(int dummy){
    keep_running = 0;
}
/*
 * Creates 3 different threads to handle incoming connections for each protocol.
 */
void init_connections_listeners() {
    pthread_t tcp4_listener;
    pthread_create(&tcp4_listener, &create_detached_attr, tcp4_handle_new_connections, NULL);

    pthread_t tcp6_listener;
    pthread_create(&tcp6_listener, &create_detached_attr, tcp6_handle_new_connections, NULL);

    pthread_t unix_listener;
    pthread_create(&unix_listener, &create_detached_attr, unix_handle_new_connections, NULL);
}
/*
 * Handle incoming connections to the UNIX listener.
 * For each new connection creates a new detached thread.
 */
void* unix_handle_new_connections(void* non){
    while (keep_running){
        int unix_client_fd;
        socklen_t unix_client_addr_len = sizeof(unix_client_addr);

        printf("Waiting for UNIX connections...\n");
        unix_client_fd = accept(unix_listener_socket_fd, (struct sockaddr*)&unix_client_addr, &unix_client_addr_len);
        if( unix_client_fd == -1 ){
            printf("Error on accept() call \n");
            perror("Error: ");
            return NULL;
        }
        set_socket_timeouts(unix_client_fd, 1);
        pthread_t unix_conn_handler_thread;
        int* pclient = malloc(sizeof(int));
        *pclient = unix_client_fd;
        pthread_create(&unix_conn_handler_thread, &create_detached_attr, read_file_from_unix_client, pclient);
    }
    return NULL;
}

/*
 * Configures a socket as a UNIX domain socket with connection.
 */
int setup_unix_socket() {

    struct sockaddr_un unix_listener_addr;
    int len = 0;

    unix_listener_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(-1 == unix_listener_socket_fd )
    {
        printf("Error on socket() call \n");
        return 1;
    }

    unix_listener_addr.sun_family = AF_UNIX;
    strcpy(unix_listener_addr.sun_path, unix_socket_path );
    unlink(unix_listener_addr.sun_path);
    len = strlen(unix_listener_addr.sun_path) + sizeof(unix_listener_addr.sun_family);
    if(bind(unix_listener_socket_fd, (struct sockaddr*)&unix_listener_addr, len) != 0)
    {
        printf("Error on binding socket \n");
        return 1;
    }

    if(listen(unix_listener_socket_fd, UNIX_CONNECTIONS_QUEUE_SIZE) != 0 )
    {
        printf("Error on listen call \n");
        return 1;
    }
    return 0;
}

/*
 * Handler function for the threads created when a UNIX connection is accepted.
 * This will read a fixed size file from a client socket fd in chunks of size defined by the user.
 */
void* read_file_from_unix_client(void* client_socket){
    int client_fd = *((int*)client_socket);
    free(client_socket);
    char input_buffer[BUFFER_SIZE];
    long read_bytes = 0L;
    printf("Starting to receive the file contents...\n");
    while(keep_running){
        bzero(input_buffer,BUFFER_SIZE);
        int current_read = recv(client_fd, input_buffer, BUFFER_SIZE,0);
        if( current_read <= 0){
            printf("Failure to read %i bytes on file transfer. Recv result: %i\n",BUFFER_SIZE,current_read);
            perror("Error _recv:");
            break;
        }
        read_bytes += current_read;
        //log_debug("%i bytes SAVED ON FILE.",saved_bytes);
        pthread_mutex_lock(&unixconn_mutex);
        unixconn_byte_count += current_read;
        pthread_mutex_unlock(&unixconn_mutex);
    }
    printf("Total of %ld bytes READ.\n ",read_bytes);
    printf("Successful file transfer.\n");
    close(client_fd);
    return NULL;
}
/*
 * Handle incoming connections to the IPv6 TCP listener.
 * For each new connection creates a new detached thread.
 */
void* tcp6_handle_new_connections(void* non){
    while(keep_running) {
        int tcp6_client_socket_fd=-1;
        printf("Waiting for TCP6 connections...\n");
        /* Do TCP handshake with client */
        tcp6_client_socket_fd = accept(tcp6_listener_socket_fd,
                                (struct sockaddr *) &tcp6_client_addr,
                                &tcp6_client_addr_len);
        if (tcp6_client_socket_fd == -1) {
            perror("accept()");
            close(tcp6_listener_socket_fd);
            return NULL;
        }
        set_socket_timeouts(tcp6_client_socket_fd, 1);
        inet_ntop(AF_INET6, &(tcp6_client_addr.sin6_addr),
                  str_addr, sizeof(str_addr));
        printf("New connection from: %s:%d ...\n",
               str_addr,
               ntohs(tcp6_client_addr.sin6_port));

        pthread_t tcp6_conn_handler_thread;
        int* pclient = malloc(sizeof(int));
        *pclient = tcp6_client_socket_fd;
        pthread_create(&tcp6_conn_handler_thread, &create_detached_attr, read_file_from_tcp6_client, pclient);
    }
    printf("Done TCP6 listening...\n");
    return NULL;
}
/*
 * Configures a socket as a IPv6 TCP socket connection.
 */
int setup_tcp6_socket() {

    struct sockaddr_in6 tcp6_server_addr;

    int ret, flag;

    /* Create socket for listening (client requests) */
    tcp6_listener_socket_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if(tcp6_listener_socket_fd == -1) {
        perror("socket()");
        return EXIT_FAILURE;
    }

    /* Set socket to reuse address */
    flag = 1;
    ret = setsockopt(tcp6_listener_socket_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    if(ret == -1) {
        perror("setsockopt()");
        return EXIT_FAILURE;
    }

    tcp6_server_addr.sin6_family = AF_INET6;
    tcp6_server_addr.sin6_addr = in6addr_any;
    tcp6_server_addr.sin6_port = htons(tcp6_server_port);

    /* Bind address and socket together */
    ret = bind(tcp6_listener_socket_fd, (struct sockaddr*)&tcp6_server_addr, sizeof(tcp6_server_addr));
    if(ret == -1) {
        perror("bind()");
        close(tcp6_listener_socket_fd);
        return EXIT_FAILURE;
    }

    /* Create listening queue (client requests) */
    ret = listen(tcp6_listener_socket_fd, TCP6_CONNECTIONS_QUEUE_SIZE);
    if (ret == -1) {
        perror("listen()");
        close(tcp6_listener_socket_fd);
        return EXIT_FAILURE;
    }
    return 0;

}
int set_socket_timeouts(int client_fd, int seconds){
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;

    if (setsockopt (client_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
        printf("setsockopt failed\n");
        return 1;
    }

    if (setsockopt (client_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
        printf("setsockopt failed\n");
        return 2;
    }
    return 0;
}

/*
 * Handle incoming connections to the IPv4 TCP listener.
 * For each new connection creates a new detached thread.
 */
void* tcp4_handle_new_connections(void *non) {

    while (keep_running){
        printf("Waiting for TCP4 connections...\n");
        int tcp4_client_socket_fd;
        addr_size=sizeof(struct sockaddr_in);
        tcp4_client_socket_fd=accept(tcp4_server_socket_fd, (struct sockaddr*)&tcp4_client_addr_in, (socklen_t*)&addr_size);
        if (tcp4_client_socket_fd == -1 && keep_running == 1){
            printf("Failed to accept connection. Will keep trying ...");
            break;
        }
        if (tcp4_client_socket_fd == -1 && keep_running == 0){
            break;
        }
        printf("Connected\n");
        set_socket_timeouts(tcp4_client_socket_fd, 1);
        pthread_t t;
        int* pclient = malloc(sizeof(int));
        *pclient = tcp4_client_socket_fd;
        pthread_create(&t, &create_detached_attr, read_file_from_tcp4_client, pclient);
    }
    printf("Done TCP4 listening...\n");
    return NULL;
}
/*
 * Configures a socket as a IPv4 TCP socket connection.
 */
void setup_tcp4_socket() {


    check(tcp4_server_socket_fd=socket(AF_INET, SOCK_STREAM, 0),
          "Failed to create socket");
    //init all addresses
    bzero((char *)&tcp4_server_addr_in, sizeof(tcp4_server_addr_in));
    tcp4_server_addr_in.sin_family=AF_INET;
    tcp4_server_addr_in.sin_addr.s_addr=INADDR_ANY;
    tcp4_server_addr_in.sin_port=htons(tpc4_server_port);

    int opt_val = 1;
    setsockopt(tcp4_server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);

    check(bind(tcp4_server_socket_fd, (struct sockaddr*)&tcp4_server_addr_in, sizeof(tcp4_server_addr_in)),
          "Failed when trying to bind socket to address");
    check(listen(tcp4_server_socket_fd, TCP4_CONNECTIONS_QUEUE_SIZE),
          "Failed when trying to listen for connections");
}
/*
 * Configures a handler function that will be executed when the user
 * terminates the program by typing Ctrl+C on the terminal.
 */
void setup_ctrlc_signal_catcher() {
    struct sigaction act;
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);
    sigaction(SIGTSTP, &act, NULL);
}
/*
 * Configures a local file that will be used as a log for the bandwidth usage calculator.
 * If the file already exists then it is overwritten. After creation the file must be closed so it can be reopen
 * on the measuring thread on append mode to ensure continuous reading using tail -F.
 */
void setup_log_file() {
    bw_usage_log_file = fopen(log_file_path,"w+");
    if (bw_usage_log_file==NULL){
        perror("Failed to create log file");
        exit(1);
    }
    fclose(bw_usage_log_file);
}
/*
 * Initiates a thread that will measure the bandwidth used by each protocol.
 */
void init_bandwidth_monitor() {
    pthread_t bw_monitor_thread;
    pthread_create(&bw_monitor_thread, &create_detached_attr, measure_band_width_usage, NULL);
}

int check(int exp, const char *msg){
    if(exp==SOCKET_ERROR){
        perror(msg);
        exit(1);
    }
    return exp;
}
/*
 * Handler function for the threads created when a IPv4 TCP connection is accepted.
 * This will read a fixed size file from a client socket fd in chunks of size defined by the user.
 */
void* read_file_from_tcp4_client(void* client_socket){
    int client_fd = *((int*)client_socket);
    free(client_socket);
    char input_buffer[BUFFER_SIZE];
    long read_bytes = 0L;
    printf("Starting to receive the file contents...\n");
    while(keep_running){
        bzero(input_buffer,BUFFER_SIZE);
        int current_read = recv(client_fd, input_buffer, BUFFER_SIZE,0);
        if( current_read <= 0){
            printf("Failure to read %i bytes on file transfer. Recv result: %i\n",BUFFER_SIZE,current_read);
            perror("Error _recv:");
            break;
        }
        read_bytes += current_read;
        //log_debug("%i bytes SAVED ON FILE.",saved_bytes);
        pthread_mutex_lock(&tcp4_mutex);
        tcp4_byte_count += current_read;
        pthread_mutex_unlock(&tcp4_mutex);
    }
    printf("Total of %ld bytes READ.\n ",read_bytes);
    printf("Successful file transfer.\n");
    close(client_fd);
    return NULL;
}
/*
 * Handler function for the threads created when a IPv6 TCP connection is accepted.
 * This will read a fixed size file from a client socket fd in chunks of size defined by the user.
 */
void* read_file_from_tcp6_client(void* client_socket){
    int client_fd = *((int*)client_socket);
    free(client_socket);
    char input_buffer[BUFFER_SIZE];
    long read_bytes = 0L;
    printf("Starting to receive the file contents...\n");
    while(keep_running){
        bzero(input_buffer,BUFFER_SIZE);
        int current_read = recv(client_fd, input_buffer, BUFFER_SIZE,0);
        if( current_read <= 0){
            printf("Failure to read %i bytes on file transfer. Recv result: %i\n",BUFFER_SIZE,current_read);
            perror("Error _recv:");
            break;
        }
        read_bytes += current_read;
        //log_debug("%i bytes SAVED ON FILE.",saved_bytes);
        pthread_mutex_lock(&tcp6_mutex);
        tcp6_byte_count += current_read;
        pthread_mutex_unlock(&tcp6_mutex);
    }
    printf("Total of %ld bytes READ.\n ",read_bytes);
    printf("Successful file transfer.\n");
    close(client_fd);
    return NULL;
}
/*
 * Handler function for the thread that calculates the bandwidth used by each protocol
 * writes the values obtained on the log file and uses mutexes to synchronize the access to the shared counters.
 */
void* measure_band_width_usage(void* non){
    long execution_seconds = 0L;

    while (keep_running){
        //"bw_usage_log_file.txt"
        bw_usage_log_file = fopen(log_file_path,"a+");
        long tcp4_current, tcp6_current, unixconn_current;
        sleep(1);
        execution_seconds++;

        pthread_mutex_lock(&tcp4_mutex);
        tcp4_current = tcp4_byte_count;
        tcp4_byte_count = 0L;
        pthread_mutex_unlock(&tcp4_mutex);
        if (tcp4_current > 0){
            tcp4_current=tcp4_current/MEGA;
        }
            //printf("%ld MB/s for protocol TCP4.\n",tcp4_current/100000);

        pthread_mutex_lock(&tcp6_mutex);
        tcp6_current = tcp6_byte_count;
        tcp6_byte_count = 0L;
        pthread_mutex_unlock(&tcp6_mutex);
        if (tcp6_current > 0){
            tcp6_current=tcp6_current/MEGA;
        }

        pthread_mutex_lock(&unixconn_mutex);
        unixconn_current = unixconn_byte_count;
        unixconn_byte_count = 0L;
        pthread_mutex_unlock(&unixconn_mutex);
        if (unixconn_current > 0){
            unixconn_current=unixconn_current/MEGA;
        }

        fprintf(bw_usage_log_file,"SEC: %ld | TCP IPv4: %ld MB/s | TCP IPv6: %ld MB/s | UNIX CONNECTED: %ld MB/s \n",
                execution_seconds,
                tcp4_current,
                tcp6_current,
                unixconn_current);

        fclose(bw_usage_log_file);
    }

    return NULL;
}