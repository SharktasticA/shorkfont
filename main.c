/*
    ######################################################
    ##             SHORK UTILITY - SHORKFONT            ##
    ######################################################
    ## A utility for SHORK 486 that changes the         ##
    ## terminal's foreground (text) colour              ##
    ######################################################
    ## Licence: GNU GENERAL PUBLIC LICENSE Version 3    ##
    ######################################################
    ## Kali (sharktastica.co.uk)                        ##
    ######################################################
*/



#include <ctype.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>



typedef struct {
    char *name;
    char *ascii;
} Colour;

typedef struct {
    Colour colour;
    char *font;
} Config;



static const Colour COLOURS[] = {
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
static const char *COLOURS_STR = "blue | blue_bright | cyan | cyan_bright | green | green_bright | grey | magenta | magenta_bright | red | red_bright | white | white_bright | yellow | yellow_bright";
static Config CONFIG;
static const char *DOT_CONF = "/etc/shorkfont.conf";
static const char *FONT_DIR = "/usr/share/consolefonts";
static struct winsize TERM_SIZE;



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
 * Apply the selected font
 * @param font Selected font's path
 */
void applyFont(char *font)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "setfont %s", font);
    system(cmd);
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
 * Loads the current values from DOT_CONF.
 */
Config loadConf(void)
{
    Config config;

    // Default/fallback values that mirror the initial DOT_CONF provided by SHORK 486
    config.colour.name = "white";
    config.colour.ascii = "0;37";
    config.font = "default";

    FILE *stream = fopen(DOT_CONF, "r");
    if (stream)
    {
        char line[256];

        while (fgets(line, sizeof(line), stream))
        {
            line[strcspn(line, "\r\n")] = '\0';

            char *value = strchr(line, '=');
            if (!value) continue;

            value++;

            if (*value == '"')
            {
                value++;
                char *end = strrchr(value, '"');
                if (end) *end = '\0';
            }

            if (strncmp(line, "NAME=", 5) == 0)
            {
                config.colour.name = strdup(value);
            }
            else if (strncmp(line, "ASCII=", 6) == 0)
            {
                config.colour.ascii = strdup(value);
            }
            else if (strncmp(line, "FONT=", 5) == 0)
            {
                config.font = strdup(value);
            }
        }

        fclose(stream);
    }

    return config;
}

void showArgumentsList(void)
{
    char cmdDesc[64] = "Changes the console's foreground colour or font.";
    formatNewLines(cmdDesc, TERM_SIZE.ws_col);
    printf("%s\n\n", cmdDesc);

    char usage1[256];
    snprintf(usage1, sizeof(usage1), "Usage: shorkfont [-c|--colour] {%s}", COLOURS_STR);
    formatNewLines(usage1, TERM_SIZE.ws_col);
    printf("%s\n", usage1);

    char usage2[72];
    snprintf(usage2, sizeof(usage2), "       shorkfont [-f|--font] {font_name | font_path | default}");
    formatNewLines(usage2, TERM_SIZE.ws_col);
    printf("%s\n\n", usage2);

    printf("Current colour: %s\n", CONFIG.colour.name);

    char currFont[256];
    snprintf(currFont, sizeof(currFont), "Current font: %s", CONFIG.font);
    formatNewLines(currFont, TERM_SIZE.ws_col);
    printf("%s\n", currFont);
}

/**
 * Validates if an inputted string is in the shorkfont colour palette.
 * @param input Input string to validate for colour
 * @returns ASCII escape code for the inputted colour if valid; NULL if invalid
 */
char *validateColour(char *input)
{
    size_t count = sizeof(COLOURS) / sizeof(COLOURS[0]);
    for (size_t i = 0; i < count; i++)
        if (strcmp(input, COLOURS[i].name) == 0)
            return (char*)COLOURS[i].ascii;

    char err[480];
    snprintf(err, sizeof(err), "Invalid colour \"%s\". Available options: %s\n", input, COLOURS_STR);
    formatNewLines(err, TERM_SIZE.ws_col);
    printf("%s", err);
    return NULL;
}

/**
 * Validates if an inputted string is an existing console font file.
 * @param input Input string to validate for font
 * @returns Path for the inputted font if valid; NULL if invalid
 */
char *validateFont(char *input)
{
    // Assume font is in FONT_DIR
    if (!strstr(input, "/") && !strstr(input, "."))
    {
        static char fontPath[480];
        snprintf(fontPath, sizeof(fontPath), "%s/%s.psf", FONT_DIR, input);
        if (access(fontPath, F_OK) == 0)
            return fontPath;
    }
    // Assume full font path
    else
    {
        size_t inputLen = strlen(input);
        const char *ext = ".psf";
        size_t extLen = strlen(ext);

        if (inputLen >= extLen)
        {
            const char *end = input + inputLen - extLen;

            int extValid = 1;
            for (size_t i = 0; i < extLen; i++)
            {
                if (tolower((unsigned char)end[i]) != ext[i])
                {
                    extValid = 0;
                    break;
                }
            }

            if (extValid)
                if (access(input, F_OK) == 0)
                    return input;
        }
    }

    char err[480];
    snprintf(err, sizeof(err), "Invalid font \"%s\". Please ensure it is a PC Screen Font (.psf) file, and that the full path to the font is provided or the font name exists in \"%s\".\n", input, FONT_DIR);
    formatNewLines(err, TERM_SIZE.ws_col);
    printf("%s", err);
    return NULL;
}

/**
 * Writes new values to a .conf file.
 * @param name New colour name
 * @param ascii New colour ASCII value
 * @param font New font
 */
void writeConf(char *name, char *ascii, char *font)
{
    FILE *stream = fopen(DOT_CONF, "w");
    if (!stream)
    {
        perror("ERROR: failed to create or open /etc/shorkfont.conf");
        return;
    }
    fprintf(stream, "NAME=\"%s\"\n", name);
    fprintf(stream, "ASCII=\"%s\"\n", ascii);
    fprintf(stream, "FONT=\"%s\"\n", font);
    fclose(stream);
}



int main(int argc, char *argv[])
{
    TERM_SIZE = getTerminalSize();
    CONFIG = loadConf();

    if (argc != 3) showArgumentsList();
    else
    {
        if (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--colour") == 0)
        {
            char *ascii = validateColour(argv[2]);
            if (ascii)
            {
                writeConf(argv[2], ascii, CONFIG.font);
                applyColour(ascii);
                killParentTerminal();
                printf("\033[%smApplied colour: %s\033[0m\n", ascii, argv[2]);
                return 0;
            }
        }
        else if (strcmp(argv[1], "-f") == 0 || strcmp(argv[1], "--font") == 0)
        {
            if (strcmp(argv[2], "default") == 0)
            {
                writeConf(CONFIG.colour.name, CONFIG.colour.ascii, "default");

                char out[160];
                snprintf(out, sizeof(out), "Applied font: default. If a font other than \"default\" was previously applied, you must restart your computer before this change will take effect.\n");
                formatNewLines(out, TERM_SIZE.ws_col);
                printf("%s", out);

                return 1;
            }
            else
            {
                char *font = validateFont(argv[2]);
                if (font)
                {
                    writeConf(CONFIG.colour.name, CONFIG.colour.ascii, font);
                    applyFont(font);
                    printf("Applied font: %s\n", font);
                    return 1;
                }
            }
        }
        else showArgumentsList();
    }

    return 1;
}
