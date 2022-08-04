#ifndef _SOCK_H
#define _SOCK_H

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <iostream>
#include <netinet/tcp.h>
#include <sys/signal.h>

#include "error.h"

char IP[15];
int PORT;

class Sock
{
public:
  static int Socket()
  {
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd < 0) {
      my_error(std::string("socket"), __FILE__, __LINE__);
    }
    return socketfd;
  }

  static void Bind(char *ip, int sockfd, int port)
  {
    struct sockaddr_in this_addr;
    memset(&this_addr, 0, sizeof(this_addr));
    this_addr.sin_family = AF_INET;
    this_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &this_addr.sin_addr);

    if(bind(sockfd, (struct sockaddr*)&this_addr, sizeof(this_addr)) < 0) {
      my_error(std::string("bind"), __FILE__, __LINE__);
    }    
  }

  static void Listen(int sockfd, int limits) 
  {
    if(listen(sockfd, limits) < 0) {
      my_error(std::string("listen"), __FILE__, __LINE__);
    }
  }

  static void Accept(int sockfd) 
  {
    if(accept(sockfd, NULL, NULL) < 0) {
      my_error(std::string("accept"), __FILE__, __LINE__);
    }
  }

  static void Connect(int sockfd, char *ip, int port)
  {
    struct sockaddr_in this_addr;
    memset(&this_addr, 0 ,sizeof(this_addr));
    this_addr.sin_family = AF_INET;
    this_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &this_addr.sin_addr);
    
    if(connect(sockfd, (struct sockaddr*)&this_addr, sizeof(this_addr)) < 0) {
      my_error(std::string("connect"), __FILE__, __LINE__);
    }

  }

  static void Setsock(int sockfd)
  {
    int keepalive = 1;   // 开启TCP KeepAlive功能
    int keepidle = 27; // tcp_keepalive_time
    int keepcnt = 3;     // tcp_keepalive_probes
    int keepintvl = 3;  // tcp_keepalive_intvl
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, NULL, 0);
    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
    setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, (void *)&keepidle, sizeof(keepidle));
    setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, (void *)&keepcnt, sizeof(keepcnt));
    setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepintvl, sizeof(keepintvl));
  }
};

#endif