
/*
 * dev_math.c
 *
 * This file will handle math functions
 * 
 */

#include "dev_math.h"


/**
 * @brief getting substring from string with a character. it will return
 *      closest position that the character appears first time
 * 
 * @param buf: input buffer
 * @param c: character to compare
 * 
 * @retval a string where 'c' character appears
 */
char *dev_math_getNextStringByChar(char *buf, char c)
{
    char *p = buf;

    while ((*p != 0) && (*p != c)) {
        p++;
    }
    if (*p == c) {
        p++;
        return p;
    }
    return NULL;
}