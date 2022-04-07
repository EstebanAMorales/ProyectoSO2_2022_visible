//
// Created by andresteve07fedesk on 01/04/2022.
//

#ifndef SO2_2022_TP1_CLIENTS_COMMON_H
#define SO2_2022_TP1_CLIENTS_COMMON_H

#define FILE_CHUNK_BUF_SIZE 1024
#define DEFAULT_LAG_VALUE_MS 500L

extern volatile int keep_client_running;
extern long lag_between_sends_ms;

static void client_sig_handler(int dummy);
int set_socket_timeouts(int socket_fd, int seconds);
int send_file_over_socket(char* file_name, int socket_fd);
void setup_termination_signal_catcher();
void process_exec_arguments(int arguments_count, const char **arguments);
void print_exec_help();
void process_lag_argument(const char **arguments);
#endif //SO2_2022_TP1_CLIENTS_COMMON_H
