/********************
	event.c
	create by: jkhpro
********************/

#include "stdint.h"
#include "stdbool.h"

#include "stdio.h"
#include "event.h"

static uint32_t sEventFlag; //이벤트 flag 맵 상태

void Kernel_event_flag_init(void)
{
    sEventFlag = 0;//초기화 flag map:  00000000000000000000000000000000
}

void Kernel_event_flag_set(KernelEventFlag_t event)
{
    sEventFlag |= (uint32_t)event;
}

void Kernel_event_flag_clear(KernelEventFlag_t event)
{
    sEventFlag &= ~((uint32_t)event);
}

bool Kernel_event_flag_check(KernelEventFlag_t event)
{
    if (sEventFlag & (uint32_t)event)
    {
        Kernel_event_flag_clear(event);
        return true;
    }
    return false;
}

