#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#define BUFFER_SIZE 256

void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Sunucudan gelen mesajları dinleyen thread
void *receive_messages(void *sockfd_ptr) {
    int sockfd = *(int *)sockfd_ptr;
    char buffer[BUFFER_SIZE];
    int n;

    while (1) {
        bzero(buffer, BUFFER_SIZE);
        n = read(sockfd, buffer, BUFFER_SIZE - 1);
        if (n < 0) {
            error("Error reading from socket");
        } else if (n == 0) {
            printf("Server closed the connection.\n");
            break;
        }
        printf("Server: %s", buffer);
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];
    pthread_t recv_thread;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
        exit(0);
    }

    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error opening socket!");
    }

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "Error, no such host\n");
        exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        error("Connection failed");
    }

    printf("Connected to the server. You can start sending messages.\n");

    // Sunucudan gelen mesajları dinleyen thread başlatılıyor
    if (pthread_create(&recv_thread, NULL, receive_messages, (void *)&sockfd) != 0) {
        error("Error creating thread");
    }

    while (1) {
        // Kullanıcıdan mesaj al
        bzero(buffer, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE - 1, stdin);

        // Mesajı sunucuya gönder
        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0) {
            error("Error writing to socket");
        }

        // "Bye" mesajı gönderilirse bağlantıyı sonlandır
        if (strncmp("Bye", buffer, 3) == 0) {
            printf("Disconnected from server.\n");
            break;
        }
    }


    pthread_join(recv_thread, NULL);
    close(sockfd);
    return 0;
}
