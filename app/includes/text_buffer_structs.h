typedef struct TextBufferData {
    GtkTextBuffer *textBuffer;
    GtkWidget     *statusbar;
    int           *serverSocket;
    int           currentCursorLine;
    int           totalLines;
} TextBufferData;

typedef struct WindowsAndConnection {
    GtkEntry *server_ip;
    GtkEntry *server_port;

    GtkWidget *login_window;
    GtkWidget *editor_window;
} WindowsAndConnection;
