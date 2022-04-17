#include <signal.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include "clients_common.h"

volatile int keep_client_running = 1;
long lag_between_sends_ms;

static void client_sig_handler(int dummy){
    keep_client_running = 0;
}
/*
 * Configures a handler function that will be executed when the user
 * terminates the program by typing Ctrl+C on the terminal.
 */
void setup_termination_signal_catcher() {
    struct sigaction act;
    act.sa_handler = client_sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);
    sigaction(SIGTSTP, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

}

int send_file_over_socket(char* file_name, int socket_fd){
    FILE *file_ptr;
    file_ptr=fopen(file_name,"rb");
    //file_ptr=input_file;
    if (file_ptr == NULL){
        printf("File doesnt exist\n");
        perror("fopen: ");
        return 1;
    }
    int file_desc = fileno(file_ptr);
    char file_buffer[FILE_CHUNK_BUF_SIZE];
    long sent_bytes = 0;
    int write_bytes;
    int read_bytes;
    do{
        write_bytes = 0;
        read_bytes = read(file_desc,file_buffer,FILE_CHUNK_BUF_SIZE);
        //read_bytes = load_file_buffer(file_ptr, file_buffer, FILE_CHUNK_BUF_SIZE, file_desc);
        if(read_bytes == -1) {
            printf("Failure to read content from file.\n");
            return 1;
        }
        if(read_bytes>0){
            write_bytes = send(socket_fd, file_buffer, read_bytes,0);
            if( write_bytes <= 0){
                printf("Failure to write %i bytes on file transfer.\nCause: %s\n",read_bytes, strerror(errno));
                if (file_ptr != NULL){
                    fclose(file_ptr);
                }
                return 2;
            }
            sent_bytes += write_bytes;
        }
        bzero(file_buffer,FILE_CHUNK_BUF_SIZE);
    } while (read_bytes > 0);
    //printf("End of file!\n");
    if (file_ptr != NULL){
        fclose(file_ptr);
    }
    //printf("Total of %i bytes sent.\n", sent_bytes);
    return 0;
}

void process_lag_argument(const char **arguments) {
    const char* lag_string_arg = arguments[3];
    printf("ARG3:%s\n",lag_string_arg);
    lag_between_sends_ms = (long)atoi(lag_string_arg);
    if (lag_between_sends_ms < 10L){
        lag_between_sends_ms = DEFAULT_LAG_VALUE_MS;
    }
    printf("LAG IN MS: %ld\n", lag_between_sends_ms);
}