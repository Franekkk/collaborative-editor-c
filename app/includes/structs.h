typedef struct Message {
    int type;
    int row;
    char text[200];
} message_t;

typedef enum MessageType {
    LINE_MODIFIED = 1,
    LINE_ADDED = 2,
    LINE_REMOVED = 3,
    FINISHED_SENDING_DATA = 4
} message_type_t;
