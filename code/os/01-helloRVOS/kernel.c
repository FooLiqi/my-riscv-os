// 外部函数声明
extern void uart_init(void);
extern void uart_puts(char *s);
extern char uart_getc(void);
extern int uart_putc(char ch);

void start_kernel(void)
{
	uart_init();							// 初始化串口
	uart_puts("Hello, RVOS!\n");			// 打印 Hello, RVOS!

	while (1) {
		char ch = uart_getc();				// 获取字符
		if (ch == '\r') {					// 回车
			uart_puts("\n"); 				// 换行
		}
		else {
			uart_putc(ch);					// 回显
		}

	}; // stop here?
}

