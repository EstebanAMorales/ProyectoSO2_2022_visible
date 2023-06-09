//
// Created by andresteve07fedesk on 31/03/2022.
//

#ifndef SO2_2022_TP1_SERVER_MULTITHREAD_H
#define SO2_2022_TP1_SERVER_MULTITHREAD_H
#define FILE_SIZE 20971520
//#define TCP4_SERVER_PORT 7799
//#define TCP6_SERVER_PORT 7077
#define BUFFER_SIZE 1024
#define SOCKET_ERROR (-1)
#define TCP4_CONNECTIONS_QUEUE_SIZE 10
#define TCP6_CONNECTIONS_QUEUE_SIZE 10
#define UNIX_CONNECTIONS_QUEUE_SIZE 10
//#define UNIX_SOCKET_PATH "./so2_2022_tp1.socket"
#define MEGA 1000000L

typedef enum{
    SRV_OK = 0,
    SRV_SETUP_SQL_ERR = -1,
    socket_unknown = -2,
}server_result;

typedef struct thread_args {
    int client_socket_fd;
    int connection_index;
}thread_args;

void* execute_sql_query_from_tcp4_client(void* client_socket);
void* execute_sql_query_from_tcp6_client(void* client_socket);
void* send_backup_file_to_unix_client(void* params);
void* measure_band_width_usage(void* non);
int check(int exp, const char *msg);
void init_bandwidth_monitor();

void setup_log_file();

void setup_exit_signal_catcher();

void setup_tcp4_socket();

server_result setup_sqlite();

void* tcp4_handle_new_connections(void* non);

void* tcp6_handle_new_connections(void* non);

void* unix_handle_new_connections(void* non);

int setup_tcp6_socket();

int setup_unix_socket();

void init_connections_listeners();
int set_socket_timeouts(int client_fd, int seconds);

static void sig_handler(int dummy);

void process_exec_arguments(int arguments_count, const char **arguments);
void print_exec_help();

int get_connection_index();
void init_mutexes();
void destroy_mutexes();

void vacuum_backup_database(int connection_index);
void close_sqlite_connections();
#endif //SO2_2022_TP1_SERVER_MULTITHREAD_H
