
#ifndef CLI_COMMAND_CLI_INPUT_H_
#define CLI_COMMAND_CLI_INPUT_H_

#include "cli_typedef.h"
#include "cli_queue.h"
#include "cli_config.h"

#if (CONFIG_CLI_ENABLED == 1)

typedef struct {
	bool		inValid;

	uint8_t		keycode;
	bool		isAlphabet;

	bool		isEnter;
	bool		isBackspace;
	bool		isKeyup;
	bool		isKeydown;
	bool		isKeyright;
	bool		isKeyleft;
	bool		isKeydel;
	bool		isKeyhome;
	bool		isKeyend;
} cli_input_param_t;


typedef struct {
	char		data[CONFIG_CLI_INPUT_MAX_SIZE + 1];
	uint16_t	length;
	uint16_t	cursor_pos;
} cli_input_buff_t;


#if (CLI_LOG_ENABLE != 0)
typedef struct {
	char		log[CONFIG_CLI_LOG_SIZE][CONFIG_CLI_INPUT_MAX_SIZE + 1];
	int8_t		cur_pos;
	int8_t		num;
} cli_log_cmd_t;
#endif

/**
 * @brief initialize cli input. clear all parameters
 * @param None
 * @return None
*/
void cli_input_init(void);

/**
 * @brief get a character from input and push it in a queue (fifo)
 * @param c: input character
 * @return None
*/
void cli_input_get(char c);

/**
 * @brief check keyboard codes, the codes get from Queue
 * @param None
 * @return input key type
*/
cli_input_param_t cli_input_check_keycode(void);

/**
 * @brief add a character to CLI input buffer
 * @param c: input character
 * @return None
*/
void cli_input_add_char(char c);

/**
 * @brief handle ENTER code
 * @param
 * 		cmd_list: list of system commands
 * 		cmd_num: number of system commands
 * @return None
*/
void cli_input_keycode_enter(cli_cmd_t **cmd_list, uint8_t cmd_num);

/**
 * @brief handle BACKSPACE code
 * @param None
 * @return None
*/
void cli_input_keycode_backspace(void);

/**
 * @brief handle LEFT code
 * @param None
 * @return None
*/
void cli_input_keycode_left(void);

/**
 * @brief handle RIGHT code
 * @param None
 * @return None
*/
void cli_input_keycode_right(void);

/**
 * @brief handle UP code
 * @param None
 * @return None
*/
void cli_input_keycode_up(void);

/**
 * @brief handle DOWN code
 * @param None
 * @return None
*/
void cli_input_keycode_down(void);

/**
 * @brief handle DELETE code
 * @param None
 * @return None
*/
void cli_input_keycode_delete(void);

/**
 * @brief handle HOME code
 * @param None
 * @return None
*/
void cli_input_keycode_home(void);

/**
 * @brief handle END code
 * @param None
 * @return None
*/
void cli_input_keycode_end(void);

#endif // CONFIG_CLI_ENABLED

#endif /* CLI_COMMAND_CLI_INPUT_H_ */
















