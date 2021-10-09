/********************
	synch.c
	create by: jkhpro
********************/

#include "stdint.h"
#include "stdbool.h"

#include "synch.h"

#define DEF_SEM_MAX 8 //컨텍스트 진입 가능 개수

static int32_t sSemMax;
static int32_t sSem;

KernelMutext_t sMutex;

void Kernel_sem_init(int32_t max)
{
	 //max:1~8
    sSemMax = (max <= 0) ? DEF_SEM_MAX : max;
    sSemMax = (max >= DEF_SEM_MAX) ? DEF_SEM_MAX : max;

    sSem = sSemMax;
}

//lock
bool Kernel_sem_test(void)
{
    if (sSem <= 0)
    {
        return false;
    }

    sSem--;

    return true;
}
//unlock
void Kernel_sem_release(void)
{
    sSem++;

    if (sSem >= sSemMax)
    {
        sSem = sSemMax;
    }
}
//mutex
void Kernel_mutex_init(void)
{
    sMutex.owner = 0;
    sMutex.lock = false;
}
//lock
bool Kernel_mutex_lock(uint32_t owner)
{
    if (sMutex.lock)
    {
        return false;
    }

    sMutex.owner = owner;
    sMutex.lock = true;
    return true;
}
//unlock
bool Kernel_mutex_unlock(uint32_t owner)
{
    if (owner == sMutex.owner)
    {
        sMutex.lock = false;
        return true;
    }
    return false;
}

