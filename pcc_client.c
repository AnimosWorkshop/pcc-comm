#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MB (1024 * 1024)


int main(int argc, char **argv) {
    struct sockaddr_in serv_addr, my_addr, peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in);
    int file_fd, sock_fd;
    u_int32_t fsize, written, written_cur, in_buff, from_buff, pcc_count;
    char buff[MB];

    if (argc != 4) {
        fprintf(stderr, "Argument count should be 4, entered %d.\n", argc);
        return -EINVAL;
    }
    
    memset(&serv_addr, 0, addrsize);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));
    file_fd = open(argv[3], O_RDONLY);

    if (file_fd < 0) {
        fprintf(stderr, "%s\n", strerror(ENOENT));
        return -ENOENT;
    }
    fsize = lseek(file_fd, 0, SEEK_END);
    sprintf(buff, "%u", htonl(fsize));
    lseek(file_fd, 0, SEEK_SET);

    // Create socket
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Could not create socket. %s\n", strerror(errno));
        return -errno;
    }

    // Connect
    if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "%s\n", strerror(ETIMEDOUT));
        return -ETIMEDOUT;
    }

    getsockname(sock_fd, (struct sockaddr *)&my_addr, &addrsize);
    getpeername(sock_fd, (struct sockaddr *)&peer_addr, &addrsize);

    memset(buff, 0, MB * sizeof(char));

    // Send file size
    written = 0;
    while (written < sizeof(fsize)) {
        written_cur = write(sock_fd, buff, sizeof(fsize) - written);
        if (written_cur < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            return errno;
        }
        written += written_cur;
    }

    // Send file data
    written = 0;
    while (written < fsize) {
        in_buff = read(file_fd, buff, sizeof(buff));
        if (in_buff < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            return errno;
        }
        from_buff = 0;
        while (from_buff < in_buff) {
            written_cur = write(sock_fd, buff + from_buff, in_buff - from_buff);
            if (written_cur < 0) {
                fprintf(stderr, "%s\n", strerror(errno));
                return errno;
            }
            from_buff += written_cur;
        }
    }

    // Receive response
    written = 0;
    while (written < sizeof(pcc_count)) {
        written_cur = read(sock_fd, buff, sizeof(pcc_count) - written);
        if (written_cur < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            return errno;
        }
        written += written_cur;
    }
    buff[written] = '\0';
    pcc_count = ntohl(strtoul(buff, NULL, 0));
    printf("# of printable characters: %u\n", pcc_count);

    close(sock_fd);

    return 0;
}
