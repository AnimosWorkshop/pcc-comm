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

    memset(buff, 0, MB * sizeof(char));

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
    buff[10] = '\0';
    lseek(file_fd, 0, SEEK_SET);

    // Create socket
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Could not create socket. %s\n", strerror(errno));
        return -errno;
    }
    getsockname(sock_fd, (struct sockaddr *)&my_addr, &addrsize);

    printf("Socket created at %s:%u\n", inet_ntoa(my_addr.sin_addr), ntohs(my_addr.sin_port));
    printf("Trying to connect to %s:%u\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
    
    // Connect
    if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Connection error. %s\n", strerror(ETIMEDOUT));
        close(sock_fd);
        return -ETIMEDOUT;
    }

    printf("Connected successfully.\n");

    getsockname(sock_fd, (struct sockaddr *)&my_addr, &addrsize);
    getpeername(sock_fd, (struct sockaddr *)&peer_addr, &addrsize);

    // Send file size
    printf("Sending file size %u.\n", fsize);
    written = 0;
    while (written < 10) {
        written_cur = write(sock_fd, buff, 10 - written);
        if (written_cur < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            close(sock_fd);
            return errno;
        }
        written += written_cur;
    }

    memset(buff, 0, MB * sizeof(char));

    // Send file data
    printf("Sending file data.\n");
    in_buff = 0;
    while (in_buff < fsize) {
        in_buff = read(file_fd, buff, sizeof(buff));
        printf("in_buff %u\n", in_buff);
        if (in_buff < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            close(sock_fd);
            return errno;
        }
        from_buff = 0;
        while (from_buff < in_buff) {
            written_cur = write(sock_fd, buff + from_buff, in_buff - from_buff);
            printf("written_cur %u\n", written_cur);
            if (written_cur < 0) {
                fprintf(stderr, "%s\n", strerror(errno));
                close(sock_fd);
                return errno;
            }
            from_buff += written_cur;
            printf("from_buff %u\n", from_buff);
        }
    }

    // Receive response
    printf("Awaiting response.\n");
    written = 0;
    while (written < 10) {
        written_cur = read(sock_fd, buff, 10 - written);
        if (written_cur < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            close(sock_fd);
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
