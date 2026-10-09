
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define CHUNK_SIZE 1024

int main(int argc, char *argv[])
{
    // Usage: ./client <server-ip> <port> <file>
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server-ip> <port> <file>\n",
                argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    const char *filename = argv[3];

    // 1. Open the file in binary mode.
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("fopen");
        return 1;
    }

    // 2. Create a UDP socket.
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        fclose(fp);
        return 1;
    }

    // 3. Set up the server's IPv4 address and port.
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip,
                  &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IPv4 address: %s\n", server_ip);
        close(sockfd);
        fclose(fp);
        return 1;
    }

    // 4. Read and send one chunk per UDP datagram.
    unsigned char buffer[CHUNK_SIZE];
    size_t bytes_read;
    uint32_t sequence = 0;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        ssize_t sent = sendto(
            sockfd,
            buffer,
            bytes_read,
            0,
            (struct sockaddr *)&server_addr,
            sizeof(server_addr)
        );

        if (sent < 0 || (size_t)sent != bytes_read) {
            perror("sendto");
            close(sockfd);
            fclose(fp);
            return 1;
        }

        printf("Sent chunk %u: %zu bytes\n",
               sequence, bytes_read);
        sequence++;
    }

    if (ferror(fp)) {
        perror("fread");
        close(sockfd);
        fclose(fp);
        return 1;
    }

    printf("Finished sending file bytes.\n");

    // 5. Release resources.
    fclose(fp);
    close(sockfd);

    return 0;
}
