#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void sigchld_handler(int s) {
  int saved_errno = errno;

  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  errno = saved_errno;
}

void setupSocket() {

  int serverFd, newFd, status, sockFd, yes = 1;
  socklen_t sin_size;
  struct addrinfo hints, newConn;
  struct addrinfo *servInfo, *p;
  struct sigaction sa;
  char *error;
  char buf[1024];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  status = getaddrinfo(NULL, "4090", &hints, &servInfo);
  if (status != 0) {
    fprintf(stderr, "getaddrinfo error : %s\n", gai_strerror(status));
    exit(1);
  }

  for (p = servInfo; p != NULL; p = p->ai_next) {
    sockFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockFd == -1) {
      perror("server : socket");
      continue;
    }

    if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int) == -1)) {
      perror("setsockopt");
    }

    status = bind(sockFd, p->ai_addr, p->ai_addrlen);
    if (status == -1) {
      close(sockFd);
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(servInfo);

  if (p == NULL) {
    fprintf(stderr, "server : failed to bind\n");
    exit(1);
  }

  if (listen(sockFd, 10) == -1) {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  printf("server : waiting for connectins...\n");

  while (1) {
    sin_size = sizeof(newConn);
    newFd = accept(sockFd, (struct sockaddr *)&newConn, &sin_size);
    printf("New connection descriptor : %d\n", newFd);

    if (newFd == -1) {
      perror("accept");
      continue;
    }

    uint32_t value;
    if (!fork()) {
      printf("Got a new connection!\n");
      int recvResult = recv(newFd, buf, sizeof(buf), 0);
      for (int i = 0; i < sizeof(buf); i++){
        printf("%c ", value);
      }
    } else {
      close(sockFd);
    }
  }
}
