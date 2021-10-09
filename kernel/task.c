/********************
	task.c
	create by: jkhpro
********************/

#include "stdint.h"
#include "stdbool.h"

#include "ARMv7AR.h"
#include "task.h"

static KernelTcb_t  sTask_list[MAX_TASK_NUM]; //task 블록 64개 배열, 동적할당 방지
static KernelTcb_t* sCurrent_tcb;// 생성한 task index 저장 변수, task 생성개수 파악용 변수
static KernelTcb_t* sNext_tcb;
static uint32_t     sAllocated_tcb_index;//이 변수보다 작은 인덱스 블록은 이미 할당됨, 큰값의 sTask_list배열은 할당x
static uint32_t     sCurrent_tcb_index;

static KernelTcb_t* Scheduler_round_robin_algorithm(void);
static void Save_context(void);
static void Restore_context(void);

void Kernel_task_init(void)
{
    sAllocated_tcb_index = 0;// 배열 시작은 0, 초기화 시 할당된 task는 없음
    sCurrent_tcb_index = 0;// 0으로 초기화

    for(uint32_t i = 0 ; i < MAX_TASK_NUM ; i++)
    {
        sTask_list[i].stack_base = (uint8_t*)(TASK_STACK_START + (i * USR_TASK_STACK_SIZE));
        sTask_list[i].sp = (uint32_t)sTask_list[i].stack_base + USR_TASK_STACK_SIZE - 4;

        sTask_list[i].sp -= sizeof(KernelTaskContext_t);
        KernelTaskContext_t* ctx = (KernelTaskContext_t*)sTask_list[i].sp;
        ctx->pc = 0;
        ctx->spsr = ARM_MODE_BIT_SYS;
    }
}

// 커널 시작 시 최초 1회 실행
/* 컨텍스트 스위칭 과정에서 최초 실행시 문제를 해결하기 위한 함수
 * 1. 현재 실행 중인 테스크의 컨텍스트를 해당 테스크의 스텍에 백업하고 테스크 컨트롤 블럭에 스택포인터 백업
 * 2. 스케줄러가 알려준 TCB에서 SP를 받아 해당 스텍에서 테스크의 컨텍스트 복구
 * 이 스케줄링은 현재 동작중인 테스크가 있다는 것을 가정하고 있다.
 * 처음 시작시에는 현재 tcb idx가 0 인 상태에서 복구만 진행한다.
 */
void Kernel_task_start(void)
{
    sNext_tcb = &sTask_list[sCurrent_tcb_index];
    Restore_context();
}

//task 실행 시 필요한 것
/*
	1. task 생성
	2. 스케줄러
	3. 컨텍스트 스위칭
*/

uint32_t Kernel_task_create(KernelTaskFunc_t startFunc)
{
    KernelTcb_t* new_tcb = &sTask_list[sAllocated_tcb_index++];//새로운 task 컨트롤 블록에 등록, 그리고 할당된 인덱스 값 + 1 

	 //할당된 task 개수가 64개 초과 시
    if (sAllocated_tcb_index > MAX_TASK_NUM)
    {
        return NOT_ENOUGH_TASK_NUM;
    }

    KernelTaskContext_t* ctx = (KernelTaskContext_t*)new_tcb->sp;
    ctx->pc = (uint32_t)startFunc;

    return (sAllocated_tcb_index - 1);//현재 작업의 task index값 return sAllocated_tcb_index값은 이미 1증가 되어있으므로 -1
}

uint32_t Kernel_task_get_current_task_id(void)
{
    return sCurrent_tcb_index;
}

// task 스케줄러: 지금 실행중인 테스크 다음에 실행할 테스크 지정하는 함수
void Kernel_task_scheduler(void)
{
    sCurrent_tcb = &sTask_list[sCurrent_tcb_index];
    sNext_tcb = Scheduler_round_robin_algorithm();

    Kernel_task_context_switching();//컨텍스트 스위칭
}

// 컨텍스트 스위칭 함수: 2가지 함수를 불러온다 
/* 1. 현재 컨텍스트를 현재 테스크에 저장한다. 2. 다음 테스크에 접근한 후 백업한 컨텍스트 복구 
 * : 다음 테스크로 이동전 현재 테스크 백업 후 이동
 */
/* GCC attribute 기능
 * __attribute__((naked)) 옵션 없이 선언하면 스택 앞뒤로 스택 확보 코드와 리턴 코드가 생성된다.
 * 컨텍스트를 스택에 백업하고 스택에서 복구하기 위해 __attribute__((naked)) 사용한다.
 * 스택확보도 안하고 LR에 리턴 주소도 넣지 않음, 일반적인 C언어 함수 호출과 다름
 * 어셈블리어로 직접 제어했기 때문에 문제되지 않는다.
 */
__attribute__ ((naked)) void Kernel_task_context_switching(void)
{
	//불러오기 옵션 B: ARM 인스트럭션 B -> LR 유지
    __asm__ ("B Save_context");
    __asm__ ("B Restore_context");
}

//컨텍스트 저장
static __attribute__ ((naked)) void Save_context(void)
{
    // save current task context into the current task stack
	 // 함수가 불러오는 시점:현재 테스크상황에서 상태레지스터, 범용레지스터, 링크 레지스터  저장 
	
	 /* 구조체는 메모리주소가 낮은 값부터 저장을 한다.
	  * ex) 메모리 주소 0x04부터 시작한다면
	  * uint32_t spsr; 0x04
	  * uint32_t r0_r12; 0x08 ~ 0x38
	  * uint32_t pc; 0x3C
	  스텍은 메모리 주소가 높은 값에서 낮은 값으로 진행하므로 백업은 역순으로 진행해야 한다.
	  */
    __asm__ ("PUSH {lr}");
    __asm__ ("PUSH {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12}");
    __asm__ ("MRS   r0, cpsr");
    __asm__ ("PUSH {r0}");
    // save current task stack pointer into the current TCB
	 /*(uint32_t)(*sCurrent_tcb) = SP;*/
    __asm__ ("LDR   r0, =sCurrent_tcb");//현재 동작하고 있는 테스크 컨텍스트 블록의 포인터 변수 읽기
    __asm__ ("LDR   r0, [r0]");// 포인터에 저장된 값 읽기
    __asm__ ("STMIA r0!, {sp}");//읽은 값을 베이스 메모리 주소로 SP 저장
}
//컨텍스트 복구
//컨텍스트 저장 순서에서 정확히 역순(제어도 역으로 한다. PUSH->POP, STMIA->SDMIA)
static __attribute__ ((naked)) void Restore_context(void)
{
    // restore next task stack pointer from the next TCB
    __asm__ ("LDR   r0, =sNext_tcb");
    __asm__ ("LDR   r0, [r0]");
    __asm__ ("LDMIA r0!, {sp}");
    // restore next task context from the next task stack
    __asm__ ("POP  {r0}");
    __asm__ ("MSR   cpsr, r0");
    __asm__ ("POP  {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12}");
    __asm__ ("POP  {pc}");
}

/* 라운드로빈(RR) 알고리즘
 * task 최대 개수(64) 일 때, (우선순위 x, task 인덱스 순서대로 진행된다고 가정)
 * 
 * task #0 -> task #1 -> task #2 ... task #62 -> task #63 -> task #0 ...
 * 이 알고리즘을 컨텍스트 스위칭에 적용
 */
static KernelTcb_t* Scheduler_round_robin_algorithm(void)
{
    sCurrent_tcb_index++;//index 1 추가
    sCurrent_tcb_index %= sAllocated_tcb_index;//모듈로 연산을 통해 다음 인덱스를 선정(최대값 이상이 되면 최소값으로 돌린다.)

    return &sTask_list[sCurrent_tcb_index]; //index 1 추가된 task return 해서 다음 테스크에 넣음
}

//스케줄러와 스위칭 제작 완료, => '스케줄링', '언제' 스케줄링 하는지 (선점형/비선점형) 선점형 -> 비선점형으로 변경 (21.10.07)
