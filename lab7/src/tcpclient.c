#include <arpa/inet.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SIZE sizeof(struct sockaddr_in)

int main(int argc, char **argv) {
  int port = 10050;
  int bufsize = 100;
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

  int fd, nread;
  char *buf = malloc(bufsize);
  struct sockaddr_in servaddr;

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket creating");
    return 1;
  }

  memset(&servaddr, 0, SIZE);
  servaddr.sin_family = AF_INET;

  if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
    perror("bad address");
    return 1;
  }
  servaddr.sin_port = htons((uint16_t)port);

  if (connect(fd, (SADDR *)&servaddr, SIZE) < 0) {
    perror("connect");
    return 1;
  }

  write(1, "Input message to send\n", 22);
  while ((nread = read(0, buf, bufsize)) > 0) {
    if (write(fd, buf, nread) < 0) {
      perror("write");
      return 1;
    }
  }

  close(fd);
  free(buf);
  return 0;
}