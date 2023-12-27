#include "os.h"

#define DELAY 1000

void user_task0(void)
{
	uart_puts("Task 0: Created!\n");

	// task_yield();
	// uart_puts("Task 0: I'm back!\n");
	while (1) {
		uart_puts("Task 0: Running...\n");
		// task_yield();
		task_delay(DELAY);
	}
}

void user_task1(void)
{
	uart_puts("Task 1: Created!\n");
	while (1) {
		uart_puts("Task 1: Running...\n");
		// task_yield();
		task_delay(DELAY);
	}
}

void user_task2(void)
{
	uart_puts("Task 2: Created!\n");
	while (1) {
		uart_puts("Task 2: Running...\n");
		// task_yield();
		task_delay(DELAY);
	}
}

void user_task3(void)
{
	uart_puts("Task 3: Created!\n");
	while (1) {
		uart_puts("Task 3: Running...\n");
		// task_yield();
		task_delay(DELAY);
	}
}

/* NOTICE: DON'T LOOP INFINITELY IN main() */
void os_main(void)
{
	task_create(user_task0, 1);
	task_create(user_task1, 2);
	task_create(user_task2, 3);
	task_create(user_task3, 4);
}

