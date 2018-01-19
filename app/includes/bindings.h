#include <gtk/gtk.h>
#include "text_buffer_handler.h"

struct TextViewWithSocket {
    GtkTextBuffer* textBuffer;
    TextBufferData* bufferData;
    int clientSocket;

    message_t * lastReceivedMessage;
};

void killClient() {
    clientIsWorking = FALSE;
}

void bindOnExit(const GtkWidget *window, const GtkToolItem *exit, TextBufferData *data) {
    g_signal_connect(G_OBJECT(exit), "clicked", G_CALLBACK(disconnectFromServer), data->serverSocket);
    g_signal_connect(G_OBJECT(exit), "clicked", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(exit), "clicked", G_CALLBACK(killClient), NULL);

    g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(disconnectFromServer), data->serverSocket);
    g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(killClient), NULL);
}

void bindOnChangeSendModifiedLinesToServer(GtkTextBuffer *buffer,TextBufferData *data) {
    onChangeSignalId = g_signal_connect(buffer, "changed", G_CALLBACK(sendModifiedLinesToServer), data);
}

void unbindOnChangeSendModifiedLinesToServer(GtkTextBuffer *buffer) {
    g_signal_handler_disconnect(buffer, onChangeSignalId);
    onChangeSignalId = NULL;
}

void bindOnBufferChanged(GtkTextBuffer *buffer, TextBufferData *data) {

    g_signal_connect(buffer, "mark_set", G_CALLBACK(setCurrentCursorLineHandler), data);
    bindOnChangeSendModifiedLinesToServer(buffer, data);
//    g_signal_connect_object(buffer, "mark_set", G_CALLBACK(mark_set_callback), data->statusbar, 0);
}

void initStatusBar(GtkTextBuffer *buffer, TextBufferData *data) {
    updateStatusbar(buffer, GTK_STATUSBAR(data->statusbar));
}

void *on_client_connect(GtkButton * button, WindowsAndConnection * windowsAndConnection) {
    printf("Connecting to ip: %s, port: %s",
           gtk_entry_get_text(windowsAndConnection->server_ip),
           gtk_entry_get_text(windowsAndConnection->server_port));

    gtk_widget_hide(windowsAndConnection->login_window);
    gtk_main_quit();
}

void bindEventListeners(
    GtkWidget *window,
    GtkToolItem *exit,
    GtkTextBuffer *buffer,
    TextBufferData *data
) {

    bindOnExit(window, exit, data);
    bindOnBufferChanged(buffer, data);

    initStatusBar(buffer, data);
}