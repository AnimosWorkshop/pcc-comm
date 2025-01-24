#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>

#define MB (1024 * 1024)


int main(int argc, char **argv) {
    struct sockaddr_in serv_addr, my_addr, peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in);
    int file_fd, sock_fd;
    u_int32_t fsize, written, written_cur, in_buff, from_buff, response;
    char buff[MB];

    if (argc != 4) {
        fprintf(stderr, "Argument count should be 4, entered %d.\n", argc);
        return -EINVAL;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = ntohs(atoi(argv[2]));
    file_fd = open(argv[3], O_RDONLY);

    if (file_fd < 0) {
        fprintf(stderr, strerror(ENOENT), argv[3]);
        return -ENOENT;
    }
    fsize = lseek(file_fd, 0, SEEK_END);
    sprintf(buff, "%lu", hton(fsize));
    lseek(file_fd, 0, SEEK_SET);

    // Create socket
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Could not create socket.\n");
        return -EINVAL;
    }

    // Connect
    if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, strerror(ETIMEDOUT));
        return -ETIMEDOUT;
    }

    getsockname(sock_fd, (struct sockaddr *)&my_addr, &addrsize);
    getpeername(sock_fd, (struct sockaddr *)&peer_addr, &addrsize);

    // Send file size
    written = 0;
    while (written < sizeof(fsize)) {
        written_cur = write(sock_fd, buff, sizeof(fsize) - written);
        if (written_cur < 0) {
            fprintf(stderr, strerror(errno));
            return errno;
        }
        written += written_cur;
    }

    // Send file data
    written = 0;
    while (written < fsize) {
        written_cur = read(file_fd, buff, sizeof(buff));
        if (written_cur < 0) {
            fprintf(stderr, strerror(errno));
            return errno;
        }
        in_buff = written_cur;
        from_buff = 0;
        while (from_buff < in_buff) {
            written_cur = write(sock_fd, buff, sizeof(fsize) - written);
            if (written_cur < 0) {
                fprintf(stderr, strerror(errno));
                return errno;
            }
            from_buff += written_cur;
        }
    }

    // Read response
    written = 0;
    while (written < sizeof(response)) {
        written_cur = read(sock_fd, buff, sizeof(response) - written);
        if (written_cur < 0) {
            fprintf(stderr, strerror(errno));
            return errno;
        }
        written += written_cur;
    }
    buff[written] = '\0';
    response = ntohi(strtoul(buff, NULL, 0));
    printf("# of printable characters: %u\n");

    return 0;
}
