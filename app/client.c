#include <pthread.h>
#include <gtk/gtk.h>
#include "includes/common.h"
#include "includes/prepares.h"
#include "includes/bindings.h"

void display_login_window(WindowsAndConnection *windowsAndConnection) {
    GtkBuilder *builder;

    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "./../app/login_window_design.glade", NULL);

    windowsAndConnection->login_window = GTK_WIDGET(gtk_builder_get_object(builder, "login_window"));

    windowsAndConnection->server_ip   = GTK_ENTRY(gtk_builder_get_object(builder, "server_ip"));
    windowsAndConnection->server_port = GTK_ENTRY(gtk_builder_get_object(builder, "server_port"));
    GtkButton *connect_button = GTK_BUTTON(gtk_builder_get_object(builder, "connect"));

    gtk_builder_connect_signals(builder, NULL);
    g_signal_connect(connect_button, "clicked", G_CALLBACK(on_client_connect), windowsAndConnection);

    g_object_unref(builder);

    gtk_widget_show(windowsAndConnection->login_window);
    gtk_main();
}

void display_editor_window(WindowsAndConnection *windowsAndConnection) {
    int serverSocket = connectToServer(
        gtk_entry_get_text(windowsAndConnection->server_ip),
        atoi(gtk_entry_get_text(windowsAndConnection->server_port))
    );

    windowsAndConnection->editor_window = prepareWindow("Collaborative editor");
    GtkWidget   *vbox    = prepareVerticalBox(windowsAndConnection->editor_window);
    GtkWidget   *toolbar = prepareToolbar();
    GtkToolItem *exit    = prepareExitButton(toolbar);

    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 5);

    GtkWidget     *textView  = prepareTextView(vbox);
    GtkTextBuffer *buffer    = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
    GtkWidget     *statusbar = prepareStatusBar(vbox);

    gtk_widget_show_all(windowsAndConnection->editor_window);

    TextBufferData *data = malloc(sizeof(TextBufferData));
    data->statusbar    = statusbar;
    data->serverSocket = &serverSocket;
    bindEventListeners(windowsAndConnection->editor_window, exit, buffer, data);

    struct TextViewWithSocket *textViewWithSocket = malloc(sizeof(struct TextViewWithSocket));
    textViewWithSocket->textBuffer   = buffer;
    textViewWithSocket->bufferData   = data;
    textViewWithSocket->clientSocket = serverSocket;

    eventLoops(textViewWithSocket);
}

WindowsAndConnection *initWindowsAndConnection() {
    WindowsAndConnection * windowsAndConnection = malloc(sizeof(WindowsAndConnection));
    windowsAndConnection->server_ip = malloc(sizeof(GtkEntry));
    windowsAndConnection->server_port = malloc(sizeof(GtkEntry));
    windowsAndConnection->login_window = malloc(sizeof(GtkWidget));
    windowsAndConnection->editor_window = malloc(sizeof(GtkWidget));
    
    return windowsAndConnection;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    WindowsAndConnection *windowsAndConnection = initWindowsAndConnection();

    display_login_window(windowsAndConnection);
    display_editor_window(windowsAndConnection);

    return 0;
}

static gboolean resolveIncomingMessage(struct TextViewWithSocket *textViewWithSocket) {
    G_LOCK(lockParsingIncomingMessage); // TODO: a gówno to dało
    isServerSendingData = 1;

    GtkTextIter start, end;
    message_t *message = textViewWithSocket->lastReceivedMessage;

    switch (message->type) {
        case FINISHED_SENDING_DATA:
            isServerSendingData = 0;
            break;
        case LINE_ADDED:
            printf("LINE_ADDED:%d:`%s`\n", message->row, message->text);
            getBoundsOfLine(textViewWithSocket->textBuffer, message->row - 1, &start, &end);
            gtk_text_buffer_insert(textViewWithSocket->textBuffer, &end, "\n ", 2);
            getBoundsOfLine(textViewWithSocket->textBuffer, message->row, &start, &end);
            gtk_text_buffer_insert(textViewWithSocket->textBuffer, &start, message->text, strlen(message->text));

            break;
        case LINE_REMOVED:
            printf("LINE_REMOVED:%d:\n", message->row);
            gtk_text_buffer_get_iter_at_line(textViewWithSocket->textBuffer, &start, message->row);
            gtk_text_buffer_backspace(textViewWithSocket->textBuffer, &start, FALSE, TRUE);

            break;
        case LINE_MODIFIED:
            printf("LINE_MODIFIED:%d:`%s`\n", message->row, message->text);

            getBoundsOfLine(textViewWithSocket->textBuffer, message->row, &start, &end);
            gtk_text_buffer_delete(textViewWithSocket->textBuffer, &start, &end);
            getBoundsOfLine(textViewWithSocket->textBuffer, message->row, &start, &end);

            gtk_text_buffer_insert(textViewWithSocket->textBuffer, &start, message->text, strlen(message->text));

            break;
        default: break;
    }

    G_UNLOCK(lockParsingIncomingMessage);

    return G_SOURCE_REMOVE;
}

void *incomingMessageListener(void *threadContext) {
    // need to make thread-independent instance of TextViewWithSocket
    struct TextViewWithSocket *textViewWithSocket = g_new0(struct TextViewWithSocket, 1);
    textViewWithSocket = (struct TextViewWithSocket *) threadContext;
    textViewWithSocket->lastReceivedMessage = malloc(sizeof(message_t));

    while (TRUE) {
        g_usleep(3*1000);
        size_t messageSize   = sizeof(message_t);
        char   *socketBuffer = malloc(messageSize);

        if (recv(textViewWithSocket->clientSocket, socketBuffer, messageSize, NULL) != -1) {
            if (!lockParsingIncomingMessage) {
                memcpy(textViewWithSocket->lastReceivedMessage, socketBuffer, messageSize);

                lastCollaborator = textViewWithSocket->lastReceivedMessage->lastCollaborator;
                getCursorStatus(textViewWithSocket->textBuffer);
                gdk_threads_add_idle(resolveIncomingMessage, textViewWithSocket);
            }
        }
    }
}

void *gtkListener() {
    gtk_main();
}

void eventLoops(struct TextViewWithSocket *textViewWithSocket) {
    pthread_t thread[2];

    //starting the thread
    pthread_create(&thread[0], NULL, gtkListener, NULL);
    g_usleep(10*1000);
    pthread_create(&thread[1], NULL, incomingMessageListener, textViewWithSocket);

    //waiting for completion
    pthread_join(thread[0], NULL);
    pthread_join(thread[1], NULL);
}

char *messageToString(message_t *message) {
    size_t size    = sizeof(message_t);
    char   *buffer = malloc(size);
    memset(buffer, 0x00, size);
    memcpy(buffer, message, size);

    return buffer;
}

void sendMessageToServer(message_t *message, int serverSocket) {
    if ((send(serverSocket, messageToString(message), sizeof(message_t), 0)) < 0) {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }
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