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

#if 0
    printf("accept...\n");
    int clientfd = accept(sockfd, (struct sockaddr* )&clientaddr, &len);
    printf("accept finished\n");

    char buffer[1024] = {};
    int count = recv(clientfd, buffer, 1024,0);
    printf("REV: %s\n", buffer);

    count = send(clientfd,buffer,1024,0);
    printf("send: %d\n", count);
#elif 0

    while (1)
    {
        printf("accept\n");
        int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
        printf("accept finished\n");

        char buffer[1024] = {};
        int count = recv(clientfd, buffer, 1024, 0);
        printf("REV: %s\n", buffer);

        count = send(clientfd, buffer, 1024, 0);
        printf("send: %d\n", count);
    }
#elif 0
    while (1)
    {
        printf("accept----->\n");
        int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
        printf("accept finished\n");
        pthread_t thid;

        pthread_create(&thid, NULL, client_thread, &clientfd);
    }
#elif 0
    fd_set rfds, rset;

    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);

    int maxfd = sockfd;

    while (1)
    {
        memcpy(&rset, &rfds, sizeof(fd_set));
        int nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &rset))
        { // accept
            printf("accept----->\n");
            int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
            printf("accept finished\n");

            FD_SET(clientfd, &rfds); // 正确添加clientfd

            if (clientfd > maxfd)
                maxfd = clientfd;
        }

        // recv
        int i = 0;
        for (i = sockfd + 1; i <= maxfd; i++)
        {

            if (FD_ISSET(i, &rset))
            {
                char buffer[1024] = {0};
                int count = recv(i, buffer, 1024, 0);
                if (count == 0)
                { // disconnect
                    printf("client disconnect.\n");
                    FD_CLR(i, &rfds); // 从监控集合移除
                    close(i);
                    break;
                }

                printf("REV: %s\n", buffer);

                count = send(i, buffer, 1024, 0);
                printf("send: %d\n", count);
            }
        }
    }
#elif 0
    struct pollfd fds[1024] = {0};
    fds[sockfd].fd = sockfd;
    fds[sockfd].events = POLLIN;
    int maxfd = sockfd;

    while (1)
    {

        int nready = poll(fds, maxfd+1, -1);

        if(fds[sockfd].revents & POLLIN){
            printf("accept----->\n");
            int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
            printf("accept finished\n");

            fds[clientfd].fd = clientfd; // 正确添加clientfd
            fds[clientfd].events = POLLIN;
            if (clientfd > maxfd){
                maxfd = clientfd;
            }            
        }
        // recv

        int i = 0;
        for (i = sockfd + 1; i <= maxfd; i++)
        {

            if (fds[i].revents & POLLIN)
            {
                char buffer[1024] = {0};
                int count = recv(i, buffer, 1024, 0);
                if (count == 0)
                { // disconnect
                    printf("client disconnect.\n");
                    // FD_CLR(i, &rfds); // 从监控集合移除
                    close(i);
                    
                    fds[i].fd = -1;
                    fds[i].events = 0;
                    continue;
                }

                printf("REV: %s\n", buffer);

                count = send(i, buffer, 1024, 0);
                printf("send: %d\n", count);
            }
        }
        
    }
#else
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