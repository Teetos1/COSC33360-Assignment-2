#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <pthread.h>
#include <cmath>

//Stores the probability and cumulative x to calculate the binary in a thread;
struct Node {
    double prob;
    double cumux;
    std::string binary;
};

//Function threadbar gets an individual symbol and calculates the fbar.
//Calculates the total of 1's and 0's each symbol has.
//Converts the fbar to binary in respect to the length.
std::string threadbinary(Node ptr)
{
    double bar;
    int total = 0;
    bar = (ptr.cumux - ptr.prob) + ptr.prob / 2;
    total = ceil(log2(1 / ptr.prob)) + 1;

    for(int i = 0; i < total; i++) {
        ptr.binary += '0';
    }

    int x;
    double j;
    for(x = 0, j = 0.5; x < ptr.binary.length(); x++, j /= 2) {
        if(j > bar)
            continue;
        ptr.binary[x] = '1';
        bar -= j;
    }
    
    return ptr.binary;
}

void fireman(int)
{
   while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    signal(SIGCHLD, fireman); 
    if (argc < 2)
    {
        std::cerr << "ERROR, no port provided\n";
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "ERROR opening socket";
        exit(1);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
    {
        std::cerr << "ERROR on binding";
        exit(1);
    }
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    while (true)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
        if (fork() == 0)
        {
            if (newsockfd < 0)
            {
                std::cerr << "ERROR on accept";
                exit(1);
            }
            double prob;
            n = read(newsockfd, &prob, sizeof(double));
            if(n < 0)
            {
                std::cerr << "ERROR reading from socket";
                exit(1);
            }
            double cumulative;
            n = read(newsockfd, &cumulative, sizeof(double));
            if(n < 0)
            {
                std::cerr << "ERROR reading from socket";
                exit(1);
            }

            //Makes a struct to store the variables coming from the client side
            Node p;
            p.prob = prob;
            p.cumux = cumulative;
            
            //Calculates the binary of the symbol
            p.binary = threadbinary(p);
            
            //Converting the string to char array to be able to get sent back to the client side
            const char *message = p.binary.c_str();
            int sMessage = strlen(message);
            n = write(newsockfd, &sMessage, sizeof(int));
            if (n < 0)
            {
                std::cerr << "ERROR writing to socket";
                exit(1);
            }
            n = write(newsockfd, message, sMessage);
            if (n < 0)
            {
                std::cerr << "ERROR writing to socket";
                exit(1);
            }
            close(newsockfd);
            _exit(0);
        }
    }
    close(sockfd);
    return 0;
}
