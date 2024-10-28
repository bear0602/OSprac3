#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// Print error and exit program
void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Holds data
typedef struct Node {
    char line[256];
    struct Node *next;
    struct Node *book_next;
    struct Node *next_frequent_search; 
} Node;

// Save book data to a file
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
        current = current->book_next;
    }

    fclose(file);
}

// Function to handle each connection
void *handle_connection(void *arg) {
    int client_sock = *(int*)arg;
    free(arg);

    Node *book_head = NULL, *tail = NULL;
    char buffer[256];
    int n;

    // Receive data until the connection is closed
    while ((n = recv(client_sock, buffer, sizeof(buffer), 0)) > 0) {
        buffer[n] = '\0'; // Null-terminate the received data
        
        // Create a new node and copy the line
        Node *new_node = malloc(sizeof(Node));
        if (new_node == NULL) {
            perror("Failed to allocate memory for a new node");
            close(client_sock);
            return NULL;
        }
        strncpy(new_node->line, buffer, sizeof(new_node->line));
        new_node->line[strcspn(new_node->line, "\n")] = 0; // Remove newline
        new_node->next = NULL;
        new_node->book_next = NULL;

        // Add the new node to the shared linked list
        if (book_head == NULL) {
            book_head = new_node; // First node
            tail = new_node; // First node is also the tail
        } else {
            tail->book_next = new_node; // Link the new node
            tail = new_node; // Move the tail pointer
        }
    }

    if (n < 0) {
        perror("Failed to receive data");
    }

    // Save the received book data to a file
    save_book(book_head, client_sock); // Using client_sock as book_id for now

    // Free allocated nodes
    Node *current = book_head;
    while (current != NULL) {
        Node *temp = current;
        current = current->book_next;
        free(temp);
    }

    close(client_sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    int sockfd, portno = 1234;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            portno = atoi(argv[i + 1]);
            i++;
        }
    }

    printf("Using port: %d\n", portno);

    // Check port number entered correctly
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    } 

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket"); // Socket creation error
    }

    // Clear and initialize server address structure
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; // Set address family to IPv4
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno); // Set port number

    // Bind socket to port and address given
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }

    // Set socket to listen for incoming connections with max backlog of 5
    listen(sockfd, 5);
    // Set client address length for accept call
    while (1) {
        clilen = sizeof(cli_addr);
        int *newsockfd = malloc(sizeof(int));
        if (newsockfd == NULL) {
            perror("Failed to allocate memory for new socket");
            continue;
        }

        // Accept incoming connection and create a new socket
        *newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (*newsockfd < 0) {
            perror("ERROR on accept");
            free(newsockfd);
            continue;
        }

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_connection, (void *) newsockfd) != 0) {
            perror("failed to create a thread");
            close(*newsockfd);
            free(newsockfd);
            continue;
        }
        pthread_detach(client_thread);
    }   
    // Close sockets
    close(sockfd);
    return 0;
}
