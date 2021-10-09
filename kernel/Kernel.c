/********************
	Kernel.c
	create by: jkhpro
********************/

#include "stdint.h"
#include "stdbool.h"

#include "memio.h"
#include "Kernel.h"

//첫 테스크 시작시 최초 1회만 호출
void Kernel_start(void)
{
    Kernel_task_start();
}

//비선점 스케줄링에서 테스크에서 직접 스케줄링 요청
void Kernel_yield(void)
{
    Kernel_task_scheduler();
}

//여러개 이벤트도 한번에 처리할 수 있음
/* ex)
 *		0000 0010 0000 0000 0000 0001 0000 0100
 * &	0000 0000 0000 0000 0000 0000 0000 0001
 * -------------------------------------------
 * for loop를 통해서 결과가 1일 때 flag set을 한다.
 * i가 2일 때, 8일 때, 26일 때
 * sending_event 0으로 초기화
 * sending_event에 1값을 넣고  해당 자리수 (i) 만큼 왼쪽으로 shift
 * sending_event값을 Kernel 이벤트 플래그 맵에 등록
 */
void Kernel_send_events(uint32_t event_list)
{
    for (uint32_t i = 0 ; i < 32 ; i++)
    {
        if ((event_list >> i) & 1)
        {
            KernelEventFlag_t sending_event = KernelEventFlag_Empty;
            sending_event = (KernelEventFlag_t)SET_BIT(sending_event, i);
            Kernel_event_flag_set(sending_event);
        }
    }
}
//여러개 이벤트 한번에 받을 수 있음
KernelEventFlag_t Kernel_wait_events(uint32_t waiting_list)
{
    for (uint32_t i = 0 ; i < 32 ; i++)
    {
        if ((waiting_list >> i) & 1)
        {
            KernelEventFlag_t waiting_event = KernelEventFlag_Empty;
            waiting_event = (KernelEventFlag_t)SET_BIT(waiting_event, i);

            if (Kernel_event_flag_check(waiting_event))
            {
                return waiting_event;
            }
        }
    }

    return KernelEventFlag_Empty;
}

/* ex)
 * Kernel_send_events(event1|event2|event3|event4);
 
 * Task #1
 * Kernel_wait_events(evnet1|event3);

 * Task #2
 * Kernel_wait_events(event2);

 * Task #3
 * Kernel_wait_events(event4);
 */






//메세지 보내기 API
bool Kernel_send_msg(KernelMsgQ_t Qname, void* data, uint32_t count)
{
    uint8_t* d = (uint8_t*)data;

    for (uint32_t i = 0 ; i < count ; i++)
    {
		  /* enqueue 함수 실행 후  결과 false 확인
		   * true -> d++, 계속 for loop (i) 진행
		   * flase ->  for loop (j) 실행 */
        if (false == Kernel_msgQ_enqueue(Qname, *d))
        {
            for (uint32_t j = 0 ; j < i ; j++)
            {
                uint8_t rollback;
                Kernel_msgQ_dequeue(Qname, &rollback);
            }
				//모든 데이터 rollback 후 메세지 보내기 종료
            return false;
        }
		  // 데이터를 하나씩 접근하기 위한 포인터 변수 연산
        d++;
    }
    return true;
}


uint32_t Kernel_recv_msg(KernelMsgQ_t Qname, void* out_data, uint32_t count)
{
    uint8_t* d = (uint8_t*)out_data;

    for (uint32_t i = 0 ; i < count ; i++)
    {
        /* enqueue 함수 실행 후  결과 false 확인
			* true -> d++, 계속 for loop (i) 진행
			* flase ->  for loop (j) 실행 */
		  if (false == Kernel_msgQ_dequeue(Qname, d))
        {
            return i;
        }
		  // 데이터를 하나씩 접근하기 위한 포인터 변수 연산
        d++;
    }
    return count;
}







void Kernel_lock_sem(void)
{
	 //wait
    while(false == Kernel_sem_test())
    {
        Kernel_yield();
    }
}

void Kernel_unlock_sem(void)
{
    Kernel_sem_release();
}

void Kernel_lock_mutex(void)
{
    while(true)
    {
        uint32_t current_task_id = Kernel_task_get_current_task_id();
        if (false == Kernel_mutex_lock(current_task_id))
        {
            Kernel_yield();
        }
        else
        {
            break;
        }
    }
}

void Kernel_unlock_mutex(void)
{
    uint32_t current_task_id = Kernel_task_get_current_task_id();
    if (false == Kernel_mutex_unlock(current_task_id))
    {
        Kernel_yield();
    }
}
