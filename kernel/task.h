/********************
	naming.h
	create by: jkhpro
********************/

#ifndef KERNEL_TASK_H_
#define KERNEL_TASK_H_

#include "MemoryMap.h"

#define NOT_ENOUGH_TASK_NUM     0xFFFFFFFF

#define USR_TASK_STACK_SIZE     0x100000 //개별 task 크기 0x100000:1MB 실제 할당 시 더 적은 경우가 많다.
#define MAX_TASK_NUM            (TASK_STACK_SIZE / USR_TASK_STACK_SIZE) //task 생성 최대 개수 64MB/1MB = 64 

//상태 레지스터, 범용레지스터 백업공간 확보
typedef struct KernelTaskContext_t
{
    uint32_t spsr;
    uint32_t r0_r12[13];
    uint32_t pc;
} KernelTaskContext_t;

//스택 관련 정보
typedef struct KernelTcb_t
{
    uint32_t sp;
    uint8_t* stack_base;
} KernelTcb_t;

typedef void (*KernelTaskFunc_t)(void);

void     Kernel_task_init(void);
void     Kernel_task_start(void);
uint32_t Kernel_task_create(KernelTaskFunc_t startFunc);
void     Kernel_task_scheduler(void);
void     Kernel_task_context_switching(void);
uint32_t Kernel_task_get_current_task_id(void);

#endif /* KERNEL_TASK_H_ */
