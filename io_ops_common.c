//
// Created by andresteve07fedesk on 07/04/2022.
//

#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include<sys/socket.h>
#include "io_ops_common.h"


void set_payload_size(unsigned long payload_size,char* whole_buffer){
    //LSB first
    //printf("payload_size: %lu \n", payload_size);
    whole_buffer[0] = (unsigned char) (payload_size & 0xFF);
    whole_buffer[1] = (unsigned char) ((payload_size>>8) & 0xFF);
    whole_buffer[2] = (unsigned char) ((payload_size>>16) & 0xFF);
    whole_buffer[3] = (unsigned char) ((payload_size>>24) & 0xFF);
    //printf("bytes:%i,%i,%i,%i\n",whole_buffer[0],whole_buffer[1],whole_buffer[2],whole_buffer[3]);
}

/**
 * Send data on a given socket in a single write of a maximum of IO_BUFFER_SIZE-4 bytes.
 * @param data_buffer
 * @param socket_fd
 * @return
 */
int send_data_single_write(char* data_buffer, int socket_fd){
    unsigned long data_size = strlen(data_buffer);
    if (data_size>IO_BUFFER_SIZE-4){
        printf("Can't send %lu bytes in a single write because input data exceeds BUFFER SIZE.\n",data_size);
        return 1;
    }
    char send_buf[IO_BUFFER_SIZE+1];//+1 for null-termination
    memset(send_buf,0,IO_BUFFER_SIZE);
    set_payload_size(data_size, send_buf);

    char* actual_payload = &send_buf[4];
    strcpy(actual_payload, data_buffer);

    printf("TOTAL MESG: 0x%X,0x%X,0x%X,0x%X::%s\n", send_buf[0], send_buf[1], send_buf[2], send_buf[3], &send_buf[4]);
    unsigned long single_write_res = write(socket_fd, send_buf, strlen(actual_payload) + 4);
    if( single_write_res > 0){
        return 0;
    } else {
        printf("Error on single write. RESULT: %lu, ERROR: %s\n",single_write_res, strerror(errno));
        return 1;
    }
}
unsigned  long get_amount_left_to_process(long total_data_size, long bytes_already_processed){
    unsigned  long left_to_process;
    left_to_process = total_data_size - (bytes_already_processed - 4);
    if (left_to_process < IO_BUFFER_SIZE){
        return left_to_process;
    } else {
        return IO_BUFFER_SIZE;
    }
}
/**
 *
 * @param data_buffer
 * @param socket_fd
 * @return
 */
long send_data(char* data_buffer, int socket_fd){
    unsigned long total_data_size = strlen(data_buffer);
    if (total_data_size == 0){
        return -1;
    }
    char send_buf[IO_BUFFER_SIZE+1];//+1 for string null-termination
    memset(send_buf,0,strlen(send_buf));//strlen can be used because send_buf is an array
    long total_bytes_sent=0;
    int bytes_written = 0;

    set_payload_size(total_data_size, send_buf);

    char* first_write_payload = &send_buf[4];

    unsigned long first_write_payload_size = 0;
    if (total_data_size < IO_BUFFER_SIZE-4){
        first_write_payload_size = total_data_size;
    } else {
        first_write_payload_size = IO_BUFFER_SIZE - 4;
    }
    strncpy(first_write_payload, data_buffer, first_write_payload_size);//WARNING actual_paylaod may be not NULL-terminated.
    //printf("send --%s--\n",data_buffer);
    /*
    printf("FIRST WRITE: 0x%X,0x%X,0x%X,0x%X::%.*s\n", //WARNING %.*s
           send_buf[0], send_buf[1], send_buf[2], send_buf[3], first_write_payload_size, &send_buf[4]);
    */
    //send header + payload
    bytes_written = write(socket_fd, send_buf, first_write_payload_size+4);
    total_bytes_sent+=bytes_written;
    if(bytes_written > 0){
        //printf("First write return:%i\n",bytes_written);
    } else {
        printf("send_data ERROR. First write failed. Error: %s\n",strerror(errno));
        return -1;
    }

    while (total_bytes_sent < total_data_size + 4){//+4 because we need to send the 4byte header
        unsigned  long bytes_to_write;
        bytes_to_write = get_amount_left_to_process(total_data_size,total_bytes_sent);
        bytes_written = write(socket_fd, data_buffer+(total_bytes_sent-4), bytes_to_write);
        if (bytes_written<=0){
            printf("send_data ERROR. Failure on writing loop, RESULT:%i ERROR: %s\n", bytes_written,strerror(errno));
            return 1;
        }
        total_bytes_sent+=bytes_written;

    }
    //printf("END OF WRITE\n");
    return total_bytes_sent;
}

/**
 * @brief Gets the payload size from the first 4 bytes of the input buffer.
 *
 * @param whole_buffer
 * @return int
 */
int get_payload_size(char* whole_buffer){
    int payload_size = 0;
    payload_size = (int)((unsigned char)whole_buffer[3]);
    payload_size = (payload_size<<8) + (unsigned char) whole_buffer[2];
    payload_size = (payload_size<<8) + (unsigned char) whole_buffer[1];
    payload_size = (payload_size<<8) + (unsigned char) whole_buffer[0];
    //printf("payload_size:%i\n",payload_size);
    return payload_size;

}
/**
 * Receives input data on a given socket, the function will read a maximum of IO_BUFFER_SIZE bytes on a single read
 * and will store the obtained data on the char buffer passed as argument by reference.
 * @param char_buffer
 * @param socket_fd
 * @return
 */
int recv_data_single_read(char** char_buffer, int socket_fd){
    //size_t payload_size;
    char input_buf[IO_BUFFER_SIZE];//sizeof only works ok for static arrays i.e. results on 500
    bzero(input_buf, sizeof(input_buf));
    int bytes_recv = 0;
    bytes_recv = read(socket_fd, input_buf, sizeof(input_buf));
    printf("BYTES READ: %i\n", bytes_recv);
    printf("INPUT BUF: 0x%X,0x%X,0x%X,0x%X. EXTRA 0x%X.\n",input_buf[0],input_buf[1],input_buf[2],input_buf[3],input_buf[4]);
    if(bytes_recv > 0){
        int payload_size = get_payload_size(input_buf);
        *char_buffer = malloc(payload_size * sizeof(char ));
        char* actual_payload = &input_buf[4];
        actual_payload[payload_size]='\0';
        printf("size_cadena:%zu,cadena: %s\n", strlen(actual_payload), actual_payload);
        strcpy(*char_buffer, actual_payload);
        return 0;
    } else {
        printf("FAILED TO RECEIVE RPC MSG! Errno: %s\n", strerror(errno));
        return 1;
    }
}

long recv_data(char** data_buffer, int socket_fd){
    int total_payload_size;
    long total_read_bytes_count=0;
    char input_buf[IO_BUFFER_SIZE+1];//sizeof only works ok for static arrays. +1 for null-termination
    memset(input_buf,0,strlen(input_buf));

    int bytes_recv = 0;
    //read first chunk of data the first 4 bytes contain the total size of the data to be received.
    bytes_recv = recv(socket_fd, input_buf, IO_BUFFER_SIZE,0);
    //printf("BYTES READ: %i\n", bytes_recv);
    //printf("INPUT BUF: 0x%X,0x%X,0x%X,0x%X. EXTRA 0x%X.\n",input_buf[0],input_buf[1],input_buf[2],input_buf[3],input_buf[4]);
    if(bytes_recv > 4){//makes sure that we have received at least the 4 bytes SIZE HEADER
        total_payload_size = get_payload_size(input_buf);
        total_read_bytes_count=bytes_recv;
        //return 0;
    } else {
        printf("recv_data: First read failed. read returned:%i Errno: %s\n",bytes_recv, strerror(errno));
        return -1;
    }

    *data_buffer = (char*)malloc((total_payload_size * sizeof(char)) + 1);//+1 is necessary to hold NULL char '\0' end of string
    memset(*data_buffer, 0, total_payload_size+1);
    char* first_read_tail = &input_buf[4];

    //printf("tail:%s--size: %lu\n",first_read_tail, strlen(first_read_tail));
    if (total_payload_size<(IO_BUFFER_SIZE-4)){
        strncpy(*data_buffer, first_read_tail,total_payload_size);
    } else {
        strcpy(*data_buffer, first_read_tail);
    }

    //printf("HOLO3\n");

    while (total_read_bytes_count < total_payload_size + 4){//+4 because size header bytes
        long bytes_to_read;
        bytes_to_read=get_amount_left_to_process(total_payload_size,total_read_bytes_count);
        bytes_recv = read(socket_fd, input_buf, bytes_to_read);
        if (bytes_recv <= 0){
            printf("recv_data ERROR read loop failed RESULT: %i, ERROR:  %s\n", bytes_recv, strerror(errno));
            return -1;
        }
        strcat(*data_buffer, input_buf);//WARNING may produce NOT null-terminated string
        total_read_bytes_count+=bytes_recv;
        memset(input_buf,0,IO_BUFFER_SIZE+1);

    }
    //printf("READ size:%lu data: --%s--\n",strlen(*data_buffer),*data_buffer);
    return total_read_bytes_count;
}

int set_socket_timeouts(int socket_fd, int seconds){
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;

    if (setsockopt (socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
        printf("setsockopt failed\n");
        perror("sockopt recv: ");
        return 1;
    }

    if (setsockopt (socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
        printf("setsockopt failed\n");
        perror("sockopt send: ");
        return 2;
    }
    return 0;
}