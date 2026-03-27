/*
 * cli_queue.c
 *
 * This file will handle input command using a queue.
 * the queue is a FIFO type.
 * 
 */
#include "cli_queue.h"

#if (CONFIG_CLI_ENABLED == 1)

/**
 * @brief clear Queue to empty
 * @param queueObj: Queue object to clear
 * @return None
*/
void cli_queue_free(cli_queue_t *queueObj)
{
	for (uint16_t i = 0; i < CONFIG_CLI_QUEUE_SIZE; i++) {
		queueObj->buf[i] = 0;
	}
	queueObj->length = 0;
	queueObj->read_pos = 0;
	queueObj->write_pos = 0;
}


/**
 * @brief Get current length of data in queue
 * @param queueObj: Queue object
 * @return length of data in queue
*/
uint16_t cli_queue_length(cli_queue_t *queueObj)
{
	return queueObj->length;
}


/**
 * @brief push a character to queue
 * @param
 * 		queueObj: Queue object
 * 		value: character input
 * @return
 * 		false: if queue is full
 * 		true: success
*/
uint8_t cli_queue_Push(cli_queue_t *queueObj, uint8_t value)
{
	if (cli_queue_IsFull(queueObj) == true) {
		return 0;
	}
	queueObj->buf[queueObj->write_pos++] = value;
	queueObj->length++;
	if (queueObj->write_pos >= CONFIG_CLI_QUEUE_SIZE) {
		queueObj->write_pos = 0;
	}
	return 1;
}


/**
 * @brief get a character from queue
 * @param
 * 		queueObj: Queue object
 * 		value: character output
 * @return
 * 		false: if queue is empty
 * 		true: success
*/
uint8_t cli_queue_Pop(cli_queue_t *queueObj, uint8_t *value)
{
	if (cli_queue_IsEmpty(queueObj) == true) {
		return 0;
	}

	*value = queueObj->buf[queueObj->read_pos++];
	queueObj->length--;
	if (queueObj->read_pos >= CONFIG_CLI_QUEUE_SIZE) {
		queueObj->read_pos = 0;
	}
	return 1;
}


/**
 * @brief check if queue is full or not
 * @param queueObj: Queue object
 * @return
 * 		false: if queue is not full
 * 		true: full
*/
bool cli_queue_IsFull(cli_queue_t *queueObj)
{
	return (queueObj->length >= CONFIG_CLI_QUEUE_SIZE);
}


/**
 * @brief check if queue is empty or not
 * @param queueObj: Queue object
 * @return
 * 		false: if queue is not empty
 * 		true: empty
*/
bool cli_queue_IsEmpty(cli_queue_t *queueObj)
{
	return (queueObj->length == 0);
}

#endif // CONFIG_CLI_ENABLED












