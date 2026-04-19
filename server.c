#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define END_SEQ -1   /* matches client sentinel */

struct Packet {
    int seq;
    int size;
    unsigned short checksum;
    char data[BUFFER_SIZE];
};

unsigned short calculate_checksum(char *data, int len) {
    unsigned int sum = 0;
    for (int i = 0; i < len; i++) {
        sum += (unsigned char)data[i];
    }
    return (unsigned short)(sum & 0xFFFF);
}

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);

    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    FILE *file = fopen("output.txt", "wb");
    if (!file) {
        perror("File error");
        return 1;
    }

    struct Packet pkt;
    int total_received = 0;
    int total_corrupt  = 0;
    int total_ok       = 0;
    long total_bytes   = 0;

    printf("Server listening on port %d...\n\n", PORT);

    while (1) {
        int n = recvfrom(sockfd, &pkt, sizeof(pkt), 0,
                         (struct sockaddr *)&client_addr, &len);

        if (n <= 0) continue;   /* ignore spurious errors, keep waiting */

        /* FIX: detect END sentinel and break cleanly */
        if (pkt.seq == END_SEQ) {
            printf("Received END sentinel — closing file.\n");
            break;
        }

        total_received++;

        unsigned short received   = pkt.checksum;
        unsigned short calculated = calculate_checksum(pkt.data, pkt.size);

        if (received != calculated) {
            total_corrupt++;
            printf("[PKT %4d]  CORRUPTED  | checksum expected=0x%04X got=0x%04X\n",
                   pkt.seq, calculated, received);
        } else {
            total_ok++;
            total_bytes += pkt.size;
            fwrite(pkt.data, 1, pkt.size, file);
            printf("[PKT %4d]  OK         | %d bytes  cumulative: %ld bytes\n",
                   pkt.seq, pkt.size, total_bytes);
        }

        printf("           >> recv=%d  ok=%d  corrupt=%d  loss=%.1f%%\n\n",
               total_received, total_ok, total_corrupt,
               total_received > 0 ? (total_corrupt * 100.0 / total_received) : 0.0);
    }

    /* FIX: fclose now always called — flushes all buffered data to disk */
    fclose(file);
    close(sockfd);

    printf("\n--- Server Summary ---\n");
    printf("Total received     : %d\n", total_received);
    printf("Delivered (OK)     : %d\n", total_ok);
    printf("Corrupted/dropped  : %d\n", total_corrupt);
    printf("Total bytes written: %ld\n", total_bytes);
    if (total_received > 0)
        printf("Packet loss rate   : %.1f%%\n", total_corrupt * 100.0 / total_received);
    printf("File received successfully!\n");

    return 0;
}