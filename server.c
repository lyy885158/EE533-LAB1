#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>

void error(char *msg) {
    perror(msg);
    exit(1);
}

// Handler for zombie processes
void sigchld_handler(int s) {
    while(wait(NULL) > 0);
}

void dostuff(int sockfd) {
    char buffer[256];
    int n;
    
    // Clear the buffer
    bzero(buffer, 256);
    
    // Read from socket
    n = read(sockfd, buffer, 255);
    if (n < 0) error("ERROR reading from socket");
    
    printf("Here is the message: %s\n", buffer);
    
    // Send response back
    n = write(sockfd, "I got your message", 18);
    if (n < 0) error("ERROR writing to socket");
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno, pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    struct sigaction sa;

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    // Initialize socket structure
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Bind the socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR on binding");

    // Setup signal handler for zombie processes
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        error("sigaction");
    }

    // Listen for connections
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    // Main server loop
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) 
            error("ERROR on accept");

        // Create child process
        pid = fork();
        if (pid < 0)
            error("ERROR on fork");

        if (pid == 0) {
           
            close(sockfd);  
            dostuff(newsockfd);
            close(newsockfd);
            exit(0);
        }
        else {

            close(newsockfd);  
        }
    }
    return 0;
}