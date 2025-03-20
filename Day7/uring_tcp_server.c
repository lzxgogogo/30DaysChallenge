#include <sys/socket.h>
#include <liburing.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


#define ENTRIES_LENGTH 1024
#define BUFFER_LENGTH 1024
#define EVENT_ACCEPT 0
#define EVENT_READ 1
#define EVENT_WRITE 2

struct conn_info {
    int fd;
    int event;
};

int init_server(uint16_t port){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if( -1 == bind(sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr))){
        perror("bind failed\n");
        return -1;
    }
    listen(sockfd, 10);
    return sockfd;
}


int set_event_send(struct io_uring * ring, int connfd, void * buf,
    size_t len, int flags){

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    struct conn_info *send_info = malloc(sizeof(struct conn_info));
    if (!send_info) {
        perror("malloc failed");
        return -1;
    }
    send_info->fd = connfd;
    send_info->event = EVENT_WRITE;

    io_uring_prep_send(sqe, connfd, buf, len, flags);
    sqe->user_data = (unsigned long long)send_info;
    return 0;
}


int set_event_recv(struct io_uring * ring, int connfd, void * buf,
    size_t len, int flags){

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    struct conn_info *recv_info = malloc(sizeof(struct conn_info));
    if (!recv_info) {
        perror("malloc failed");
        return -1;
    }
    recv_info->fd = connfd;
    recv_info->event = EVENT_READ;
    io_uring_prep_recv(sqe, connfd, buf, len, flags);
    sqe->user_data = (unsigned long long)recv_info;

    return 0;
}


int set_event_accept(struct io_uring * ring, int sockfd, struct sockaddr *addr,
        socklen_t *addrlen, int flags){

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    struct conn_info *accept_info = malloc(sizeof(struct conn_info));
    if (!accept_info) {
        perror("malloc failed");
        return -1;
    }
    accept_info->fd = sockfd;
    accept_info->event = EVENT_ACCEPT;
    io_uring_prep_accept(sqe, sockfd, addr, addrlen, flags);
    sqe->user_data = (unsigned long long)accept_info;

    printf("accept after..\n");
    return 0;
}




int main(int argc, char* argv[]){
    unsigned short port = 9999;
    int sockfd = init_server(port);
    
    struct io_uring_params params;
    memset(&params, 0, sizeof(params));

    struct io_uring ring;
    io_uring_queue_init_params(ENTRIES_LENGTH, &ring, &params);
    
#if 0
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    accept(sockfd, (struct sockaddr *)&clientaddr, &len);
#else 
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    set_event_accept(&ring, sockfd,(struct sockaddr *) &clientaddr, &len, 0);
#endif
    
    char buffer[BUFFER_LENGTH] = {0};

    while(1){
        
        io_uring_submit(&ring);
        
        struct io_uring_cqe * cqe;
        io_uring_wait_cqe(&ring, &cqe);
        
        struct io_uring_cqe * cqes[128];
        int nready = io_uring_peek_batch_cqe(&ring, cqes, 128);
        int i = 0;
        for(i = 0; i < nready; i++){
            printf("io_uring_peek_batch_cqe\n");

            struct io_uring_cqe * entries = cqes[i];
            struct conn_info *result = (struct conn_info *)entries->user_data;

            if(result->event == EVENT_ACCEPT){
                set_event_accept(&ring, sockfd,(struct sockaddr *)&clientaddr,&len, 0);
                printf("io_uring_peek_batch_cqe EVENT_ACCEPT\n");

                int connfd = entries->res;
                free(result); // Free memory for EVENT_ACCEPT
                set_event_recv(&ring, connfd, buffer, BUFFER_LENGTH, 0);
            }else if(result->event == EVENT_READ){
                int ret = entries->res;
                printf("set_event_recv ret: %d, %s\n", ret, buffer);

                if(ret == 0) {
                    close(result->fd);
                    free(result); // Free memory for EVENT_READ
                } else if(ret > 0){
                    set_event_send(&ring, result->fd, buffer, ret, 0);
                }
            }else if(result->event == EVENT_WRITE){
                int ret = entries->res;
                printf("set_event_send ret: %d, %s\n", ret, buffer);
                set_event_recv(&ring, result->fd, buffer, BUFFER_LENGTH, 0);
            }           
        }
        io_uring_cq_advance(&ring, nready);
    
    }
    
}
