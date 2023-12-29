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

char time_buf[8] = {'0', '0', ':', '0', '0', ':', '0', '0'};
char back_buf[8] = {'\b', '\b', '\b', '\b', '\b', '\b', '\b', '\b'};

void timer_print()
{
	printf("tick: %d\n", _tick);
	int sec = _tick % 60;
	int min = (_tick / 60) % 60;
	int hour = (_tick / 3600) % 24;
	time_buf[7] = '0' + sec % 10;
	time_buf[6] = '0' + sec / 10;
	time_buf[4] = '0' + min % 10;
	time_buf[3] = '0' + min / 10;
	time_buf[1] = '0' + hour % 10;
	time_buf[0] = '0' + hour / 10;
	if (_tick != 0) {
		printf("%c%c:%c%c:%c%c", back_buf[0], back_buf[1], back_buf[2], back_buf[3], back_buf[4], back_buf[5]);
	}
	printf("Hardware time: ");
	printf("%c%c:%c%c:%c%c\n", time_buf[0], time_buf[1], time_buf[3], time_buf[4], time_buf[6], time_buf[7]);
}

void timer_handler() 
{
	timer_print();
	_tick++;

	// 重新设置下一次中断的时间
	timer_load(TIMER_INTERVAL);

	// 调度
	// schedule();
}
