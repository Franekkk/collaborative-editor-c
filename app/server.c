//Example code: A simple server side code, which echos back the received message.
//Handle multiple socket connections with select and fd_set on Linux
#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include "includes/common.h"
#include "includes/text_handler.h"

struct CollaborativeEditorServer {
    struct sockaddr_in *address;

    int *client_sockets;
    int num_of_client_sockets;

    int  server_socket;
    Text *text;
//    char lines[LINES_LIMIT][LINE_MAX_LENGTH];

    fd_set readingFileDescriptors;
};

void initialize_client_sockets(int *client_sockets, int size) {
    for (int i = 0; i < size; i++) {
        // Assigning zero to all sockets, so we won't have any strange values there
        client_sockets[i] = 0;
    }
}

int create_server_socket() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == 0) {
        perror("Error has occcured while creating server socket");
        exit(EXIT_FAILURE);
    }

    //Set master socket to allow multiple connections
    int multiple_conns = TRUE;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &multiple_conns, sizeof(multiple_conns)) < 0) {
        perror("Setsockopt has failed");
        exit(EXIT_FAILURE);
    }

    printf("Created server socket: %i\n", server_socket);
    return server_socket;
}

void initialize_address_structure(struct sockaddr_in *address) {
    address->sin_family      = AF_INET;
    address->sin_addr.s_addr = inet_addr(HOST);
    address->sin_port        = htons(PORT);
}

void bind_server_socket_to_port(int server_socket, struct sockaddr_in address) {
    //bind the socket to localhost port 8888
    if (bind(server_socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port %d... \n", PORT);
}

void listen_on_socket(int master_socket) {
    if (listen(master_socket, MAX_CLIENTS) < 0) {
        perror("Error while listening");
        exit(EXIT_FAILURE);
    }
}

int reset_fd_set(struct CollaborativeEditorServer *server) {
    //Clear the socket set. On each event loop iteration the fd_set has to
    // be destroyed and re-created
    FD_ZERO(&server->readingFileDescriptors);

    //Add server socket to fd_set
    FD_SET(server->server_socket, &server->readingFileDescriptors);
    //Highest file descriptor number, needed for the select function
    int highest_file_descriptor = server->server_socket;

    //Add child sockets to set and determine highest file descriptor
    for (int i = 0; i < server->num_of_client_sockets; i++) {
        int socket_descriptor = server->client_sockets[i];
        if (socket_descriptor == 0) {
            // Means that description has not been allocated yet
            // and doesn't need to be added to the fdset we are listening on
            continue;
        }

        FD_SET(socket_descriptor, &server->readingFileDescriptors);
        if (socket_descriptor > highest_file_descriptor) {
            highest_file_descriptor = socket_descriptor;
        }
    }

    return highest_file_descriptor;
}

void add_new_socket_to_empty_client_slot(
    struct CollaborativeEditorServer *server,
    int new_socket
) {
    for (int i = 0; i < server->num_of_client_sockets; i++) {
        if (server->client_sockets[i] != 0) {
            continue;
        }

        server->client_sockets[i] = new_socket;
        printf("Adding socket: %d to list of sockets at index: %d\n", new_socket, i);
        break;
    }
}

void send_message(message_t message, int socket) {
    size_t size = sizeof(message_t);
    char *buffer = malloc(size);
    memset(buffer, 0x00, size);
    memcpy(buffer, &message, size);

    send(socket, buffer, size, 0);

    free(buffer);
}

void broadcast_message(struct CollaborativeEditorServer *server, message_t message, int owner_socket_id) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        int client_socket = server->client_sockets[i];
        if (client_socket == 0 || i == owner_socket_id) {
            continue;
        }

        send_message(message, client_socket);
    }
}

void initial_synchronization(struct CollaborativeEditorServer *server, int clientSocket) {
    for (int i = 0; i < *server->text->linesNumber; i++) {
        message_t message;
        message.row = i;
        strcpy(message.text, server->text->lines[i]);
        message.type = i == 0 ? LINE_MODIFIED : LINE_ADDED;

        message.lastCollaborator = clientSocket;
        send_message(message, clientSocket);
    }

//     flag that server finished sending data in this loop
    message_t message_last;
    message_last.row = -1;
    strcpy(message_last.text, "");
    message_last.type = FINISHED_SENDING_DATA;

    message_last.lastCollaborator = clientSocket;
    send_message(message_last, clientSocket);
}

void handle_server_socket_activity(struct CollaborativeEditorServer *server) {
    //If something happened on the server socket, its an incoming connection
    if (!FD_ISSET(server->server_socket, &server->readingFileDescriptors)) {
        return;
    }

    int new_socket, addrlen;
    if ((new_socket = accept(
        server->server_socket, (struct sockaddr *) server->address,
        (socklen_t *) &addrlen)) < 0
        ) {
        perror("Error while accepting client connection");
        return;
    }

    add_new_socket_to_empty_client_slot(server, new_socket);
    initial_synchronization(server, new_socket);
}

void resolveIncomingMessageFromClient(const struct CollaborativeEditorServer *server, message_t *received_message) {
    switch (received_message->type) {
        case LINE_ADDED: textAddNewLine(server->text, received_message->row, received_message->text); break;
        case LINE_REMOVED: textRemoveLine(server->text, received_message->row); break;
        case LINE_MODIFIED: textModifyLine(server->text, received_message->row, received_message->text); break;
        default: break;
    }
}

void handle_client_socket_activity(
    struct CollaborativeEditorServer *server, int socket_index
) {
    int client_socket = server->client_sockets[socket_index];
    if (!FD_ISSET(client_socket, &server->readingFileDescriptors)) {
        return;
    }

    int size = sizeof(message_t);
    char *buffer = malloc(size);
    memset(buffer, 0x00, size);

    int valread, addrlen;
    if ((valread = read(client_socket, buffer, size)) == 0) {
        getpeername(client_socket, (struct sockaddr *) server->address, (socklen_t *) &addrlen);
        close(client_socket);
        server->client_sockets[socket_index] = 0;
        printf("Disconnected socket %d at index %d\n", client_socket, socket_index);
        return;
    }

    //set the string terminating NULL byte on the end
    //of the data read
    message_t received_message;
    memcpy(&received_message, buffer, size);
    free(buffer);

    // after connecting, client sends some trash and type is 0
    if (received_message.type == 0) return;

    printf("Received message from socket %d at index %d - [row: %d, type: %d, text: %s]\n",
           client_socket, socket_index, received_message.row, received_message.type, received_message.text);

    resolveIncomingMessageFromClient(server, &received_message);

    // print_text(server);
    received_message.lastCollaborator = client_socket;
    broadcast_message(server, received_message, socket_index);
}


void handle_client_sockets_activity(struct CollaborativeEditorServer *server) {
    for (int i = 0; i < server->num_of_client_sockets; i++) {
        handle_client_socket_activity(server, i);
    }
}


void event_loop(struct CollaborativeEditorServer *server) {
    while (TRUE) {
        int highest_file_descriptor = reset_fd_set(server);

        int numberOfReadyDescriptors = select(
//            highest_file_descriptor + 1 , &server->readingFileDescriptors , NULL , NULL , NULL
            server->num_of_client_sockets + 1, &server->readingFileDescriptors, NULL, NULL, NULL
        );

        if (numberOfReadyDescriptors < 0) {
            printf("Select error");
        }

        handle_server_socket_activity(server);
        handle_client_sockets_activity(server);
    }
}

void initialize_text_content(struct CollaborativeEditorServer *server) {
    server->text = textInit();

    textModifyLine(server->text, 0, "0. .");
    textAddNewLine(server->text, 1, "1. Lorem Ipsum jest tekstem stosowanym jako przykładowy");
    textAddNewLine(server->text, 2, "2. wypełniacz w przemyśle poligraficznym. Został po raz");
    textAddNewLine(server->text, 3, "3. pierwszy użyty w XV w. przez nieznanego drukarza do");
    textAddNewLine(server->text, 4, "4. wypełnienia tekstem próbnej książki. Pięć wieków");
    textAddNewLine(server->text, 5, "5. później zaczął być używany przemyśle elektronicznym,");
    textAddNewLine(server->text, 6, "6. pozostając praktycznie niezmienionym. Spopularyzował");
    textAddNewLine(server->text, 7, "7. się w latach 60. XX w. wraz z publikacją arkuszy");
    textAddNewLine(server->text, 8, "8. Letrasetu, zawierających fragmenty Lorem Ipsum, a");
    textAddNewLine(server->text, 9, "9. ostatnio z zawierającym różne wersje Lorem Ipsum oprogramowaniem");
}

int main(int argc, char *argv[]) {
    struct sockaddr_in address;
    initialize_address_structure(&address);

    int client_sockets[MAX_CLIENTS];
    initialize_client_sockets(client_sockets, MAX_CLIENTS);

    struct CollaborativeEditorServer server;
    server.server_socket         = create_server_socket();
    server.address               = &address;
    server.client_sockets        = client_sockets;
    server.num_of_client_sockets = MAX_CLIENTS;

    initialize_text_content(&server);

    bind_server_socket_to_port(server.server_socket, address);
    listen_on_socket(server.server_socket);

    event_loop(&server);

    return 0;
}