#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/epoll.h>

#define BUFFER_LENGTH 1024
#define CONNECTION_SIZE 1024

typedef int (*RCALLBACK)(int fd);

int recv_cb(int);
int send_cb(int);
int accept_cb(int);

int epfd = 0;

struct conn{
    int fd;
    char rbuffer[BUFFER_LENGTH];
    char wbuffer[BUFFER_LENGTH];
    int rlength;
    int wlength;
    
    RCALLBACK send_callback;
    int is_sockfd;
    union {
        RCALLBACK recv_callback;
        RCALLBACK accept_callback;
    }r_action;
};

struct conn conn_list[CONNECTION_SIZE] = {0};

int set_event(int fd, int event, int status){
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    
    if(status == 1){
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    }else{
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    }
}
int recv_cb(int fd){
    
    int count = recv(fd, conn_list[fd].rbuffer, BUFFER_LENGTH,0);
    
    if(count == 0) {
        printf("client disconnect: %d\n", fd);
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        return 0;
    }
    conn_list[fd].rlength = count;
    printf("RECV: %s\n", conn_list[fd].rbuffer);

# if 1 //echo
    conn_list[fd].wlength = conn_list[fd].rlength;
    memcpy(conn_list[fd].wbuffer, conn_list[fd].rbuffer, count);
#endif
    set_event(fd, EPOLLOUT, 0);
    return count;
}

int send_cb(int fd){
    int count = send(fd, conn_list[fd].wbuffer, BUFFER_LENGTH, 0);
    set_event(fd, EPOLLIN, 0);
    
}

int accept_cb(int fd){
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    printf("accept-------\n");   
    int clientfd = accept(fd, (struct sockaddr*)&clientaddr, &len);
    printf("accept finished: %d\n",clientfd);

    conn_list[clientfd].fd = clientfd;
    conn_list[clientfd].is_sockfd = 0;
    conn_list[clientfd].r_action.recv_callback = recv_cb;
    conn_list[clientfd].send_callback = send_cb;
    memset(conn_list[clientfd].rbuffer, 0, BUFFER_LENGTH);
    memset(conn_list[clientfd].wbuffer, 0, BUFFER_LENGTH);
    conn_list->rlength = 0;
    conn_list->wlength = 0;

    set_event(clientfd, EPOLLIN, 1);

    return clientfd;
}



int init_server(unsigned short port){

    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if( -1 == bind(sockfd, (struct sockaddr*) &servaddr, sizeof(struct sockaddr))){
        printf("Bind Failed: %s\n", strerror(errno));
    }
    listen(sockfd, 10);
    printf("listen finished: %d\n", sockfd);
    
    return sockfd;
}


int main()
{
    unsigned short port = 2000;
    int sockfd = init_server(port);
    epfd = epoll_create(1);
    if(epfd < 0){
        printf("epoll create failed: %d\n", epfd);
    }
    conn_list[sockfd].fd = sockfd;
    conn_list[sockfd].r_action.accept_callback = accept_cb;
    conn_list[sockfd].is_sockfd = 1;
    set_event(sockfd, EPOLLIN, 1);

    int timeouts = -1;//-1表示没有事件就绪时epoll无限阻塞，后面添加定时器和定时器事件
    while(1) {

        struct epoll_event events[1024] = {0};

        int nready = epoll_wait(epfd, events, 1024, timeouts);  
        for(int i = 0; i < nready; i++){
            int connfd = events[i].data.fd;
            //网络IO事件
            if(events[i].events & EPOLLIN) {
                if(conn_list[connfd].is_sockfd){
                    conn_list[connfd].r_action.accept_callback(connfd);
                }else{
                    conn_list[connfd].r_action.recv_callback(connfd);
                }
            }
            if(events[i].events & EPOLLOUT) {
                conn_list[connfd].send_callback(connfd);
            }

            //定时器事件
        }
    }
    
}