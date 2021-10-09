/********************
  msg.c
  create by: jkhpro
 ********************/


#include "stdint.h"
#include "stdbool.h"
#include "stdlib.h"

#include "msg.h"

KernelCirQ_t sMsgQ[KernelMsgQ_Num];

void Kernel_msgQ_init(void)
{
    for (uint32_t i = 0 ; i < KernelMsgQ_Num ; i++)
    {
        sMsgQ[i].front = 0;
        sMsgQ[i].rear = 0;
        memclr(sMsgQ[i].Queue, MSG_Q_SIZE_BYTE);
    }
}

bool Kernel_msgQ_is_empty(KernelMsgQ_t Qname)
{
    if (Qname >= KernelMsgQ_Num)
    {
        return false;
    }
	 //큐가 공백이면
    if (sMsgQ[Qname].front == sMsgQ[Qname].rear)
    {
        return true;
    }
	 //큐가 비어있지 않으면
    return false;
}

bool Kernel_msgQ_is_full(KernelMsgQ_t Qname)
{
    if (Qname >= KernelMsgQ_Num)
    {
        return false;
    }
	 //큐가 포화상태: rear의 값이 front의 값 바로 직전 인덱스 까지 커지면
    if (((sMsgQ[Qname].rear + 1) % MSG_Q_SIZE_BYTE) == sMsgQ[Qname].front)
    {
        return true;
    }
	 //큐가 포화상태가 아니면
    return false;
}

bool Kernel_msgQ_enqueue(KernelMsgQ_t Qname, uint8_t data)
{
    if (Qname >= KernelMsgQ_Num)
    {
        return false;
    }
	 //데이터 넣기 전 큐의 포화상태 확인
    if (Kernel_msgQ_is_full(Qname))
    {
        return false;
    }
    sMsgQ[Qname].rear++; // rear +1
    sMsgQ[Qname].rear %= MSG_Q_SIZE_BYTE; // RR과 동일한 모듈로 연산 처리

    uint32_t idx = sMsgQ[Qname].rear;
    sMsgQ[Qname].Queue[idx] = data; // 데이터 삽입

    return true;
}

bool Kernel_msgQ_dequeue(KernelMsgQ_t Qname, uint8_t* out_data)
{
    if (Qname >= KernelMsgQ_Num)
    {
        return false;
    }
	 //데이터 빼기 전 큐 공백상태 확인
    if (Kernel_msgQ_is_empty(Qname))
    {
        return false;
    }

	 //enqueue 와 인덱스 처리 동일
    sMsgQ[Qname].front++; 
    sMsgQ[Qname].front %= MSG_Q_SIZE_BYTE;

    uint32_t idx = sMsgQ[Qname].front;
    *out_data = sMsgQ[Qname].Queue[idx]; //데이터 읽기

    return true;
}
