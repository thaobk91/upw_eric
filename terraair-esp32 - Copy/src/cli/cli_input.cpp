/*
 * cli_input.c
 *
 * This file will handle at top of command source. and initialize basic commands
 * 
 */
#include "cli_input.h"
#include "cli_output.h"

#if CONFIG_CLI_ENABLED == 1

#define INPUT_POP_MAX_LEN			4
#define CLI_NULL					'\0'

typedef union {
    uint32_t	u32;
    uint8_t		u8[INPUT_POP_MAX_LEN];
} key_code_t;

typedef struct {
    key_code_t		buf;
    uint8_t 		count;
} key_buf_t;

static key_buf_t g_cli_input_key_buf;

static cli_input_buff_t g_cli_input_buff;
static cli_queue_t g_cli_queue;

#if CLI_LOG_ENABLE == 1
static cli_log_cmd_t g_cli_log_cmd;
static bool	g_log_IsUP = true;
static int8_t g_log_old_pos = 0;
#endif

static const key_code_t g_value_keycode_enter 		= {.u8 = {0x0D, 0x00, 0x00, 0x00}};
static const key_code_t g_value_keycode_backspace	= {.u8 = {0x08, 0x00, 0x00, 0x00}};
static const key_code_t g_value_keycode_up			= {.u8 = {0x1B, 0x5B, 0x41, 0x00}};
static const key_code_t g_value_keycode_down		= {.u8 = {0x1B, 0x5B, 0x42, 0x00}};
static const key_code_t g_value_keycode_right	 	= {.u8 = {0x1B, 0x5B, 0x43, 0x00}};
static const key_code_t g_value_keycode_left		= {.u8 = {0x1B, 0x5B, 0x44, 0x00}};
static const key_code_t g_value_keycode_del			= {.u8 = {0x7F, 0x00, 0x00, 0x00}};
static const key_code_t g_value_keycode_home		= {.u8 = {0x1B, 0x5B, 0x31, 0x7E}};
static const key_code_t g_value_keycode_end			= {.u8 = {0x1B, 0x5B, 0x34, 0x7E}};

static void cli_input_refresh(char *cmd);
#if CLI_LOG_ENABLE  == 1
static void cli_input_saved_cmd(char *cmd);
static char *cli_input_get_logcmd(bool isUp);
#endif


/**
 * @brief initialize cli input. clear all parameters
 * @param None
 * @return None
*/
void cli_input_init(void)
{
	memset(&g_cli_input_key_buf, 0, sizeof(g_cli_input_key_buf));
	memset(&g_cli_input_buff, 0, sizeof(g_cli_input_buff));
	memset(&g_cli_queue, 0, sizeof(g_cli_queue));
#if (CLI_LOG_ENABLE != 0)
	memset(&g_cli_log_cmd, 0, sizeof(g_cli_log_cmd));
	g_log_IsUP = true;
	g_log_old_pos = 0;
#endif
}


/**
 * @brief get a character from input and push it in a queue (fifo)
 * @param c: input character
 * @return None
*/
void cli_input_get(char c)
{
	cli_queue_Push(&g_cli_queue, c);
}


/**
 * @brief check keyboard codes, the codes get from Queue
 * @param None
 * @return input key type
*/
cli_input_param_t cli_input_check_keycode(void)
{
	cli_input_param_t i_value;
	bool in_wait = false;

	memset(&i_value, 0, sizeof(i_value));
	i_value.inValid = true;
	cli_queue_Pop(&g_cli_queue, &g_cli_input_key_buf.buf.u8[g_cli_input_key_buf.count]);

	if (g_cli_input_key_buf.buf.u32 == g_value_keycode_enter.u32) {
		i_value.isEnter = true;
	} else if (g_cli_input_key_buf.buf.u32 == g_value_keycode_backspace.u32) {
		i_value.isBackspace = true;
	} else if (g_cli_input_key_buf.buf.u32 == g_value_keycode_up.u32) {
		i_value.isKeyup = true;
	} else if (g_cli_input_key_buf.buf.u32 == g_value_keycode_down.u32) {
		i_value.isKeydown = true;
	} else if (g_cli_input_key_buf.buf.u32 == g_value_keycode_right.u32) {
		i_value.isKeyright = true;
	} else if (g_cli_input_key_buf.buf.u32 == g_value_keycode_left.u32) {
		i_value.isKeyleft = true;
	} else if (g_cli_input_key_buf.buf.u32 == g_value_keycode_del.u32) {
		i_value.isKeydel = true;
	} else if (g_cli_input_key_buf.buf.u32 == g_value_keycode_home.u32) {
		i_value.isKeyhome = true;
	} else if (g_cli_input_key_buf.buf.u32 == g_value_keycode_end.u32) {
		i_value.isKeyend = true;
	} else if (g_cli_input_key_buf.buf.u8[0] == 0x1B/*'\e'*/) {
		g_cli_input_key_buf.count++;
		in_wait = true;
	} else if ((g_cli_input_key_buf.buf.u8[g_cli_input_key_buf.count] >= 0x20) &&
		(g_cli_input_key_buf.buf.u8[g_cli_input_key_buf.count] <= 0x7E)) {
		i_value.isAlphabet = true;
		i_value.keycode = g_cli_input_key_buf.buf.u8[g_cli_input_key_buf.count];
	} else {
		g_cli_input_key_buf.count = 0;
		g_cli_input_key_buf.buf.u32 = 0;
		i_value.inValid = false;
		return i_value;
	}

	if ((in_wait == false) || (g_cli_input_key_buf.count >= INPUT_POP_MAX_LEN)) {
		g_cli_input_key_buf.count = 0;
		g_cli_input_key_buf.buf.u32 = 0;
	}

	return i_value;
}


/**
 * @brief add a character to CLI input buffer
 * @param c: input character
 * @return None
*/
void cli_input_add_char(char c)
{
	if (g_cli_input_buff.length >= CONFIG_CLI_INPUT_MAX_SIZE) {
		return;
	}

	g_cli_input_buff.data[g_cli_input_buff.length] = c;
	g_cli_input_buff.length++;
	g_cli_input_buff.cursor_pos++;
	CLI_PutChar(c);
}


/**
 * @brief handle ENTER code
 * @param
 * 		cmd_list: list of system commands
 * 		cmd_num: number of system commands
 * @return None
*/
void cli_input_keycode_enter(cli_cmd_t **cmd_list, uint8_t cmd_num)
{
	CLI_PutBuff((char *)CONFIG_CLI_STRING_ENTER);
	if (g_cli_input_buff.length > 0) {
		if (cli_output_checkcommand(cmd_list, cmd_num,
			g_cli_input_buff.data, g_cli_input_buff.length) == false) {
			CLI_PutBuff((char *)"command input error. Pls check \"help\" to get command list");
		}
		CLI_PutBuff((char *)CONFIG_CLI_STRING_ENTER);

		// save cmd
		cli_input_saved_cmd(g_cli_input_buff.data);

		//reset buffer
		for (uint16_t i = 0; i < g_cli_input_buff.length; i++) {
			g_cli_input_buff.data[i] = 0;
		}
		g_cli_input_buff.cursor_pos = 0;
		g_cli_input_buff.length = 0;
	}
	g_cli_log_cmd.cur_pos = 0;
	CLI_PutBuff((char *)CONFIG_CLI_STRING_ARROW);
}


/**
 * @brief handle BACKSPACE code
 * @param None
 * @return None
*/
void cli_input_keycode_backspace(void)
{
	if (g_cli_input_buff.cursor_pos > 0) {
		CLI_PutBuff((char *)g_value_keycode_left.u8);
		CLI_PutBuff((char *)&g_cli_input_buff.data[g_cli_input_buff.cursor_pos]);
		CLI_PutChar(' ');

		for (uint16_t i = g_cli_input_buff.cursor_pos - 1;
			i < g_cli_input_buff.length; i++) {
			CLI_PutBuff((char *)g_value_keycode_left.u8);
		}

		if (g_cli_input_buff.cursor_pos == g_cli_input_buff.length) {
			g_cli_input_buff.data[g_cli_input_buff.length - 1] = 0;
		} else {
			for (uint16_t i = g_cli_input_buff.cursor_pos;
				i < g_cli_input_buff.length; i++) {
				g_cli_input_buff.data[i - 1] = g_cli_input_buff.data[i];
			}
			g_cli_input_buff.data[g_cli_input_buff.length - 1] = 0;
		}
		g_cli_input_buff.length--;
		g_cli_input_buff.cursor_pos--;
	}
}


/**
 * @brief handle LEFT code
 * @param None
 * @return None
*/
void cli_input_keycode_left(void)
{
	if (g_cli_input_buff.cursor_pos > 0) {
		g_cli_input_buff.cursor_pos--;
		CLI_PutBuff((char *)g_value_keycode_left.u8);
	}
}


/**
 * @brief handle RIGHT code
 * @param None
 * @return None
*/
void cli_input_keycode_right(void)
{
	if (g_cli_input_buff.cursor_pos < g_cli_input_buff.length) {
		g_cli_input_buff.cursor_pos++;
		CLI_PutBuff((char *)g_value_keycode_right.u8);
	}
}


/**
 * @brief handle UP code
 * @param None
 * @return None
*/
void cli_input_keycode_up(void)
{
#if (CLI_LOG_ENABLE != 0)
	cli_input_refresh(cli_input_get_logcmd(true));
#endif
}


/**
 * @brief handle DOWN code
 * @param None
 * @return None
*/
void cli_input_keycode_down(void)
{
#if (CLI_LOG_ENABLE != 0)
	cli_input_refresh(cli_input_get_logcmd(false));
#endif
}


/**
 * @brief handle DELETE code
 * @param None
 * @return None
*/
void cli_input_keycode_delete(void)
{
	if (g_cli_input_buff.cursor_pos < g_cli_input_buff.length) {
		CLI_PutBuff((char *)&g_cli_input_buff.data[g_cli_input_buff.cursor_pos + 1]);
		CLI_PutChar(' ');

		for (uint16_t i = g_cli_input_buff.cursor_pos; i < g_cli_input_buff.length; i++) {
			CLI_PutBuff((char *)g_value_keycode_left.u8);
		}

		if (g_cli_input_buff.cursor_pos == g_cli_input_buff.length) {
			g_cli_input_buff.data[g_cli_input_buff.length - 1] = 0;
		} else {
			for (uint16_t i = g_cli_input_buff.cursor_pos;
				i < g_cli_input_buff.length; i++) {
				g_cli_input_buff.data[i] = g_cli_input_buff.data[i + 1];
			}
			g_cli_input_buff.data[g_cli_input_buff.length - 1] = 0;
		}
		g_cli_input_buff.length--;
	}
}


/**
 * @brief handle HOME code
 * @param None
 * @return None
*/
void cli_input_keycode_home(void)
{
	if (g_cli_input_buff.cursor_pos > 0) {
		for (uint16_t i = 0; i < g_cli_input_buff.cursor_pos; i++) {
			CLI_PutBuff((char *)g_value_keycode_left.u8);
		}
		g_cli_input_buff.cursor_pos = 0;
	}
}


/**
 * @brief handle END code
 * @param None
 * @return None
*/
void cli_input_keycode_end(void)
{
	if (g_cli_input_buff.cursor_pos < g_cli_input_buff.length) {
		for (uint16_t i = g_cli_input_buff.cursor_pos;
			i < g_cli_input_buff.length; i++) {
			CLI_PutBuff((char *)g_value_keycode_right.u8);
		}
		g_cli_input_buff.cursor_pos = g_cli_input_buff.length;
	}
}


/*** PRIVATE FUNCTION ***/

/**
 * @brief refresh command and set command on terminal
 * @param cmd: input command
 * @return None
*/
void cli_input_refresh(char *cmd)
{
	if (cmd == NULL) {
		return;
	}

	cli_input_keycode_home();
	CLI_PutBuff(cmd);
	g_cli_input_buff.cursor_pos = strlen(cmd);

	int offset = 0;
	if (g_cli_input_buff.length > g_cli_input_buff.cursor_pos) {
		offset = g_cli_input_buff.length - g_cli_input_buff.cursor_pos;
		for (uint16_t i = 0; i < offset; i++) {
			CLI_PutChar(' ');
		}

		for (uint16_t i = 0; i < offset; i++) {
			CLI_PutBuff((char *)g_value_keycode_left.u8);
		}
	}

	strcpy(g_cli_input_buff.data, cmd);
	g_cli_input_buff.length = g_cli_input_buff.cursor_pos;
}


#if (CLI_LOG_ENABLE != 0)

/**
 * @brief save command that's entered before
 * @param cmd: input command entered
 * @return None
*/
void cli_input_saved_cmd(char *cmd)
{
	uint8_t num = g_cli_log_cmd.num;
	if (g_cli_log_cmd.num == CONFIG_CLI_LOG_SIZE) {
		num--;
	} else {
		g_cli_log_cmd.num++;
	}

	for (int8_t i = num; i > 0; i--) {
		strcpy(g_cli_log_cmd.log[i], g_cli_log_cmd.log[i - 1]);
	}

	strcpy(g_cli_log_cmd.log[0], cmd);
}


/**
 * @brief get a command that's saved in buffer
 * @param isUp: key code is UP
 * @return a command saved in buffer. this command is entered before
*/
char *cli_input_get_logcmd(bool isUp)
{
	if (isUp == true) {
		if (g_cli_log_cmd.num == 0) {
			return NULL;
		}
		if (g_log_IsUP == false) {
			if (g_log_old_pos < (g_cli_log_cmd.num - 1)) {
				g_cli_log_cmd.cur_pos = g_log_old_pos + 1;
			} else {
				g_cli_log_cmd.cur_pos = g_log_old_pos;
			}
		}
		g_log_IsUP = true;
		g_log_old_pos = g_cli_log_cmd.cur_pos;
		char *cmd = g_cli_log_cmd.log[g_cli_log_cmd.cur_pos];
		if (g_cli_log_cmd.cur_pos < (g_cli_log_cmd.num - 1)) {
			g_cli_log_cmd.cur_pos++;
		}
		return cmd;
	} else {
		if (g_cli_log_cmd.num == 0) {
			return NULL;
		}
		if (g_log_IsUP == true) {
			if (g_log_old_pos > 0) {
				g_cli_log_cmd.cur_pos = g_log_old_pos - 1;
			} else {
				g_cli_log_cmd.cur_pos = g_log_old_pos;
			}
		}
		g_log_IsUP = false;
		g_log_old_pos = g_cli_log_cmd.cur_pos;
		char *cmd = g_cli_log_cmd.log[g_cli_log_cmd.cur_pos];
		if (g_cli_log_cmd.cur_pos > 0) {
			g_cli_log_cmd.cur_pos--;
		}
		return cmd;
	}

	return NULL;
}

#endif // #if CLI_LOG_ENABLE == 1

#endif // CONFIG_CLI_ENABLED