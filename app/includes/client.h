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
#include "text_buffer_structs.h"

extern int    clientIsWorking = TRUE;
extern gulong onChangeSignalId = FALSE;
extern int isServerSendingData = FALSE;
extern int lastCollaborator = 0;

G_LOCK_DEFINE_STATIC (lockParsingIncomingMessage);
static volatile int lockParsingIncomingMessage = FALSE;

void *gtkListener();

char *messageToString(message_t *message);

void sendMessageToServer(message_t *message, int serverSocket);

/**
 * Closes socket descriptor
 */
void disconnectFromServer(int serverSocket);

/**
 * Pass in 1 parameter which is either the
 * address or host name of the app, or
 * set the app name in the #define
 * SERVER_NAME.
 */
int connectToServer(char *server_name, int server_port);