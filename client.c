#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define END_SEQ -1   /* sentinel sequence number to signal end of transfer */

struct Packet {
    int seq;
    int size;
    unsigned short checksum;
    char data[BUFFER_SIZE];
};

int generateNoise() {
    return (rand() % 10) + 1;
}

unsigned short calculate_checksum(char *data, int len) {
    unsigned int sum = 0;
    for (int i = 0; i < len; i++) {
        sum += (unsigned char)data[i];
    }
    return (unsigned short)(sum & 0xFFFF);
}

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    srand(time(NULL));

    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    FILE *file = fopen("internalInput.txt", "rb");
    if (!file) {
        perror("File open error");
        return 1;
    }

    struct Packet pkt;
    int seq           = 0;
    int total_sent    = 0;
    int total_corrupt = 0;

    while (1) {
        memset(&pkt, 0, sizeof(pkt));
        pkt.seq  = seq;
        pkt.size = fread(pkt.data, 1, BUFFER_SIZE, file);

        if (pkt.size <= 0) break;

        /* Checksum computed on clean data first */
        pkt.checksum = calculate_checksum(pkt.data, pkt.size);

        /* Only send actual header + data bytes, no padding */
        int packet_size = sizeof(pkt.seq)
                        + sizeof(pkt.size)
                        + sizeof(pkt.checksum)
                        + pkt.size;

        /* Inject noise AFTER checksum is set */
        int noise = generateNoise();
        if (noise > 8 && pkt.size > 0) {
            int index = rand() % pkt.size;
            pkt.data[index] = pkt.data[index] + 1;
            total_corrupt++;
            printf("Sent packet %d (injected noise at byte %d)\n", seq, index);
        } else {
            printf("Sent packet %d\n", seq);
        }

        sendto(sockfd, &pkt, packet_size, 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr));

        total_sent++;
        seq++;
    }

    fclose(file);

    /* FIX: Send END sentinel so server knows transfer is complete
       and can cleanly fclose() and flush the output file */
    memset(&pkt, 0, sizeof(pkt));
    pkt.seq  = END_SEQ;
    pkt.size = 0;
    pkt.checksum = 0;
    int sentinel_size = sizeof(pkt.seq) + sizeof(pkt.size) + sizeof(pkt.checksum);
    sendto(sockfd, &pkt, sentinel_size, 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Sent END sentinel\n");

    close(sockfd);

    printf("\n--- Client Summary ---\n");
    printf("Total packets sent : %d\n", total_sent);
    printf("Packets with noise : %d (%.1f%%)\n",
           total_corrupt,
           total_sent > 0 ? (total_corrupt * 100.0 / total_sent) : 0.0);
    printf("File sent successfully!\n");

    return 0;
}