#include "os.h"

#include "user_api.h"

#define DELAY 4000
// #define CONFIG_SYSCALL

void user_task0(void)
{
	uart_puts("Task 0: Created!\n");

	unsigned int hid = -1;
	unsigned int time = -1;

	/*
	 * if syscall is supported, this will trigger exception, 
	 * code = 2 (Illegal instruction)
	 */
	//hid = r_mhartid();
	//printf("hart id is %d\n", hid);

#ifdef CONFIG_SYSCALL
	int ret = -1;
	// 获取当前hart id
	ret = gethid(&hid);
	// ret = gethid(NULL);
	// printf("gethid() return: %d\n", ret);
	if (!ret) {
		printf("system call returned!, hart id is %d\n", hid);
	} else {
		printf("gethid() failed, return: %d\n", ret);
	}

	// 获取当前系统时间
	ret = gettime(&time);
	if (!ret) {
		printf("system call returned!, time is %d\n", time);
	} else {
		printf("gettime() failed, return: %d\n", ret);
	}

	// 打印字符串
	myprintf("%c", '\n');
	myprintfWithoutArgument("你好\n");
#endif

	while (1){
		uart_puts("Task 0: Running... \n");
		task_delay(DELAY);
	}
}

void user_task1(void)
{
	uart_puts("Task 1: Created!\n");
	while (1) {
		uart_puts("Task 1: Running... \n");
		task_delay(DELAY);
	}
}


void user_compile(void)
{
    const char *code = "void func1()\n{\nmyprintfWithoutArgument(\"helloworld\");\nmyprintfWithoutArgument(\"second statement\");myprintfWithoutArgument(\"你好,我是fjq\");\n}\n";

	// printf("get mastatus\n");
	// reg_t mstatus = r_mstatus();
	// printf("mstatus2: %x\n", mstatus);

#ifdef CONFIG_SYSCALL
    int ret = -1;
    ret = compileCode(code);

    if (!ret)
    {
        printf("system call returned!, compile success!\n");
    }
    else
    {
        printf("compile() failed, return: %d\n", ret);
    }
#endif

    while(1) {
        uart_puts("User Compile: Running... \n");
        task_delay(DELAY);
    };
}

/* NOTICE: DON'T LOOP INFINITELY IN main() */
void os_main(void)
{
	// task_create(user_task0);
	// task_create(user_task1);
	task_create(user_compile);
}

