/**************************************************************************/
/* This sample program provides a code for a connection-oriented client.  */
/**************************************************************************/

/**************************************************************************/
/* Header files needed for this sample program                            */
/**************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include <netdb.h>
#include <stdlib.h>

void sendMessageToServer(message_t message, int serverSocket) {
    // prepare the buffer for message
    int size = sizeof(message_t);
    char* buffer = malloc(size);
    memset(buffer, 0x00, size);
    memcpy(buffer, &message, size);

    if ((send(serverSocket, buffer, size, 0)) < 0) {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }

    free(buffer);
}


/**
 * Closes socket descriptor
 */
void disconnectFromServer(int serverSocket) {
    if (serverSocket != -1) close(serverSocket);
}

/**
 * Pass in 1 parameter which is either the
 * address or host name of the app, or
 * set the app name in the #define
 * SERVER_NAME.
 */
int connectToServer(char *server_name, int server_port) {
    // Variable and structure definitions.
    int                serverSocket = -1;
    char               host[128];
    struct sockaddr_in serverAddress;
    struct hostent     *hostp;

    /*
     * The socket() function returns a socket descriptor representing
     * an endpoint.  The statement also identifies that the INET
     * (Internet Protocol) address family with the TCP transport
     * (SOCK_STREAM) will be used for this socket.
     */
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    /*
     * If an argument was passed in, use this as the app, otherwise
     * use the #define that is located at the top of this program.
     */
    strcpy(host, server_name);

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family      = AF_INET;
    serverAddress.sin_port        = htons(server_port);
    serverAddress.sin_addr.s_addr = inet_addr(host);
    if (serverAddress.sin_addr.s_addr == (unsigned long) INADDR_NONE) {

        /*
         * The app string that was passed into the inet_addr()
         * function was not a dotted decimal IP address.  It must
         * therefore be the hostname of the app.  Use the
         * gethostbyname() function to retrieve the IP address of the
         * app.
         */
        hostp = gethostbyname(host);
        if (hostp == (struct hostent *) NULL) {
            printf("Host not found --> ");
            printf("h_errno = %d\n", h_errno);
            exit(EXIT_FAILURE);
        }

        memcpy(&serverAddress.sin_addr, hostp->h_addr, sizeof(serverAddress.sin_addr));
    }

    /*
     * Use the connect() function to establish a connection to the app.
     */
    if (connect(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        perror("connect() failed");
        exit(EXIT_FAILURE);
    }

    return serverSocket;
}