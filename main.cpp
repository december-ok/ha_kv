#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 6379
#define BUF_SIZE 65536

int main(void)
{
    int server_fd, client_fd;
    int pipefd[2];
    struct sockaddr_in addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(server_fd,
             (struct sockaddr *)&addr,
             sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 128) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("listening on %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        if (pipe(pipefd) == -1) {
            perror("pipe");
            close(client_fd);
            continue;
        }

        while (1) {
            ssize_t n = splice(
                client_fd,
                NULL,
                pipefd[1],
                NULL,
                BUF_SIZE,
                SPLICE_F_MOVE
            );

            if (n == 0) {
                // client closed connection
                break;
            }

            if (n == -1) {
                if (errno == EINTR)
                    continue;

                perror("splice socket -> pipe");
                break;
            }

            ssize_t remain = n;

            // pipe -> TCP socket
            while (remain > 0) {
                ssize_t m = splice(
                    pipefd[0],
                    NULL,
                    client_fd,
                    NULL,
                    remain,
                    SPLICE_F_MOVE
                );

                if (m == -1) {
                    if (errno == EINTR)
                        continue;

                    perror("splice pipe -> socket");
                    remain = 0;
                    break;
                }

                remain -= m;
            }
        }

        close(pipefd[0]);
        close(pipefd[1]);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}