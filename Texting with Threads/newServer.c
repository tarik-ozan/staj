#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 256

// Client soketleri değişkenler
int client_sockets[MAX_CLIENTS];
int client_count = 0;

// Client listesi mutex
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


void error(const char *msg) {
    perror(msg);
    exit(1);
}


void broadcast_message(const char *message) {
    pthread_mutex_lock(&clients_mutex);

    for(int i = 0; i < client_count; i++) {
            if(write(client_sockets[i], message, strlen(message)) < 0) {
                perror("Error writing to client");
            }
        }
    pthread_mutex_unlock(&clients_mutex);
    }





void remove_client(int sock) {
    pthread_mutex_lock(&clients_mutex);

    int i;
    for(i = 0; i < client_count; i++) {
        if(client_sockets[i] == sock) {
            break;
        }
    }

    if(i < client_count) {
        // Son istemciyi bu konuma koy
        client_sockets[i] = client_sockets[client_count - 1];
        client_count--;
    }

    pthread_mutex_unlock(&clients_mutex);
}

int client_number=1;

// Client thread fonksiyonu
void *handle_client(void *arg) {
    int sock = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    int n;

    // Client'e sıra numarası ver
    int client_id = client_number++;
    printf("Client [%d] connected.\n", client_id);

    while(1) {
        bzero(buffer, BUFFER_SIZE);
        n = read(sock, buffer, BUFFER_SIZE - 1);
        if(n < 0) {
            perror("Error reading from socket");
            break;
        } else if(n == 0) {
            printf("Client [%d] disconnected.\n", client_id);
            break;
        }

        buffer[n] = '\0'; // Null terminator ekle

        // Mesajı server tarafında göster
        printf("Client [%d]: %s\n", client_id, buffer);

        if(strncmp("Bye", buffer, 3) == 0) {
            break;
        }
    }

    close(sock);
    remove_client(sock);
    return NULL;
}

// Sunucudan gelen mesajları okuyup tüm clientlere göndewrmek için thread
void *server_input(void *arg) {
    char message[BUFFER_SIZE];

    while(1) {
        // Sunucudan mesaj girişi al
        if(fgets(message, BUFFER_SIZE, stdin) == NULL) {
            perror("Error reading from stdin");
            break;
        }

        // Mesajı yayınla
        broadcast_message(message);

        // "Bye" mesajı girilirse, tüm istemcilerle bağlantıyı sonlandır
        if(strncmp("Bye", message, 3) == 0) {
            printf("Server is shutting down.\n");
            // Tüm istemcileri kapat
            pthread_mutex_lock(&clients_mutex);
            for(int i = 0; i < client_count; i++) {
                close(client_sockets[i]);
            }
            pthread_mutex_unlock(&clients_mutex);
            exit(0);
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int sockfd, newsockfd, portno;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    // Soket oluştur
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        error("Error opening socket!");
    }

    // Sunucu adresini sıfırla
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Soketi bağla
    if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        error("Binding failed.");
    }

    // Dinlemeye başla
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    printf("Server started on port %d. Waiting for connections...\n", portno);

    // Sunucu giriş iş parçacığını başlat
    pthread_t input_thread;
    if(pthread_create(&input_thread, NULL, server_input, NULL) != 0) {
        error("Could not create server input thread");
    }

    // Yeni istemcileri kabul et ve thread oluştur
    while(1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if(newsockfd < 0) {
            perror("Error on accept!");
            continue;
        }

        printf("New client connected: %d\n", client_number);

        // Client'i listeye ekle
        pthread_mutex_lock(&clients_mutex);
        if(client_count < MAX_CLIENTS) {
            client_sockets[client_count++] = newsockfd;
        } else {
            printf("Max clients reached. Rejecting new client.\n");
            close(newsockfd);
            pthread_mutex_unlock(&clients_mutex);
            continue;
        }
        pthread_mutex_unlock(&clients_mutex);

        // İstemci thread oluştur
        pthread_t client_thread;
        int *pclient = malloc(sizeof(int));
        if(pclient == NULL) {
            perror("Malloc failed");
            exit(1);
        }
        *pclient = newsockfd;

        if(pthread_create(&client_thread, NULL, handle_client, pclient) != 0) {
            perror("Could not create thread");
            free(pclient);
            continue;
        }


        pthread_detach(client_thread);
    }


    close(sockfd);
    return 0;
}