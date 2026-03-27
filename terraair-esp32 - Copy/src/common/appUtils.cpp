
#include "appUtils.h"
#include "dev_mem.h"

static char g_hex_str[] = "123456789ABCDEF";

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
uint8_t *appUtils_find_string(uint8_t *buf, uint16_t buf_len, char *str)
{
	uint8_t i = 0;
	int len = buf_len;
	uint8_t *p = buf;
	uint8_t *s = NULL;

	while (len > 0) {
		if (*p == str[i]) {
			if (s == NULL) {
				s = p;
			}
			i++;
			if (i >= strlen(str)) {
				break;
			}
		} else {
			i = 0;
			s = NULL;
		}
		p++;
		len--;
	}

	if (i < strlen(str)) {
		return NULL;
	}
	return s;
}


/**
 * @brief get next string after a character
 * 
 * @param buf input string
 * @param c character to detect starting of string
 * 
 * @retval NULL if not exist
 * @retval string if exist
 */
char *appUtils_getString_afterChar(char *buf, char c)
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


/**
 * @brief convert a unsigned char to binary string (0, 1)
 * 
 * @param value unsigned char variable input
 * @param str binary string output
 * 
 * @return None
 */
void appUtils_convertU8ToBinaryString(uint8_t value, char str[8])
{
    uint8_t i = 0;
    for (i = 0; i < 8; i++) {
        str[i] = ((value >> (7 - i)) & 0x01) | 0x30;
    }
}


/**
 * @brief convert a array to hex string
 * 
 * @param data array input
 * @param len length of array
 * @param buf buffer output of hex string
 * 
 * @return None
 */
void appUtils_convertHexToString(uint8_t *data, uint8_t len, char *buf)
{
    int idx = 0;
    uint8_t i = 0;
    for (i = 0; i < len; i++) {
        buf[idx++] = g_hex_str[data[i] / 16];
        buf[idx++] = g_hex_str[data[i] % 16];
    }
}


/**
 * @brief convert a array to u32 number
 * 
 * @param data array input (maximum 4 bytes)
 * @param len length of array
 * 
 * @return u32 number
 */
uint32_t appUtils_convertHexStringToU32(char *data, uint8_t len)
{
    uint32_t out = 0;

    for (uint16_t i = 0; i < len; i++) {
        if ((data[i] >= '0') && (data[i] <= '9')) {
            out = (out << 4) | (data[i] ^ 0x30);
        } else if ((data[i] >= 'A') && (data[i] <= 'F')) {
            out = (out << 4) | (data[i] - 55);
        } else if ((data[i] >= 'a') && (data[i] <= 'f')) {
            out = (out << 4) | (data[i] - 87);
        } else {
            break;
        }
    }

    return out;
}


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
        char to, char *out, uint16_t out_size, uint16_t *out_len)
{
    if (str == NULL) {
        return NULL;
    }
    
    char *begin = NULL;
    char *end = NULL;

    if (from != 0) {
        begin = appUtils_getString_afterChar(str, from);
    } else {
        begin = str;
    }

    /* can't get string started from "from" */
    if (begin == NULL) {
        return NULL;
    }

    end = begin;
    int len = 0;
    while ((*end != 0) && (*end != to)) {
        end++;
        len++;
    }
    if ((*end != to) && (len == 0)) {
        return NULL;
    }

    int i = 0;
    for (i = 0; i < len && i < out_size; i++) {
        out[i] = *begin++;
    }
    *out_len = i;
    return (begin + 1);
}


/**
 * @brief check if a string is number or not
 * 
 * @param buf input string
 * @param len length of string
 * 
 * @retval true if string is a number
 * @retval false if not number
 */
bool appUtils_isNumber(char *buf, int len)
{
    for (int i = 0; i < len; i++) {
        if (i == 0) {
            if ((buf[i] != '-') && ((buf[i] < '0') || (buf[i] > '9'))) {
                return false;
            }
        } else if ((buf[i] < '0') || (buf[i] > '9')) {
            return false;
        }
    }

    return true;
}


/**
 * @brief replace "\n" string in a string to character '\n'
 * 
 * @param data input string
 * @param data_len length of input string
 * @param out output buffer
 * 
 * @return length of new string
 */
int appUtils_replace_newline(char *data, int data_len, char *out)
{
	if (data_len <= 0) {
		return 0;
	}

	int i = 0;
	int len = 0;

	while (i < data_len) {
		if ((i < (data_len - 1)) && (data[i] == '\\') && (data[i + 1] == 'n')) {
			out[len++] = '\n';
			i++;
		} else {
			out[len++] = data[i];
		}
		i++;
	}

	out[len] = 0;
	return len;
}