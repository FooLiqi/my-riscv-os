#include "syscall.h"

.global gethid
gethid:
	li a7, SYS_gethid		// 设置系统调用号
	ecall					// 系统调用
	ret

.global gettime
gettime:
	li a7, SYS_gettime		// 设置系统调用号
	ecall					// 系统调用
	ret

.global myprintf
myprintf:
	li a7, SYS_printf
	ecall
	ret

.global myprintfWithoutArgument
myprintfWithoutArgument:
	li a7, SYS_printfWithoutArgument
	ecall
	ret

.global compileCode
compileCode:
	li a7, SYS_compile
	ecall
	ret