#include "os.h"
#include "syscall.h"

extern int if_equal_to_myprintfWithoutArgument(const char *s);

int sys_gethid(unsigned int *ptr_hid)
{
	printf("--> sys_gethid, arg0 = 0x%x\n", ptr_hid);
	if (ptr_hid == NULL) {
		return -1;
	} else {
		*ptr_hid = r_mhartid();
		return 0;
	}
}

int sys_gettime(unsigned int *ptr_time)
{
	printf("--> sys_gettime, arg0 = 0x%x\n", ptr_time);
	if (ptr_time == NULL) {
		return -1;
	} else {
		int time = *(uint64_t*)CLINT_MTIME;
		*ptr_time = *(uint64_t*)CLINT_MTIME;
		printf("time now: %d\n", time);
		return 0;
	}
}

int sys_compile(char *code)
{
	char s[10][100]; 			// 最多10个printf
    int scount = 0;				// 统计printf个数
    int len = 0; 				// 统计长度

    while (code[len] != '\0')
    {
        len++;
    }
    printf("code length: %d\n", len);
    for (int i = 0; i < len; i++)
    {
        if (i < len - 23 && if_equal_to_myprintfWithoutArgument(&code[i]))
        {
            i += 24;
            int slen = 0;
            while (code[i] != ')')				// 解析字符串
            {
                s[scount][slen] = code[i];
                slen++;
                i++;
            };
            s[scount][slen] = '\0';
            scount++;
        }
    };
    printf("\n\n.extern  myprintfWithoutArgument\n.global func1\n");
    printf(".data\n");
    for (int i = 0; i < scount; i++)
    {
        printf("str%d :.string %s\n", i, s[i]);
    }
    printf(".text\nfunc1 :\n");
    for (int i = 0; i < scount; i++)
    {
        printf("la a0,str%d\n", i);
        printf("call myprintfWithoutArgument\n");
    }
    printf("ret\n.end\n");

	return 0;
}

void do_syscall(struct context *cxt)
{
	uint32_t syscall_num = cxt->a7;

	switch (syscall_num) {
		case SYS_gethid:
			cxt->a0 = sys_gethid((unsigned int *)(cxt->a0));
			break;
		case SYS_gettime:
			cxt->a0 = sys_gettime((unsigned int *)(cxt->a0));
			break;
		case SYS_printfWithoutArgument: {
			char *s = (char *)cxt->a0;
			printf(s);
			printf("\n");
			// printf() ""
			break;
		}
		case SYS_printf: {
			char *s1 = (char *)cxt->a0;
			char *s2 = (char *)cxt->a1;
			printf(s1, s2);
			// printf("%d",1);
			break;
		}
		case SYS_compile: {
			cxt->a0 = sys_compile((char *)cxt->a0);
			break;
		}
		default:
			printf("Unknown syscall no: %d\n", syscall_num);
			cxt->a0 = -1;
	}

	return;
}