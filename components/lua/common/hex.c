#include "hex.h"

#include <string.h>

int lcheck_hex_str(const char *str) {
    if (strlen(str) % 2 != 0) {
        return 0;
    }

    while (*str) {
        if (((*str < '0') || (*str > '9')) && ((*str < 'A') || (*str > 'F')) && ((*str < 'a') || (*str > 'f'))) {
            return 0;
        }

        str++;
    }

    return 1;
}
