#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h> /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h> /* for atoi() and exit() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for close() */
#include <errno.h>

#define MAXPENDING 5 /* Maximum outstanding connection requests */
#define RCVBUFSIZE 2000000
int errno;
void DieWithError(char *errorMessage); /* Error handling function */
void HandleTCPClient(int clntSocket); /* TCP client handling function */
void send404Response(int clientSocket); //Sends 404 Not Found Response
void sendOkResponse(char filebuf[], int clientSocket); //Sends 200 OK Response
void send400Response(int clientSocket); //Sends 400 Bad Request response
void sendResponse(char response[], int clientSocket); //Sends response to server

int main(int argc, char* argv[])
{
    //get user arguments
    unsigned short port;
    if(argc != 2)
    {
        fprintf(stderr, "Usage: %s <server port>", argv[0]);
        exit(1);
    }
    else
        port = atoi(argv[1]);

    //create socket
    int servSock;
    int clientSock;

    if((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        DieWithError("socket() failed");
        close(servSock);
        exit(1);
    }
    
    //construct address structure
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);

    //call setsockopt before bind (free up port in kernel)
    int yes = 1;
    if(setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        DieWithError("setsockopt() failed");
        close(servSock);
        exit(1);
    }

    //bind to the local address
    if(bind(servSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) > 0)
    {
        DieWithError("bind() failed");
        close(servSock);
        exit(1);
    }

    //Mark the socket so it will listen for incomming connections
    if(listen(servSock, MAXPENDING) < 0)
    {
        DieWithError("listen() failed");
        close(servSock);
        exit(1);
    }
    

    unsigned int clientLen;
    for(;;)
    {
        //Set the size of the in-out parameter
        clientLen = sizeof(clientAddr);
        if((clientSock = accept(servSock, (struct sockaddr *) &clientAddr, &clientLen)) < 0)
        {
            DieWithError("accept() failed");
            close(servSock);
            close(clientSock);
            exit(1);
        }
        
        //clientSock is connected to a client!
        HandleTCPClient(clientSock);
        close(clientSock);
    }

    close(servSock);
    return 0;
}

void HandleTCPClient(int clntSocket) //TCP client handling function
{
    //receive client request
    char buffer[RCVBUFSIZE];
    int bytes_read;
    if((bytes_read = recv(clntSocket, &buffer, RCVBUFSIZE - 1, 0)) < 0)
    {
        DieWithError("recv() failed");
        close(clntSocket);
        exit(1);
    }
    buffer[RCVBUFSIZE] = 0;

    printf("----------------HTTP REQUEST RECEIVED---------------\n");
    buffer[RCVBUFSIZE] = 0;
    printf("%s", buffer);

    //parse the client request
    char* request;
    char* filepath;
    int endOfRequest;
    int i;
    for(i = 0; buffer[i] != ' '; i++)
    {
        continue;
    }
    buffer[i] = '\0';
    endOfRequest = i;
    request = &buffer[0];
    for(i = i + 1; buffer[i] != ' '; i++)
    {
        continue;
    }
    buffer[i] = '\0';
    filepath = &buffer[endOfRequest + 1];

    FILE* fp;
    if(strcmp(request, "GET") == 0) //If the request is a GET request...
    {
        fp = fopen(&filepath[1], "r"); //Open the desired file
        if(fp == NULL)
        {
            send404Response(clntSocket); //If no file is found, then give a not found response
            return;
        }
        else
        {
            //If a file is found, read from the file 
            char filebuffer[RCVBUFSIZE];
            char ch;
            int charCounter = 0;
            while(ch != EOF)
            {
                ch = fgetc(fp);
                filebuffer[charCounter] = ch;
                charCounter++;
            }
            filebuffer[charCounter - 2] = '\0';

            //Write & send HTTP OK response
            sendOkResponse(filebuffer, clntSocket);
        }

        fclose(fp);
    }
    else
    {
        //If the request is not GET, then give a Bad Request response
        send400Response(clntSocket);
    }
}

void send400Response(int clientSocket)
{
    //Write the response
    char response[] = "HTTP/1.1 400 Bad Request\r\n";

    //Send the response
    sendResponse(response, clientSocket);
}

void send404Response(int clientSocket)
{
    //Write the response
    char response[] = "HTTP/1.1 404 Not Found\r\n";

    //Send the response
    sendResponse(response, clientSocket);
}

void sendOkResponse(char filebuf[], int clientSocket)
{
    //Write the response
    char header[] = "HTTP/1.1 200 OK\r\n";
    char response[RCVBUFSIZE];
    sprintf(response, "%s\r\n%s", header, filebuf);

    //Send the response
    sendResponse(response, clientSocket);
}

void sendResponse(char response[], int clientSocket)
{
    //Send the response to clientSocket;
    int len, bytes_sent;
    len = strlen(response);
    if((bytes_sent = send(clientSocket, response, len, 0)) < 0)
    {
        DieWithError("send() failed");
    }

    printf("Response sent to client!\n\n");
}

void DieWithError(char *errorMessage) //display error message with error code
{
    printf("%s\nerror code: %d\n", errorMessage, errno);
}