#include <unistd.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <map>
#include <pthread.h>

struct Node {
    char symbol;
    double prob;
    double cumux;
    std::string binary;
    pthread_t threads;
    int portno;
    char* gethost;
};

void* serverpass(void* ptr)
{
    Node *p = (Node*)ptr;
    double probs = p->prob;
    double cumulative = p->cumux;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    server = gethostbyname(p->gethost);
    if (server == NULL)
    {
        std::cerr << "ERROR, no such host\n";
        exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(p->portno); //portnumber being access from struct here

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0)
            std::cerr << "ERROR opening socket";
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "ERROR connecting" << std::endl;
        exit(1);
    }

    int n;

    n = write(sockfd, &probs, sizeof(double));
    if(n < 0)
    {
        std::cerr << "ERROR writing to socket" << std::endl;
        exit(1);
    }
    n = write(sockfd, &cumulative, sizeof(double));
    if(n < 0)
    {
        std::cerr << "ERROR writing to socket" << std::endl;
        exit(1);
    }
    int size;
    n = read(sockfd, &size, sizeof(int));
    if (n < 0)
    {
        std::cerr << "ERROR reading from socket" << std::endl;
        exit(1);
    }
    char *buffer = new char[size + 1];
    bzero(buffer, size + 1);
    n = read(sockfd, buffer, size);
    p->binary = buffer;
    delete [] buffer;
    close(sockfd);

    return nullptr;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "usage " << argv[0] << "hostname port\n";
        exit(0);
    }

    //Map is used to store the frequency for each letter
    std::string characters;
    getline(std::cin, characters);
    std::map<char,int> map;
    for(auto i: characters){
        map[i]++;
    }

    //gets the size of the map, allows me to make the size of the struct
    int total = map.size();

    //Inserts the symbols to the struct and calculates the probability of each corresponding symbol and inserting them.
    //Calculates the cumulative x and stores it to the corresponding symbol
    Node Character[total];
    double probability;
    int count = 0;
    double tempf;
    for (auto it = map.begin(); it != map.end(); it++) {
        Character[count].symbol = it->first;
        probability = it->second;
        probability = probability / characters.length();
        Character[count].prob = probability;
        tempf += Character[count].prob;
        Character[count].cumux = tempf;
        count++;
    }

    //Assign a portnumber and hostname here
    for(int i = 0; i < total; i++)
    {
        Character[i].portno = atoi(argv[2]);
        Character[i].gethost = argv[1];
    }

    for(int i = 0; i < total; i++)
    {
        if(pthread_create(&Character[i].threads, NULL, &serverpass, &Character[i]))
        {
            std::cout << "Error creating thread" << std::endl;
            return 1;
        }
    }
    for(int i = 0; i < total; i++)
    {
        pthread_join(Character[i].threads, NULL);
    }
    
    
    std::cout << "SHANNON-FANO-ELIAS Codes:" << std::endl << std::endl;
    for (int i = 0; i < count; i++) {
        std::cout << "Symbol " << Character[i].symbol << ", " << "Code: " << Character[i].binary << std::endl;
    }

    return 0;
}
