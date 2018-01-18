typedef struct Text {
    char         **lines;
    unsigned int *linesNumber;
} Text;

void textModifyLine(Text *text, int row, char *line) {
    char **text2 = malloc(*text->linesNumber * sizeof(char *));

    for (int i = 0; i < *text->linesNumber; i++) {
        if (i == row) {
            text2[i] = malloc((strlen(line) + 1) * sizeof(char));
            strcpy(text2[i], line);

            // cannot free 1st address
            if (i > 0) free(text->lines[i]);
        } else {
            text2[i] = text->lines[i];
        }
    }

    free(text->lines);

    text->lines = text2;
}

void textAddNewLine(Text *text, int row, char *line) {
    char **text2 = malloc(++*text->linesNumber * sizeof(char *));

    for (int i = 0; i < *text->linesNumber; i++) {
        if (i < row) {
            text2[i] = text->lines[i];
        } else if (i == row) {
            text2[i] = malloc((strlen(line) + 1) * sizeof(char));
            strcpy(text2[i], line);
        } else {
            text2[i] = text->lines[i - 1];
        }
    }

    free(text->lines);

    text->lines = text2;
}

void textRemoveLine(Text *text, int row) {
    char **text2 = malloc((*text->linesNumber)-- * sizeof(char *));

    for (int i = 0; i < *text->linesNumber + 1; i++) {
        if (i != *text->linesNumber) text2[i] = text->lines[i < row ? i : (i + 1)];
        if (i == row) free(text->lines[i]);
    }

    free(text->lines);

    text->lines = text2;
}

Text *textInit() {
    Text *text = malloc(sizeof(Text));

    text->lines       = malloc(1 * sizeof(char[1]));
    text->linesNumber = malloc(sizeof(int));

    *text->linesNumber = 1;
    text->lines[0] = "1";

    return text;
}

int test(int argc, char *argv[]) {

    Text *text = textInit();

    textModifyLine(text, 0, "Pierwsza linia");
    textAddNewLine(text, 1, "druga linia");
    textAddNewLine(text, 2, "trzecia linia");
    textAddNewLine(text, 3, "czwarta linia");
    textAddNewLine(text, 4, "piąta linia");
    textRemoveLine(text, 4);
    textRemoveLine(text, 3);
    textRemoveLine(text, 2);
    textAddNewLine(text, 2, "czwarta linia v2");
    textAddNewLine(text, 3, "piąta linia");
    textAddNewLine(text, 4, "szósta linia");
    textModifyLine(text, 2, "trzecia?");
    textRemoveLine(text, 3);

    return 0;
}