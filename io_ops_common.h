//
// Created by andresteve07fedesk on 07/04/2022.
//

#ifndef SO2_2022_TP2_IO_OPS_COMMON_H
#define SO2_2022_TP2_IO_OPS_COMMON_H
#define IO_BUFFER_SIZE 1000

typedef enum{
    IO_OK = 0,
    IO_RECV_ERR = -1,
    IO_SEND_ERR = -2,
}io_result;
int send_data_single_write(char* data_buffer, int socket_fd);
long send_data(char* data_buffer, unsigned long data_size, int socket_fd);
int recv_data_single_read(char** char_buffer, int socket_fd);
long recv_data(char** data_buffer, int socket_fd);
int set_socket_timeouts(int socket_fd, int seconds);
#endif //SO2_2022_TP2_IO_OPS_COMMON_H
