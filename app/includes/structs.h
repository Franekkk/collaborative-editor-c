typedef struct Message {
    int type;
    int row;
    char text[255];
    int lastCollaborator;
} message_t;

typedef enum MessageType {
    LINE_MODIFIED = 1,
    LINE_ADDED = 2,
    LINE_REMOVED = 3,
    FINISHED_SENDING_DATA = 4
} message_type_t;