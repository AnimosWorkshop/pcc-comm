#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>

#define MB (1024 * 1024)
#define PCC_SET (32)
#define PCC_END (126)


u_int32_t pcc_total[PCC_END - PCC_SET + 1] = { 0 };
bool pending_sig = false, handling = false;

void handle_sigint(int sig) {
    pending_sig = true;
}

int main(int argc, char **argv) {
    struct sockaddr_in serv_addr, my_addr, peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in);
    int listen_fd, conn_fd;
    u_int32_t fsize, written, written_cur, in_buff, pcc_count;
    u_int32_t pcc_cur[PCC_END - PCC_SET + 1] = { 0 };
    char buff[MB];
    int i, reuse = 1;

    if (argc != 2) {
        fprintf(stderr, "Argument count should be 2, entered %d.\n", argc);
        return -EINVAL;
    }

    signal(SIGINT, handle_sigint); 

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) {
        fprintf(stderr, "Setting socket to reuse failed. %s\n", strerror(errno));
        return -errno;
    }
    
    memset(&serv_addr, 0, addrsize);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    
    if (bind(listen_fd, (struct sockaddr *)&serv_addr, addrsize)) {
        fprintf(stderr, "Bind failed. %s\n", strerror(errno));
        return -errno;
    }

    if (listen(listen_fd, 10)) {
        fprintf(stderr, "Listen failed. %s\n", strerror(errno));
        return -errno;
    }

    while (1) {
        conn_fd = accept(listen_fd, (struct sockaddr *)&peer_addr, &addrsize);
        if (conn_fd < 0) {
            fprintf(stderr, "Accept failed. %s\n", strerror(errno));
            return -errno;
        }

        handling = true;

        getsockname(conn_fd, (struct sockaddr *)&my_addr, &addrsize);
        getpeername(conn_fd, (struct sockaddr *)&peer_addr, &addrsize);

        for (i = 0; i <= PCC_END - PCC_SET; i++) {
            pcc_cur[i] = 0;
        }
        pcc_count = 0;
        memset(buff, 0, MB * sizeof(char));

        // Receive file size
        written = 0;
        while (written < sizeof(fsize)) {
            written_cur = read(conn_fd, buff, sizeof(fsize) - written);
            if (written_cur < 0) {
                fprintf(stderr, "%s\n", strerror(errno));
                return errno;
            }
            written += written_cur;
        }
        buff[written] = '\0';
        fsize = ntohl(strtoul(buff, NULL, 0));

        // Receive file data
        written = 0;
        while (written < fsize) {
            in_buff = read(conn_fd, buff, sizeof(buff));
            if (in_buff < 0) {
                fprintf(stderr, "%s\n", strerror(errno));
                return errno;
            }
            for (i = 0; i < in_buff; i++) {
                if (PCC_SET <= (int)buff[i] && (int)buff[i] <= PCC_END) {
                    pcc_count++;
                    pcc_cur[(int)buff[i]]++;
                }
            }
            written += in_buff;
        }

        // Send response
        sprintf(buff, "%u", htonl(pcc_count));
        buff[sizeof(pcc_cur)] = '\0';
        written = 0;
        while (written < sizeof(pcc_count)) {
            written_cur = write(conn_fd, buff, sizeof(pcc_count) - written);
            if (written_cur < 0) {
                fprintf(stderr, "%s\n", strerror(errno));
                return errno;
            }
            written += written_cur;
        }

        // Print PCC if signal caught
        if (pending_sig) {
            for (i = 0; i <= PCC_END - PCC_SET; i++) {
                printf("char '%c' : %u times\n", (char)(i + PCC_SET), pcc_total[i]);
            }
            exit(SIGINT);
        }

        handling = false;
    }
}