#ifndef __SUPER_H__
#define __SUPER_H__

#define SUPER_WD_TIMEOUT            4000
#define SUPER_HARAKIRI_TIMEOUT_SEC  36000
typedef enum
{
    SUPER_CNTR_SENSOR = 0,
    SUPER_CNTR_R69, 
    SUPER_CNTR_IO,
    SUPER_CNTR_HARAKIRI,
    SUPER_CNTR_NBR_OF
} super_cntr_et;

void super_initialize(void);

void super_clear_cntr(super_cntr_et cntr_indx);

#endif