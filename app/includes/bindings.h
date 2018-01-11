#include <gtk/gtk.h>
#include "text_buffer_handler.h"

typedef struct BindingData {
    GtkWidget *statusbar;
    int       *serverSocket;
} BindingData;

struct TextViewWithSocket {
    GtkTextView* textView;
    int clientSocket;
};

void killClient() {
    clientIsWorking = FALSE;
}

void onExit(const GtkWidget *window, const GtkToolItem *exit, BindingData *data) {
    g_signal_connect(G_OBJECT(exit), "clicked", G_CALLBACK(disconnectFromServer), data->serverSocket);
    g_signal_connect(G_OBJECT(exit), "clicked", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(exit), "clicked", G_CALLBACK(killClient), NULL);

    g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(disconnectFromServer), data->serverSocket);
    g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(killClient), NULL);
}

void onBufferChanged(GtkTextBuffer *buffer, BindingData *data) {
    g_signal_connect(buffer, "changed", G_CALLBACK(updateStatusbar), data->statusbar);
    g_signal_connect(buffer, "changed", G_CALLBACK(sendCurrentLineToServer), data->serverSocket);
    g_signal_connect_object(buffer, "mark_set", G_CALLBACK(mark_set_callback), data->statusbar, 0);
}

void initStatusBar(GtkTextBuffer *buffer, BindingData *data) {
    updateStatusbar(buffer, GTK_STATUSBAR(data->statusbar));
}

void bindEventListeners(
    GtkWidget *window,
    GtkToolItem *exit,
    GtkTextBuffer *buffer,
    GtkWidget *statusbar,
    int *serverSocket
) {
    BindingData data;
    data.statusbar    = statusbar;
    data.serverSocket = serverSocket;

    onExit(window, exit, &data);
    onBufferChanged(buffer, &data);

    initStatusBar(buffer, &data);
}