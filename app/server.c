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

struct CollaborativeEditorServer {
    struct sockaddr_in *address;

    int *client_sockets;
    int num_of_client_sockets;

    int server_socket;
    char lines[LINES_LIMIT][LINE_MAX_LENGTH];

    fd_set read_fds;
};

void initialize_client_sockets(int* client_sockets, int size) {
    for (int i = 0; i < size; i++) {
        // Assigning zero to all sockets, so we won't have any strange values there
        client_sockets[i] = 0;
    }
}

int create_server_socket() {
    int server_socket = socket(AF_INET, SOCK_STREAM , 0);
    if (server_socket == 0) {
        perror("Error has occcured while creating server socket");
        exit(EXIT_FAILURE);
    }

    //Set master socket to allow multiple connections
    int multiple_conns = TRUE;
    if( setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&multiple_conns, sizeof(multiple_conns)) < 0 )
    {
        perror("Setsockopt has failed");
        exit(EXIT_FAILURE);
    }

    printf("Created server socket: %i\n", server_socket);
    return server_socket;
}

void initialize_address_structure(struct sockaddr_in *address) {
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = inet_addr(HOST);
    address->sin_port = htons(PORT);
}

void bind_server_socket_to_port(int server_socket, struct sockaddr_in address) {
    //bind the socket to localhost port 8888
    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address))<0)
    {
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

void print_text(struct CollaborativeEditorServer *server) {
    printf("Current text:\n");
    for (int i=0; i < LINES_LIMIT; i++) {
        printf("[%d] %s\n", i, server->lines[i]);
    }
}

int reset_fd_set(struct CollaborativeEditorServer *server) {
    //Clear the socket set. On each event loop iteration the fd_set has to
    // be destroyed and re-created
    FD_ZERO(&server->read_fds);

    //Add server socket to fd_set
    FD_SET(server->server_socket, &server->read_fds);
    //Highest file descriptor number, needed for the select function
    int highest_file_descriptor = server->server_socket;

    //Add child sockets to set and determine highest file descriptor
    for (int i = 0; i < server->num_of_client_sockets; i++)
    {
        int socket_descriptor = server->client_sockets[i];
        if (socket_descriptor == 0) {
            // Means that description has not been allocated yet
            // and doesn't need to be added to the fdset we are listening on
            continue;
        }

        FD_SET(socket_descriptor, &server->read_fds);
        if(socket_descriptor > highest_file_descriptor) {
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
        if(server->client_sockets[i] != 0) {
            continue;
        }

        server->client_sockets[i] = new_socket;
        printf("Adding socket: %d to list of sockets at index: %d\n" , new_socket, i);
        break;
    }
}

void send_message(message_t message, int socket) {
    size_t size = sizeof(message_t);
    char* buffer = malloc(size);
    memset(buffer, 0x00, size);
    memcpy(buffer, &message, size);

    send(socket, buffer, size, 0);

    free(buffer);
}

void broadcast_message(struct CollaborativeEditorServer *server, message_t message, int owner_socket_id) {
    for (int i=0; i < MAX_CLIENTS; i++) {
        int client_socket = server->client_sockets[i];
        if (client_socket == 0 || i == owner_socket_id) {
            continue;
        }

        send_message(message, client_socket);
    }
}

void initial_synchronization(struct CollaborativeEditorServer *server, int clientSocket) {
    for (int i=0; i < LINES_LIMIT; i++) {
        message_t message;
        message.row = i;
        strcpy(message.text, server->lines[i]);
        message.type = LINE_MODIFIED;

        send_message(message, clientSocket);
    }
}

void handle_server_socket_activity(struct CollaborativeEditorServer *server) {
    //If something happened on the server socket, its an incoming connection
    if (!FD_ISSET(server->server_socket, &server->read_fds)) {
        return;
    }

    int new_socket, addrlen;
    if ((new_socket = accept(
      server->server_socket, (struct sockaddr *) server->address,
      (socklen_t*)&addrlen)) < 0
    ) {
        perror("Error while accepting client connection");
        return;
    }

    add_new_socket_to_empty_client_slot(server, new_socket);
    initial_synchronization(server, new_socket);
}

void handle_client_socket_activity(
    struct CollaborativeEditorServer *server, int socket_index
) {
    int client_socket = server->client_sockets[socket_index];
    if (!FD_ISSET(client_socket, &server->read_fds)) {
        return;
    }

    int size = sizeof(message_t);
    char* buffer = malloc(size);
    memset(buffer, 0x00, size);

    int valread, addrlen;
    if ((valread = read(client_socket, buffer, size)) == 0) {
        getpeername(client_socket, (struct sockaddr *) server->address, (socklen_t*)&addrlen);
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

    printf("Received message from socket %d at index %d - [row: %d, type: %d, text: %s]\n",
            client_socket, socket_index, received_message.row, received_message.type, received_message.text);

    strcpy(server->lines[received_message.row], received_message.text);
    // print_text(server);

    broadcast_message(server, received_message, socket_index);
}


void handle_client_sockets_activity(struct CollaborativeEditorServer *server) {
    for (int i = 0; i < server->num_of_client_sockets; i++) {
        handle_client_socket_activity(server, i);
    }
}


void event_loop(struct CollaborativeEditorServer *server) {
    while(TRUE) {
        int highest_file_descriptor = reset_fd_set(server);

        int activity = select(
            highest_file_descriptor + 1 , &server->read_fds , NULL , NULL , NULL);

        if (activity < 0) {
            printf("Select error");
        }

        handle_server_socket_activity(server);
        handle_client_sockets_activity(server);
    }
}

void initialize_text_content(struct CollaborativeEditorServer *server) {
    for (int i=0; i < LINES_LIMIT; i++) {
        memset(server->lines[i], ' ', LINE_MAX_LENGTH);
        server->lines[i][LINE_MAX_LENGTH - 1] = '\0';
    }
}

int main(int argc , char *argv[])
{
    struct sockaddr_in address;
    initialize_address_structure(&address);

    int client_sockets[MAX_CLIENTS];
    initialize_client_sockets(client_sockets, MAX_CLIENTS);

    struct CollaborativeEditorServer server;
    server.server_socket = create_server_socket();
    server.address = &address;
    server.client_sockets = client_sockets;
    server.num_of_client_sockets = MAX_CLIENTS;

    initialize_text_content(&server);

    bind_server_socket_to_port(server.server_socket, address);
    listen_on_socket(server.server_socket);

    event_loop(&server);

    return 0;
}
