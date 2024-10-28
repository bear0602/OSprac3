#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

//print error and exit program
void error(char *msg) {
    perror(msg);
    exit(1);
} 

// handle client connection
void *handle_connection(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);
}

//nolds data
typedef struct Node {
    char line[256];
    struct Node *next;
    struct Node *book_next;
    struct Node *next_frequent_search; 
} Node;

void save_book(Node *book_head, int book_id) {
    char filename[20];
    sprintf(filename, "book_%02d.txt", book_id);
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    Node *current = book_head;
    while (current != NULL) {
        fprintf(file, "%s\n", current->line);
        current = current ->book_next;
    }

    fclose(file);
}

int main(int argc, char *argv[ ]) {
    int sockfd, newsockfd, portno = 1234;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            portno = atoi(argv[i + 1]);
            i++;
        }
    }

    printf("Using port: %d\n", portno);

    // check port number entered correctly
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    } 

    //create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket"); //socket creation error
    }

    //clear and initalise serve address structure
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET; //set address family to IPv4
    serv_addr.sin_port = htons(portno); //set port number

    //bind socket to port and address given
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }

    //set sockt to listen for incomming connections with max backlog of 5
    listen(sockfd, 5);
     //set client address length for accept call
    clilen = sizeof(cli_addr);
    //accept incoming connection and create a new socket
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) {
        error("ERROR on accept"); // accept error
    }

    //recieve data if nothing to recieve
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    //creates a new thread to handle connection
    pthread_t client_thread;
    pthread_create(&client_thread, NULL, handle_connection, (void *)&newsockfd);  

    //close sockets
    close(newsockfd);
    close(sockfd);
    return 0;

    
}