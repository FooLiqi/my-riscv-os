#include "syscall.h"

.global gethid
gethid:
	li a7, SYS_gethid		// 设置系统调用号
	ecall					// 系统调用
	ret
