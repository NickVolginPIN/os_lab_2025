#include <arpa/inet.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

int main(int argc, char **argv) {
  int port = 20001;
  int bufsize = 1024;

  while (1) {
    static struct option options[] = {
        {"port",    required_argument, 0, 0},
        {"bufsize", required_argument, 0, 0},
        {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;
    switch (c) {
      case 0:
        switch (option_index) {
          case 0: port = atoi(optarg); break;
          case 1: bufsize = atoi(optarg); break;
        }
        break;
      case '?':
        fprintf(stderr, "Unknown option\n");
        return 1;
    }
  }

  if (port <= 0 || port > 65535 || bufsize <= 0) {
    fprintf(stderr, "Usage: %s [--port N] [--bufsize N]\n", argv[0]);
    return 1;
  }

  int sockfd, n;
  char *mesg = malloc(bufsize + 1);   // +1 под '\0' (исправление оригинала)
  char ipadr[16];
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    return 1;
  }

  memset(&servaddr, 0, SLEN);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons((uint16_t)port);

  if (bind(sockfd, (SADDR *)&servaddr, SLEN) < 0) {
    perror("bind problem");
    return 1;
  }
  printf("UDP SERVER on port %d, bufsize %d\n", port, bufsize);
  fflush(stdout);

  while (1) {
    unsigned int len = SLEN;

    if ((n = recvfrom(sockfd, mesg, bufsize, 0, (SADDR *)&cliaddr, &len)) < 0) {
      perror("recvfrom");
      return 1;
    }
    mesg[n] = 0;

    printf("REQUEST %s      FROM %s : %d\n", mesg,
           inet_ntop(AF_INET, (void *)&cliaddr.sin_addr.s_addr, ipadr, 16),
           ntohs(cliaddr.sin_port));
    fflush(stdout);

    if (sendto(sockfd, mesg, n, 0, (SADDR *)&cliaddr, len) < 0) {
      perror("sendto");
      return 1;
    }
  }

  free(mesg);
  close(sockfd);
  return 0;
}