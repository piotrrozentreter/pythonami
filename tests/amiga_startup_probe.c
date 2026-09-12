#include <proto/dos.h>

static void write_text(const char *text)
{
    LONG length = 0;
    while (text[length] != '\0') {
        ++length;
    }
    Write(Output(), (CONST_APTR)text, length);
}

int main(int argc, char **argv)
{
    write_text("pythonami startup probe\n");
    if (argc == 0) {
        write_text("argc=0\n");
    } else {
        write_text("argc>0\n");
    }
    return 0;
}
