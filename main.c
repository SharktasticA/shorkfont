/*
    ######################################################
    ##             SHORK UTILITY - SHORKCOL             ##
    ######################################################
    ## A utility for SHORK 486 that changes the         ##
    ## terminal's foreground (text) colour              ##
    ######################################################
    ## Licence: GNU GENERAL PUBLIC LICENSE Version 3    ##
    ######################################################
    ## Kali (sharktastica.co.uk)                        ##
    ######################################################
*/



#include <sys/ioctl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>



typedef struct {
    const char *name;
    const char *ascii;
} Colour;



static const Colour colourTable[] = {
    {"blue",           "0;34"},
    {"blue_bright",    "0;1;34"},
    {"cyan",           "0;36"},
    {"cyan_bright",    "0;1;36"},
    {"green",          "0;32"},
    {"green_bright",   "0;1;32"},
    {"grey",           "0;1;30"},
    {"magenta",        "0;35"},
    {"magenta_bright", "0;1;35"},
    {"red",            "0;31"},
    {"red_bright",     "0;1;31"},
    {"white",          "0;37"},
    {"white_bright",   "0;1;37"},
    {"yellow",         "0;33"},
    {"yellow_bright",  "0;1;33"}
};

static const char *colours = "blue | blue_bright | cyan | cyan_bright | green | green_bright | grey | magenta | magenta_bright | red | red_bright | white | white_bright | yellow | yellow_bright";
static struct winsize termSize;



/**
 * Apply the selected colour to core system files
 * @param ascii Selected colour's ASCII value
 */
void applyColour(char *ascii)
{
    char cmd[256];

    // Write to /etc/profile
    snprintf(cmd, sizeof(cmd), "sed -i 's|\\\\033\\[[0-9;]*m|\\\\033[%sm|g' /etc/profile", ascii);
    system(cmd);

    // Write to etc/init.d/rc
    snprintf(cmd, sizeof(cmd), "sed -i 's|\\\\033\\[[0-9;]*m|\\\\033[%sm|g' /etc/init.d/rc", ascii);
    system(cmd);

    // Write to terminfo.src
    snprintf(cmd, sizeof(cmd), "sed -i 's|\\\\E\\[[0-9;]*m|\\\\E[%sm|g' /usr/share/terminfo/src/terminfo.src", ascii);
    system(cmd);

    // Rebuild terminfo
    system("tic -x -1 -o /usr/share/terminfo /usr/share/terminfo/src/terminfo.src");
}

/**
 * Adds new lines to a given string based on the requested line width.
 * @param buffer Input string
 * @param width Characters per line
 * @return Number of lines in the string
 */
int formatNewLines(char *buffer, int width)
{
    if (!buffer || width < 1) return 0;

    size_t bufferStrLen = strlen(buffer);
    int lines = 1;
    int lastSpace = -1;
    int widthCount = 1;
    for (int i = 0; i < bufferStrLen; i++)
    {
        if (buffer[i] == '\033')
        {
            while (i < bufferStrLen && buffer[i] != 'm')
            {
                i++;
            }
            if (i >= bufferStrLen) break;
            continue; 
        }
        
        if (buffer[i] == ' ') lastSpace = i;
        else if (buffer[i] == '\n')
        {
            lines++;
            widthCount = 0;
        }

        if (widthCount == width)
        {
            if (lastSpace != -1) buffer[lastSpace] = '\n';
            widthCount = i - lastSpace;
            lines++;
        }

        widthCount++;
    }

    return lines;
}

/**
 * Gets the current colour from shorkcol.conf.
 * @returns Name of the current colour
 */
char *getCurrentColour(void)
{
    FILE *stream = fopen("/etc/shorkcol.conf", "r");
    if (!stream) return strdup("white");

    static char line[256];
    while (fgets(line, sizeof(line), stream))
    {
        if (strncmp(line, "NAME=", 5) == 0)
        {
            char *name = line + 6;
            name[strcspn(name, "\r\n")] = '\0';
            name[strlen(name) - 1] = '\0';
            fclose(stream);
            return strdup(name);
        }
    }

    fclose(stream);
    return strdup("white");
}

/**
 * @return winsize struct containing the current terminal size in columns and rows
 */
struct winsize getTerminalSize(void)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        ws.ws_col = 80;
        ws.ws_row = 24;
    }
    return ws;
}

/**
 * Kills the parent terminal's process so the new colour can take affect.
 */
void killParentTerminal(void)
{
    pid_t ppid = getppid();
    if (kill(ppid, SIGTERM) == -1)
        perror("ERROR: failed to kill parent terminal");
}

/**
 * @param name Selected colour's name
 * @param ascii Selected colour's ASCII value
 */
void printAppliedColour(const char *name, const char *ascii)
{
    printf("\033[%smApplied colour: %s\033[0m\n", ascii, name);
}

void showArgumentsList(void)
{
    char cmdDesc[90] = "Changes the terminal's foreground (font) colour.\n\n";
    formatNewLines(cmdDesc, termSize.ws_col);
    printf("%s\n", cmdDesc);

    char usage[200];
    snprintf(usage, 200, "Usage: shorkcol {%s}\n", colours);
    formatNewLines(usage, termSize.ws_col);
    printf("%s", usage);

    char currCol[60];
    snprintf(currCol, 60, "Current colour: %s\n", getCurrentColour());
    formatNewLines(currCol, termSize.ws_col);
    printf("%s", currCol);
}

/**
 * Validates if an inputted string is in the shorkcol colour palette.
 * @param input Input string to validate for colour
 * @returns ASCII escape code for the inputted colour if valid; NULL if invalid
 */
char *validateColour(char *input)
{
    size_t count = sizeof(colourTable) / sizeof(colourTable[0]);
    for (size_t i = 0; i < count; i++)
        if (strcmp(input, colourTable[i].name) == 0)
            return (char*)colourTable[i].ascii;

    printf("Invalid colour: %s\n", input);
    char avail[200];
    snprintf(avail, sizeof(avail), "Available colours: %s\n", colours);
    formatNewLines(avail, termSize.ws_col);
    printf("%s", avail);
    return NULL;
}

/**
 * Writes selected colour's new values to a .conf file.
 * @param name Selected colour's name
 * @param ascii Selected colour's ASCII value
 */
void writeConf(char *name, char *ascii)
{
    FILE *stream = fopen("/etc/shorkcol.conf", "w");
    if (!stream)
    {
        perror("ERROR: failed to create or open /etc/shorkcol.conf");
        return;
    }
    fprintf(stream, "NAME=\"%s\"\n", name);
    fprintf(stream, "ASCII=\"%s\"\n", ascii);
    fclose(stream);
}



int main(int argc, char *argv[])
{
    termSize = getTerminalSize();

    if (argc == 1 || argc > 2) showArgumentsList();
    else
    {
        char *ascii = validateColour(argv[1]);
        if (ascii)
        {
            writeConf(argv[1], ascii);
            applyColour(ascii);
            killParentTerminal();
            printAppliedColour(argv[1], ascii);
            return 0;
        }
    }

    return 1;
}
