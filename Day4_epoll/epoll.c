#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/epoll.h>
void *client_thread(void *arg)
{
    int clientfd = *(int *)arg;
    while (1)
    {
        char buffer[1024] = {0};
        int count = recv(clientfd, buffer, 1024, 0);
        if (count == 0)
        { // disconnect
            printf("client connect");
            close(clientfd);
            break;
        }

        printf("REV: %s\n", buffer);

        count = send(clientfd, buffer, 1024, 0);
        printf("send: %d\n", count);
    }
}
int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
    servaddr.sin_port = htons(2000);

    if (-1 == bind(sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)))
    {
        printf("bind Failed: %s\n", strerror(errno));
    }
    listen(sockfd, 10);
    printf("listen finished\n");

    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);

    int epfd = epoll_create(1);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,sockfd,&ev);                                                                           
    
    while(1){
        struct epoll_event events[1024] = {0};
        int nready = epoll_wait(epfd, events, 1024, -1);
        
        int i = 0;
        for(i = 0; i < nready; i++){
            int connfd = events[i].data.fd;
            if(connfd == sockfd){
                printf("accept----->\n");
                int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
                printf("accept finished\n");
                
                ev.events = EPOLLIN;
                ev.data.fd = clientfd;
                epoll_ctl(epfd,EPOLL_CTL_ADD,clientfd,&ev);                     

            }else if(events[i].events & EPOLLIN){

                char buffer[1024] = {0};
                int count = recv(connfd, buffer, 1024, 0);

                if (count == 0)
                { // disconnect
                    printf("client disconnect.\n");
                    close(connfd);
                    epoll_ctl(epfd,EPOLL_CTL_DEL,connfd,&ev);
                    continue;
                }

                printf("REV: %s\n", buffer);

                count = send(connfd, buffer, 1024, 0);
                printf("send: %d\n", count);
            }
        }
    }

#endif

    getchar();
    printf("Exit\n");
    return 0;
}