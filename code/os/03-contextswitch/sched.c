#include "os.h"

/* defined in entry.S */
extern void switch_to(struct context *next);

#define STACK_SIZE 1024
/*
 * In the standard RISC-V calling convention, the stack pointer sp
 * is always 16-byte aligned.
 */
uint8_t __attribute__((aligned(16))) task_stack[STACK_SIZE];			// 内存对齐
struct context ctx_task;												// 当前任务上下文

// 写入mscratch寄存器
static void w_mscratch(reg_t x)
{
	asm volatile("csrw mscratch, %0" : : "r" (x));
}

void user_task0(void);
void sched_init()
{
	w_mscratch(0);

	ctx_task.sp = (reg_t) &task_stack[STACK_SIZE];						// 栈指针
	ctx_task.ra = (reg_t) user_task0;									// 返回地址
}

void schedule()
{
	struct context *next = &ctx_task;
	switch_to(next);
}

/*
 * a very rough implementaion, just to consume the cpu
 */
void task_delay(volatile int count)
{
	count *= 50000;
	while (count--);
}


void user_task0(void)
{
	uart_puts("Task 0: Created!\n");
	while (1) {
		uart_puts("Task 0: Running...\n");
		task_delay(1000);
	}
}

