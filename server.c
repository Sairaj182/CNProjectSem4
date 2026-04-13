// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

struct Packet {
    int seq;
    int size;
    unsigned short checksum;
    char data[BUFFER_SIZE];
};

unsigned short calculate_checksum(char *data, int len) {
    unsigned int sum = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }
    return (unsigned short)(sum & 0xFFFF);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    FILE *file = fopen("output.txt", "wb");
    if (!file) {
        perror("File error");
        return 1;
    }

    struct Packet pkt;

    while (1) {
        int n = recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&client_addr, &len);

        if (n <= 0) break;

        unsigned short received = pkt.checksum;
        unsigned short calculated = calculate_checksum(pkt.data, pkt.size);

        if (received != calculated) {
            printf("Packet %d corrupted\n", pkt.seq);
            continue;
        }

        fwrite(pkt.data, 1, pkt.size, file);

        printf("Received packet %d \n", pkt.seq);
    }

    fclose(file);
    close(sockfd);

    printf("File received successfully!\n");
    return 0;
}