#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

void error(const char *msg){
    perror(msg);
    exit(1);
}


int calculate_expression(const char *expression){
    
    
    int num1, num2;
    char op;

    sscanf(expression,"%d%c%d",&num1, &op, &num2);

    switch(op){
    case '+':return num1 + num2;
    case '-':return num1 - num2;
    case '*':return num1*num2;
    case '/':return (num2!=0)?num1/num2 : 0;
    default: return 0;}

}




int main(int argc , char *argv[]){



    if(argc < 2){
        fprintf(stderr, "Port no not provided. Program terminated\n");
        exit(1);
    }


    int sockfd, newsockfd, portno, n;
    char buffer[255];

    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd <0){

        error("Error opening socket!");
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        error("Binding failed.");}

    listen(sockfd , 5);
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if(newsockfd < 0)
        error("Error on accept!");

    while(1){

        bzero(buffer, 256);
        n = read(newsockfd, buffer, 255);
        if(n < 0)
            error("Error on reading!");
        else
            printf("Client : %s\n", buffer);
        //bzero(buffer , 255);
        //fgets(buffer, 255, stdin);
        
        int result = calculate_expression(buffer);
        sprintf(buffer, "Result : %d\n" , result);

        n = write(newsockfd, buffer, strlen(buffer));
        if(n < 0)
            error("Error on writing!");

        
        
        int i = strncmp("Bye", buffer, 3);
        if(i==0)
        break;
    }

    close(newsockfd);
    close(sockfd);
    return 0;
}
