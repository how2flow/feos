#include "stdint.h"
#include "stdbool.h"

#include "HalUart.h"
#include "HalInterrupt.h"
#include "HalTimer.h"

#include "stdio.h"
#include "stdlib.h"

#include "Kernel.h"

static void Hw_init(void);
static void Kernel_init(void);

static void Printf_test(void);
static void Timer_test(void);

static void Test_critical_section(uint32_t p, uint32_t taskId);

//task
void User_task0(void);
void User_task1(void);
void User_task2(void);

void main(void)
{
    Hw_init();

    uint32_t i = 100;
	 //uart test
    while(i--)
    {
        Hal_uart_put_char('N');
    }
    Hal_uart_put_char('\n');

    putstr("Hello World!\n");

    Printf_test();
    Timer_test();

    Kernel_init();

    while(true);
}

static void Hw_init(void)
{
    Hal_interrupt_init();
    Hal_uart_init();
    Hal_timer_init();
}

static void Kernel_init(void)
{
    uint32_t taskId;

    Kernel_task_init();
    Kernel_event_flag_init();
    Kernel_msgQ_init();
    Kernel_sem_init(1);
    Kernel_mutex_init();

    taskId = Kernel_task_create(User_task0);
    if (NOT_ENOUGH_TASK_NUM == taskId)
    {
        putstr("Task0 creation fail\n");
    }

    taskId = Kernel_task_create(User_task1);
    if (NOT_ENOUGH_TASK_NUM == taskId)
    {
        putstr("Task1 creation fail\n");
    }

    taskId = Kernel_task_create(User_task2);
    if (NOT_ENOUGH_TASK_NUM == taskId)
    {
        putstr("Task2 creation fail\n");
    }

    Kernel_start();
}

static void Printf_test(void)
{
    char* str = "printf pointer test";
    char* nullptr = 0;
    uint32_t i = 5;
    uint32_t* sysctrl0 = (uint32_t*)0x10001000;

    debug_printf("%s\n", "Hello printf");
    debug_printf("output string pointer: %s\n", str);
    debug_printf("%s is null pointer, %u number\n", nullptr, 10);
    debug_printf("%u = 5\n", i);
    debug_printf("dec=%u hex=%x\n", 0xff, 0xff);
    debug_printf("print zero %u\n", 0);
    debug_printf("SYSCTRL0 %x\n", *sysctrl0);
}

static void Timer_test(void)
{
    for(uint32_t i = 0; i < 5 ; i++)
    {
        debug_printf("current count : %u\n", Hal_timer_get_1ms_counter());
        delay(1000);
    }
}

void User_task0(void)
{
    uint32_t local = 0;
    debug_printf("User Task #0 SP=0x%x\n", &local);

    uint8_t  cmdBuf[16];
    uint32_t cmdBufIdx = 0;
    uint8_t  uartch = 0;//메세지를 받을 그릇

    while(true)
    {
        KernelEventFlag_t handle_event = Kernel_wait_events(KernelEventFlag_UartIn|KernelEventFlag_CmdOut);
        switch(handle_event)
        {
        case KernelEventFlag_UartIn://Uart 이벤트 발생 시
            Kernel_recv_msg(KernelMsgQ_Task0, &uartch, 1); // 메세지를 받음
				//받은 메세지가 enter키 입력일 경우 cmdbuf모든 데이터를 보낸다. 보낸 후 cmdbuf 초기화
            if (uartch == '\r')
            {
                cmdBuf[cmdBufIdx] = '\0';
					
                Kernel_send_msg(KernelMsgQ_Task1, &cmdBufIdx, 1); //데이터 (데이터가 들어간 배열의 개수)
                Kernel_send_msg(KernelMsgQ_Task1, cmdBuf, cmdBufIdx); // 모든 데이터
                Kernel_send_events(KernelEventFlag_CmdIn);// CmdIn 이벤트

                cmdBufIdx = 0;
            }
				//보통 키보드 문자일 경우 배열에 데이터를 쌓는다.
            else
            {
                cmdBuf[cmdBufIdx] = uartch;
                cmdBufIdx++;
                cmdBufIdx %= 16;
            }
            break;
        case KernelEventFlag_CmdOut:
            //debug_printf("\nCmdOut Event by Task0\n");
            Test_critical_section(5, 0);
            break;
        }
        Kernel_yield();// 스케줄링 요청
    }
}

void User_task1(void)
{
    uint32_t local = 0;

    debug_printf("User Task #1 SP=0x%x\n", &local);

    uint8_t cmdlen = 0;
    uint8_t cmd[16] = {0};

    while(true)
    {
        KernelEventFlag_t handle_event = Kernel_wait_events(KernelEventFlag_CmdIn|KernelEventFlag_Unlock);
        switch(handle_event)
        {
        case KernelEventFlag_CmdIn://Task0에서 보낸 메세지와 이벤트를 받는다.
            memclr(cmd, 16);
            Kernel_recv_msg(KernelMsgQ_Task1, &cmdlen, 1);
            Kernel_recv_msg(KernelMsgQ_Task1, cmd, cmdlen);
            debug_printf("\nRecv Cmd: %s\n", cmd);
            break;
        case KernelEventFlag_Unlock:

            break;
        }
        Kernel_yield();
    }
}

void User_task2(void)
{
    uint32_t local = 0;

    debug_printf("User Task #2 SP=0x%x\n", &local);

    while(true)
    {
        Test_critical_section(3, 2);
        Kernel_yield();
    }
}

static uint32_t shared_value;
static void Test_critical_section(uint32_t p, uint32_t taskId)
{
    Kernel_lock_mutex();

    debug_printf("User Task #%u Send=%u\n", taskId, p);
    shared_value = p;
    Kernel_yield();
    delay(1000);
    debug_printf("User Task #%u Shared Value=%u\n", taskId, shared_value);

    Kernel_unlock_mutex();
}
