#include <gtk/gtk.h>
#include "client.h"

#define min(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

gint *getCursorCords(GtkTextBuffer *buffer, GtkTextIter *iter) {
    static gint cords[2];

    gtk_text_buffer_get_iter_at_mark(buffer, iter, gtk_text_buffer_get_insert(buffer));

    cords[0] = gtk_text_iter_get_line(iter);
    cords[1] = gtk_text_iter_get_line_offset(iter);

    return cords;
}

gchar *getCursorStatus(GtkTextBuffer *buffer) {
    GtkTextIter iter;
    gint        *cords = getCursorCords(buffer, &iter);

    gint row = cords[0];
    gint col = cords[1];

    return g_strdup_printf("%d:%d | Last Collaborator: #%d", col + 1, row + 1, lastCollaborator);
}

/**
 * @return The current cursor line in text buffer AFTER action.
 */
gint postCurrentCursorLine(GtkTextBuffer *buffer) {
    GtkTextIter iter;

    // mark the iter at the current position in the buffer
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));

    return gtk_text_iter_get_line(&iter);
}

void getBoundsOfLine(GtkTextBuffer *buffer, int lineNumber, GtkTextIter *start, GtkTextIter *end) {
    // set start iterator to the very beginning of the line
    gtk_text_buffer_get_iter_at_line(buffer, start, lineNumber);
    // copy value of start to end
    (*end) = (*start);
    // and move it to the end of line
    gtk_text_iter_forward_to_line_end(end);
}

void getBoundsOfCurrentLine(GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end) {
    getBoundsOfLine(buffer, postCurrentCursorLine(buffer), start, end);
}

gchar *getLineText(GtkTextBuffer *buffer, int lineNumber) {
    GtkTextIter start, end;
//    getBoundsOfCurrentLine(buffer, &start, &end);
    getBoundsOfLine(buffer, lineNumber, &start, &end);

    // retrieve message between the start and end
    return gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
}

char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    while (current_pos){
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    return str;
}
/**
 * @return The text of the line lineNumber from GtkTextBuffer *buffer.
 */
message_t *messageFromLineOfTextBuffer(GtkTextBuffer *buffer, enum MessageType type, int lineNumber) {
    message_t *messageToSend = malloc(sizeof(message_t));

    messageToSend->type = type;
    messageToSend->row  = lineNumber;

    char *lineText = type == LINE_REMOVED ? "" : getLineText(buffer, lineNumber);
    if (type == LINE_ADDED && lineText[0] == '\n') lineText = "";
    if (type == LINE_MODIFIED) {
        if (lineText[0] == '\n') {
            strcpy(lineText, " ");
        } else {
            replace_char(lineText, '\n', ' ');
        }
    }

    strcpy(messageToSend->text, lineText);

    return messageToSend;
}

void sendModifiedLinesToServer(GtkTextBuffer *buffer, TextBufferData *data) {
    if (!isServerSendingData) {
        int linesDiff    = postCurrentCursorLine(buffer) - data->currentCursorLine;
        int startingLine = data->currentCursorLine;

        if (linesDiff == 0 && gtk_text_buffer_get_line_count(buffer) < data->totalLines) {
            linesDiff = gtk_text_buffer_get_line_count(buffer) - data->totalLines;
            startingLine = min(postCurrentCursorLine(buffer), data->currentCursorLine) - linesDiff;

            setCurrentCursorLine(buffer, data);
        }

        enum MessageType type;

        int i = 0;
        do {
            type = (i == min(0, linesDiff) /*|| min != max && i == max*/)
                   ? LINE_MODIFIED
                   : (i > 0 ? LINE_ADDED : LINE_REMOVED);

            sendMessageToServer(
                messageFromLineOfTextBuffer(buffer, type, startingLine + i),
                *(data->serverSocket)
            );
            // flag that finished sending data in this loop
            g_usleep(5*1000); //c'mon...
        } while (
            linesDiff != 0
            && (linesDiff > 0 ? (i++) : (i--)) != linesDiff
            );

        message_t message_last;
        message_last.row = -1;
        strcpy(message_last.text, "");
        message_last.type = FINISHED_SENDING_DATA;

        sendMessageToServer(&message_last, *(data->serverSocket));
        setCurrentCursorLine(buffer, data);
    }
}

void updateStatusbar(GtkTextBuffer *buffer, GtkStatusbar *statusbar) {
    gtk_statusbar_pop(statusbar, 0);
    gchar *msg = getCursorStatus(buffer);
    gtk_statusbar_push(statusbar, 0, msg);

    g_free(msg);
}

void setCurrentCursorLine(GtkTextBuffer *buffer, TextBufferData *data) {
    GtkTextIter start, end;
    data->currentCursorLine = (int) getCursorCords(buffer, &start)[0];
    data->totalLines = (int) gtk_text_buffer_get_line_count(buffer);

    updateStatusbar(buffer, GTK_STATUSBAR(data->statusbar));
}

void setCurrentCursorLineHandler(GtkTextBuffer *buffer, const GtkTextIter *new_location, GtkTextMark *mark, TextBufferData *data) {
    setCurrentCursorLine(buffer, data);
}
