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
  char *ip = NULL;

  while (1) {
    static struct option options[] = {
        {"ip",      required_argument, 0, 0},
        {"port",    required_argument, 0, 0},
        {"bufsize", required_argument, 0, 0},
        {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;
    switch (c) {
      case 0:
        switch (option_index) {
          case 0: ip = optarg; break;
          case 1: port = atoi(optarg); break;
          case 2: bufsize = atoi(optarg); break;
        }
        break;
      case '?':
        fprintf(stderr, "Unknown option\n");
        return 1;
    }
  }

  if (!ip || port <= 0 || port > 65535 || bufsize <= 0) {
    fprintf(stderr, "Usage: %s --ip ADDR [--port N] [--bufsize N]\n", argv[0]);
    return 1;
  }

  int sockfd, n;
  char *sendline = malloc(bufsize);
  char *recvline = malloc(bufsize + 1);
  struct sockaddr_in servaddr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    return 1;
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons((uint16_t)port);

  if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
    perror("inet_pton problem");
    return 1;
  }

  write(1, "Enter string\n", 13);

  while ((n = read(0, sendline, bufsize)) > 0) {
    if (sendto(sockfd, sendline, n, 0, (SADDR *)&servaddr, SLEN) == -1) {
      perror("sendto problem");
      return 1;
    }

    memset(recvline, 0, bufsize + 1);
    int r = recvfrom(sockfd, recvline, bufsize, 0, NULL, NULL);
    if (r == -1) {
      perror("recvfrom problem");
      return 1;
    }
    recvline[r] = 0;
    printf("REPLY FROM SERVER= %s\n", recvline);
  }

  close(sockfd);
  free(sendline);
  free(recvline);
  return 0;
}