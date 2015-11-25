#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strings.h"
#include "escape.h"

// Parse a json twitter timeline
// Find a status and do something with it (return 1) or just skip (0)
int findStatus(const char *buffer, int (*statusAction)(char* sender, char* status), void *context)
{
    static char sender[32+1];
    static char status[256+1];
    static char status_decoded[256+1];

    char* string_start;
    char* string_stop;
    size_t string_length;

    string_start = strstr(buffer, TWTR_TEXT);
    if (NULL != string_start) {
        string_start += sizeof(TWTR_TEXT) - 1;
        string_stop = strstr(string_start, TWTR_COMMA);
        if (NULL != string_stop) {
            string_length = string_stop - string_start;
            strncpy(status, string_start, string_length);
            *(status + string_length) = '\0';
            // Make sure to not find this one again (when copied from inputbuf[1] to inputbuf[0])
            *(string_start - 1) = 0x20; // ' '
        }
    }

    string_start = strstr(buffer, TWTR_SCREEN_NAME);
    if (NULL != string_start) {
        string_start += sizeof(TWTR_SCREEN_NAME) - 1;
        string_stop = strstr(string_start, TWTR_COMMA);
        if (NULL != string_stop) {
            string_length = string_stop - string_start;
            strncpy(sender, string_start, string_length);
            *(sender + string_length) = '\0';
            // Make sure to not find this one again (when copied from inputbuf[1] to inputbuf[0])
            *(string_start - 1) = 0x20; // ' '

            decode_escape(status, status_decoded);
            if (statusAction) {
                return statusAction(sender, status_decoded);
            }
        }
    }
    return 1;
}
