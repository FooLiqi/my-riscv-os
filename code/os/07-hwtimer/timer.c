#include "os.h"

/* interval ~= 1s */
// 设置中断的时间间隔 ~= 1s
#define TIMER_INTERVAL CLINT_TIMEBASE_FREQ

static uint32_t _tick = 0;

extern void schedule(void);

/* load timer interval(in ticks) for next timer interrupt.*/
// 设置下一次中断的时间
void timer_load(int interval)
{
	/* each CPU has a separate source of timer interrupts. */
	int id = r_mhartid();
	
	*(uint64_t*)CLINT_MTIMECMP(id) = *(uint64_t*)CLINT_MTIME + interval;
}

void timer_init()
{
	/*
	 * On reset, mtime is cleared to zero, but the mtimecmp registers 
	 * are not reset. So we have to init the mtimecmp manually.
	 */
	timer_load(TIMER_INTERVAL);

	/* enable machine-mode timer interrupts. */
	w_mie(r_mie() | MIE_MTIE);

	/* enable machine-mode global interrupts. */
	w_mstatus(r_mstatus() | MSTATUS_MIE);
}

void timer_handler() 
{
	_tick++;
	printf("tick: %d\n", _tick);

	// 重新设置下一次中断的时间
	timer_load(TIMER_INTERVAL);

	// 调度
	// schedule();
}
