#include <getopt.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr

int main(int argc, char **argv) {
  int port = 10050;
  int bufsize = 100;

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

  const size_t kSize = sizeof(struct sockaddr_in);
  int lfd, cfd, nread;
  char *buf = malloc(bufsize);
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return 1;
  }

  int opt = 1;
  setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  memset(&servaddr, 0, kSize);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons((uint16_t)port);

  if (bind(lfd, (SADDR *)&servaddr, kSize) < 0) {
    perror("bind");
    return 1;
  }
  if (listen(lfd, 5) < 0) {
    perror("listen");
    return 1;
  }

  printf("TCP SERVER on port %d, bufsize %d\n", port, bufsize);
  fflush(stdout);

  while (1) {
    unsigned int clilen = kSize;

    if ((cfd = accept(lfd, (SADDR *)&cliaddr, &clilen)) < 0) {
      perror("accept");
      return 1;
    }
    printf("connection established\n");
    fflush(stdout);

    while ((nread = read(cfd, buf, bufsize)) > 0) {
      write(1, buf, nread);
    }

    if (nread == -1) {
      perror("read");
      return 1;
    }
    close(cfd);
  }

  free(buf);
  close(lfd);
  return 0;
}