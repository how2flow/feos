/********************
  msg.h
  create by: jkhpro
 ********************/


#ifndef KERNEL_MSG_H_
#define KERNEL_MSG_H_

#define MSG_Q_SIZE_BYTE     512 //메세지 큐 크기 변경 가능

//task당 한개씩 큐 지시
typedef enum KernelMsgQ_t
{
    KernelMsgQ_Task0,
    KernelMsgQ_Task1,
    KernelMsgQ_Task2,

    KernelMsgQ_Num
} KernelMsgQ_t;

typedef struct KernelCirQ_t
{
    uint32_t front;//앞 데이터가 나갈 때 +1
    uint32_t rear;//뒤 데이터 저장할 때 +1
    uint8_t  Queue[MSG_Q_SIZE_BYTE];
} KernelCirQ_t;

void Kernel_msgQ_init(void);
bool Kernel_msgQ_is_empty(KernelMsgQ_t Qname);
bool Kernel_msgQ_is_full(KernelMsgQ_t Qname);
bool Kernel_msgQ_enqueue(KernelMsgQ_t Qname, uint8_t data);
bool Kernel_msgQ_dequeue(KernelMsgQ_t Qname, uint8_t* out_data);

#endif /* KERNEL_MSG_H_ */
