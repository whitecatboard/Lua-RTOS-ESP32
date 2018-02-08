#include "hex.h"

int lcheck_hex_str(const char *str) {
    while (*str) {
        if (((*str < '0') || (*str > '9')) && ((*str < 'A') || (*str > 'F'))) {
            return 0;
        }

        str++;
    }

    return 1;
}
