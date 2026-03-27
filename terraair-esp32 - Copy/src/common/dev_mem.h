
#ifndef __DEV_MEM_H
#define __DEV_MEM_H

#include "app_cfg.h"

/**
 * @brief malloc allocate memory, input memory will choose from internal or external RAM
 * 
 * @param size: size of memory needs to allocate
 * 
 * @retval memory is allocated or NULL if error
 */
void *dev_mem_malloc_selectRam(uint32_t size);

/**
 * @brief calloc allocate memory, input memory will choose from internal or external RAM
 * 
 * @param num: number of element need to allocate
 * @param element_size: size of a element
 * 
 * @retval memory is allocated or NULL if error
 */
void *dev_mem_calloc_selectRam(uint32_t num, uint32_t element_size);

/**
 * @brief malloc allocate memory, input memory is internal RAM
 * 
 * @param size: size of memory needs to allocate
 * 
 * @retval memory is allocated or NULL if error
 */
void *dev_mem_malloc_internalRam(uint32_t size);

/**
 * @brief malloc allocate memory, input memory is external RAM
 * 
 * @param size: size of memory needs to allocate
 * 
 * @retval memory is allocated or NULL if error
 */
void *dev_mem_malloc_externalRam(uint32_t size);

/**
 * @brief calloc allocate memory, input memory is iternal RAM
 * 
 * @param num: number of element need to allocate
 * @param element_size: size of a element
 * 
 * @retval memory is allocated or NULL if error
 */
void *dev_mem_calloc_internalRam(uint32_t num, uint32_t element_size);

/**
 * @brief calloc allocate memory, input memory is external RAM
 * 
 * @param num: number of element need to allocate
 * @param element_size: size of a element
 * 
 * @retval memory is allocated or NULL if error
 */
void *dev_mem_calloc_externalRam(uint32_t num, uint32_t element_size);

/**
 * @brief release memory
 * 
 * @param value variable needs to release memory
 * 
 * @return None
 */
void dev_mem_free(void **value);

#endif // __DEV_MEM_H