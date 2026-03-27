
#ifndef __DEV_MATH_H
#define __DEV_MATH_H

#include "app_cfg.h"

/**
 * @brief getting substring from string with a character. it will return
 *      closest position that the character appears first time
 * 
 * @param buf: input buffer
 * @param c: character to compare
 * 
 * @retval a string where 'c' character appears
 */
char *dev_math_getNextStringByChar(char *buf, char c);

#endif // __DEV_MATH_H