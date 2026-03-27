
#ifndef CLI_COMMAND_CLI_QUEUE_H_
#define CLI_COMMAND_CLI_QUEUE_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "cli_config.h"

#if (CONFIG_CLI_ENABLED == 1)

typedef struct {
	char buf[CONFIG_CLI_QUEUE_SIZE + 1];
	uint16_t	length;
	uint16_t	read_pos;
	uint16_t	write_pos;
} cli_queue_t;

/**
 * @brief clear Queue to empty
 * @param queueObj: Queue object to clear
 * @return None
*/
void cli_queue_free(cli_queue_t *queueObj);

/**
 * @brief Get current length of data in queue
 * @param queueObj: Queue object
 * @return length of data in queue
*/
uint16_t cli_queue_length(cli_queue_t *queueObj);

/**
 * @brief push a character to queue
 * @param
 * 		queueObj: Queue object
 * 		value: character input
 * @return
 * 		false: if queue is full
 * 		true: success
*/
uint8_t cli_queue_Push(cli_queue_t *queueObj, uint8_t value);

/**
 * @brief get a character from queue
 * @param
 * 		queueObj: Queue object
 * 		value: character output
 * @return
 * 		false: if queue is empty
 * 		true: success
*/
uint8_t cli_queue_Pop(cli_queue_t *queueObj, uint8_t *value);

/**
 * @brief check if queue is full or not
 * @param queueObj: Queue object
 * @return
 * 		false: if queue is not full
 * 		true: full
*/
bool cli_queue_IsFull(cli_queue_t *queueObj);

/**
 * @brief check if queue is empty or not
 * @param queueObj: Queue object
 * @return
 * 		false: if queue is not empty
 * 		true: empty
*/
bool cli_queue_IsEmpty(cli_queue_t *queueObj);

#endif // CONFIG_CLI_ENABLED 

#endif /* CLI_COMMAND_CLI_QUEUE_H_ */
