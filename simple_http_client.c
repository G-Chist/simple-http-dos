#include <sys/types.h>
#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h> /* for sockaddr_in and inet_addr() */
#include <stdlib.h> /* for atoi() and exit() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for close() */
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>
#define RCVBUFSIZE 1000000 /* Size of receive buffer */
int errno;
void DieWithError(char *errorMessage); /* Error handling function */ 

//define getaddrinfo struct
    int getaddrinfo(const char *node,
                    const char *service,
                    const struct addrinfo *hints,
                    struct addrinfo **res);

int main(int argc, char* argv[])
{
    //get user input
    int printRTT = 0;
    char *url;
    char *port;

    if(argc == 3)
    {
        url = argv[1];
        port = argv[2];
    }
    else if(argc == 4 && strcmp(argv[1], "-p") == 0)
    {
        printRTT = 1;
        url = argv[2];
        port = argv[3];
    }
    else
    {
        printf("Usage: %s [-options] server_url port_number\n", argv[0]);
        exit(1);
    }

    //If the URL contains a filepath, get it
    int slashPos;
    char defaultFilepath[] = "index.html";
    char* filepath = &defaultFilepath[0];
    if(strchr(url, '/') != NULL)
    {
        slashPos = (int)(strchr(url, '/') - url);
        filepath = &url[slashPos + 1];
        url[slashPos] = '\0';
    }

    //call getaddrinfo()
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if((status = getaddrinfo(url, port, &hints, &servinfo)) != 0)
    {
        DieWithError("getaddrinfo() failed\n");
        exit(1);
    }

    //call socket()
    int sockfd;
    if((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0)
    {
        DieWithError("socket() failed\n");
        close(sockfd);
        exit(1);
    }

    //before calling connect(), get time of day
    struct timeval t0;
    gettimeofday(&t0, NULL);

    //call connect()
    int connect_status;
    if((connect_status = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) < 0)
    {
        DieWithError("connect() failed");
        close(sockfd);
        exit(1);
    }
    freeaddrinfo(servinfo);

    //Get new time of day after connect() and calculate RTT estimate
    struct timeval t1;
    gettimeofday(&t1, NULL);
    float timediff = (float)((int)t1.tv_usec - (int)t0.tv_usec) / 1000.0;

    //write a HTTP message
    char request[strlen(url) + strlen(filepath) + 40];
    sprintf(request, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", filepath, url);
    printf("-----------------HTTP REQUEST SENT-----------------\n");
    printf("%s", request);

    //send HTTP message
    int len, bytes_sent;
    len = strlen(request);
    if((bytes_sent = send(sockfd, request, len, 0)) < 0)
    {
        DieWithError("send() failed");
        close(sockfd);
        exit(1);
    }

    //receive server response
    int bytes_read, total_read;
    char buffer[RCVBUFSIZE];
    while((bytes_read = recv(sockfd, &buffer[total_read], RCVBUFSIZE - 1, 0)) > 0)
    {
        total_read += bytes_read;
    }
    buffer[RCVBUFSIZE] = 0;
    close(sockfd);

    //Save buffer to a new file
    FILE* fp;
    fp = fopen("index.html", "w");
    fprintf(fp, "%s", buffer);
    fclose(fp);
    printf("Server response saved to index.html\n\n");


    //Print RTT message (IF FLAG)
    if(printRTT)
    {
        printf("RTT (milliseconds): %.3f\n\n", timediff);
    }

    return 0;
}

void DieWithError(char *errorMessage) //display error message with error code
{
    printf("%s\nerror code: %d\n", errorMessage, errno);
}