
/*
 * dev_mem.c
 *
 * This file will handle memory allocate
 * 
 */

#include "dev_mem.h"


/**
 * @brief malloc allocate memory, input memory will choose from internal or external RAM
 * 
 * @param size: size of memory needs to allocate
 * 
 * @retval memory is allocated or NULL if error
 */
void *dev_mem_malloc_selectRam(uint32_t size)
{
    void *value = NULL;
#if CONFIG_SPIRAM == 1
    value = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
#else
    value = malloc(size);
#endif // CONFIG_SPIRAM
    return value;
}


/**
 * @brief calloc allocate memory, input memory will choose from internal or external RAM
 * 
 * @param num: number of element need to allocate
 * @param element_size: size of a element
 * 
 * @retval memory is allocated or NULL if error
 */
void *dev_mem_calloc_selectRam(uint32_t num, uint32_t element_size)
{
    void *value = NULL;
#if CONFIG_SPIRAM == 1
    value = heap_caps_calloc(num, element_size, MALLOC_CAP_SPIRAM);
#else
    value = calloc(num, element_size);
#endif // CONFIG_SPIRAM
    return value;
}


/**
 * @brief malloc allocate memory, input memory is internal RAM
 * 
 * @param size: size of memory needs to allocate
 * 
 * @retval memory is allocated or NULL if error
 */
void *dev_mem_malloc_internalRam(uint32_t size)
{
    return malloc(size);
}


/**
 * @brief malloc allocate memory, input memory is external RAM
 * 
 * @param size: size of memory needs to allocate
 * 
 * @retval memory is allocated or NULL if error
 */
void *dev_mem_malloc_externalRam(uint32_t size)
{
#if CONFIG_SPIRAM == 1
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
#else
    return NULL;
#endif // #if CONFIG_SPIRAM == 1
}


/**
 * @brief calloc allocate memory, input memory is iternal RAM
 * 
 * @param num: number of element need to allocate
 * @param element_size: size of a element
 * 
 * @retval memory is allocated or NULL if error
 */
void *dev_mem_calloc_internalRam(uint32_t num, uint32_t element_size)
{
    return calloc(num, element_size);
}


/**
 * @brief calloc allocate memory, input memory is external RAM
 * 
 * @param num: number of element need to allocate
 * @param element_size: size of a element
 * 
 * @retval memory is allocated or NULL if error
 */
void *dev_mem_calloc_externalRam(uint32_t num, uint32_t element_size)
{
#if CONFIG_SPIRAM == 1
    return heap_caps_calloc(num, element_size, MALLOC_CAP_SPIRAM);
#else
    return NULL;
#endif // #if CONFIG_SPIRAM == 1
}


/**
 * @brief release memory
 * 
 * @param value variable needs to release memory
 * 
 * @return None
 */
void dev_mem_free(void **value)
{
    if (*value == NULL) {
        return;
    }
    free(*value);
    *value = NULL;
}