// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

struct Packet {
    int seq;
    int size;
    unsigned short checksum;
    char data[BUFFER_SIZE];
};

int generateNoise() {
    return (rand()%10)+1;
}

unsigned short calculate_checksum(char *data, int len) {
    unsigned int sum = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }
    return (unsigned short)(sum & 0xFFFF);
}

int main() {
    srand(time(NULL)); 
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    FILE *file = fopen("input.txt", "rb");
    if (!file) {
        perror("File open error");
        return 1;
    }

    struct Packet pkt;
    int seq = 0;

    while (1) {
        pkt.seq = seq;

        pkt.size = fread(pkt.data, 1, BUFFER_SIZE, file);
        if (pkt.size <= 0) break;

        pkt.checksum = calculate_checksum(pkt.data, pkt.size);
        int noise = generateNoise();
        if(noise>8){
            int corruptDiv = generateNoise();
            pkt.data[(pkt.size-1)/corruptDiv] = pkt.data[(pkt.size-1)/corruptDiv] + 1;
        }
        sendto(sockfd, &pkt, sizeof(pkt), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr));

        printf("Sent packet %d\n", seq);
        seq++;
    }

    fclose(file);
    close(sockfd);

    printf("File sent successfully!\n");
    return 0;
}