#include "os.h"

/* defined in entry.S */
extern void switch_to(struct context *next);

#define MAX_TASKS 10
#define STACK_SIZE 1024
/*
 * In the standard RISC-V calling convention, the stack pointer sp
 * is always 16-byte aligned.
 */
uint8_t __attribute__((aligned(16))) task_stack[MAX_TASKS][STACK_SIZE];
struct context ctx_tasks[MAX_TASKS];

struct tasks {
	int priority;
	int waiting_time;
}tasks[MAX_TASKS];

/*
 * _top is used to mark the max available position of ctx_tasks
 * _current is used to point to the context of current task
 */
static int _top = 0;
static int _current = -1;

void sched_init()
{
	w_mscratch(0);

	/* enable machine-mode software interrupts. */
	// 打开机器模式的软件中断
	w_mie(r_mie() | MIE_MSIE);
}

/*
 * implment a simple cycle FIFO schedular
 */
void schedule()
{
	if (_top <= 0) {
		panic("Num of task should be greater than zero!");
		return;
	}

	// _current = (_current + 1) % _top;
	// 优先级调度
	int highest_score = 0;
	for (int i = 0; i < _top; i++) {
		tasks[i].waiting_time++;
		int score = tasks[i].priority + tasks[i].waiting_time;
		printf("task %d, priority: %d, waiting time: %d, score: %d\n", i, tasks[i].priority, tasks[i].waiting_time, score);
		if (score > highest_score) {
			highest_score = score;
			_current = i;
		}
	}

	printf("switch to task %d\n", _current);
	tasks[_current].waiting_time = 0;

	struct context *next = &(ctx_tasks[_current]);
	switch_to(next);
}

/*
 * DESCRIPTION
 * 	Create a task.
 * 	- start_routin: task routine entry
 * RETURN VALUE
 * 	0: success
 * 	-1: if error occured
 */
int task_create(void (*start_routin)(void), int priority)
{
	if (_top < MAX_TASKS) {
		ctx_tasks[_top].sp = (reg_t) &task_stack[_top][STACK_SIZE];
		ctx_tasks[_top].pc = (reg_t) start_routin;
		tasks[_top].priority = priority;
		tasks[_top].waiting_time = 0;
		_top++;
		return 0;
	} else {
		return -1;
	}
}

/*
 * DESCRIPTION
 * 	task_yield()  causes the calling task to relinquish the CPU and a new 
 * 	task gets to run.
 */
void task_yield()
{
	// 通过软件中断来切换任务
	/* trigger a machine-level software interrupt */
	int id = r_mhartid();
	*(uint32_t*)CLINT_MSIP(id) = 1;
}

/*
 * a very rough implementaion, just to consume the cpu
 */
void task_delay(volatile int count)
{
	count *= 50000;
	while (count--);
}

