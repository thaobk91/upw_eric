
#ifndef __APP_UTILS_H
#define __APP_UTILS_H

#include "common.h"

#define FUNC_CHAR_TO_NUM(x)             (x ^ 0x30)
#define FUNC_2_CHAR_TO_INT(x, y)        (((int)(x ^ 0x30) * 10) + (y ^ 0x30))

/**
 * @brief find a string in a array/buffer
 * 
 * @param buf input array
 * @param buf_len length of array
 * @param str string to compare
 * 
 * @retval NULL if not exist
 * @retval start position of string
 */
uint8_t *appUtils_find_string(uint8_t *buf, uint16_t buf_len, char *str);

/**
 * @brief get next string after a character
 * 
 * @param buf input string
 * @param c character to detect starting of string
 * 
 * @retval NULL if not exist
 * @retval string if exist
 */
char *appUtils_getString_afterChar(char *buf, char c);

/**
 * @brief convert a unsigned char to binary string (0, 1)
 * 
 * @param value unsigned char variable input
 * @param str binary string output
 * 
 * @return None
 */
void appUtils_convertU8ToBinaryString(uint8_t value, char str[8]);

/**
 * @brief convert a array to hex string
 * 
 * @param data array input
 * @param len length of array
 * @param buf buffer output of hex string
 * 
 * @return None
 */
void appUtils_convertHexToString(uint8_t *data, uint8_t len, char *buf);

/**
 * @brief convert a array to u32 number
 * 
 * @param data array input (maximum 4 bytes)
 * @param len length of array
 * 
 * @return u32 number
 */
uint32_t appUtils_convertHexStringToU32(char *data, uint8_t len);

/**
 * @brief get a string from a string started at "from" character to "to" character
 * 
 * @param str string input
 * @param from begin character. if 0 then output string will start at begin of input string
 * @param to end character.
 * @param out string output
 * @param out_size size of output buffer
 * @param out_len length of output buffer
 * 
 * @retval NULL if error or next string is NULL
 * @retval next string after "to" character
 */
char *appUtils_getStringFromTo(char *str, char from,
        char to, char *out, uint16_t out_size, uint16_t *out_len);

/**
 * @brief check if a string is number or not
 * 
 * @param buf input string
 * @param len length of string
 * 
 * @retval true if string is a number
 * @retval false if not number
 */
bool appUtils_isNumber(char *buf, int len);

/**
 * @brief replace "\n" string in a string to character '\n'
 * 
 * @param data input string
 * @param data_len length of input string
 * @param out output buffer
 * 
 * @return length of new string
 */
int appUtils_replace_newline(char *data, int data_len, char *out);

#endif // __APP_UTILS_H