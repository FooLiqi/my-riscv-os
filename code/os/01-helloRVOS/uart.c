#include "types.h"
#include "platform.h"

/*
 * The UART control registers are memory-mapped at address UART0. 
 * This macro returns the address of one of the registers.
 */
// c语言宏定义,得到寄存器的值
// 用volatile声明的变量表示该变量随时可能发生变化，与该变量有关的运算，不要进行编译优化，以免出错
// 这里得到的是一个指针. UART0是一个地址, reg是一个偏移量
#define UART_REG(reg) ((volatile uint8_t *)(UART0 + reg))

/*
 * Reference
 * [1]: TECHNICAL DATA ON 16550, http://byterunner.com/16550.html
 */

/*
 * UART control registers map. see [1] "PROGRAMMING TABLE"
 * note some are reused by multiple functions
 * 0 (write mode): THR/DLL
 * 1 (write mode): IER/DLM
 */
// 串口对应的寄存器
#define RHR 0	// Receive Holding Register (read mode)
#define THR 0	// Transmit Holding Register (write mode)
#define DLL 0	// LSB of Divisor Latch (write mode)
#define IER 1	// Interrupt Enable Register (write mode)
#define DLM 1	// MSB of Divisor Latch (write mode)
#define FCR 2	// FIFO Control Register (write mode)
#define ISR 2	// Interrupt Status Register (read mode)
#define LCR 3	// Line Control Register
#define MCR 4	// Modem Control Register
#define LSR 5	// Line Status Register
#define MSR 6	// Modem Status Register
#define SPR 7	// ScratchPad Register

/*
 * POWER UP DEFAULTS
 * IER = 0: TX/RX holding register interrupts are both disabled
 * ISR = 1: no interrupt penting
 * LCR = 0
 * MCR = 0
 * LSR = 60 HEX
 * MSR = BITS 0-3 = 0, BITS 4-7 = inputs
 * FCR = 0
 * TX = High
 * OP1 = High
 * OP2 = High
 * RTS = High
 * DTR = High
 * RXRDY = High
 * TXRDY = Low
 * INT = Low
 */

/*
 * LINE STATUS REGISTER (LSR)
 * LSR BIT 0:
 * 0 = no data in receive holding register or FIFO.
 * 1 = data has been receive and saved in the receive holding register or FIFO.
 * ......
 * LSR BIT 5:
 * 0 = transmit holding register is full. 16550 will not accept any data for transmission.
 * 1 = transmitter hold register (or FIFO) is empty. CPU can load the next character.
 * ......
 */
#define LSR_RX_READY (1 << 0)								// 接收缓冲区非空
#define LSR_TX_IDLE  (1 << 5)								// 发送缓冲区为空

#define uart_read_reg(reg) (*(UART_REG(reg)))				// 读取寄存器的值
#define uart_write_reg(reg, v) (*(UART_REG(reg)) = (v))		// 写入寄存器的值

void uart_init()
{
	/* disable interrupts. */
	uart_write_reg(IER, 0x00);

	/*
	 * Setting baud rate. Just a demo here if we care about the divisor,
	 * but for our purpose [QEMU-virt], this doesn't really do anything.
	 *
	 * Notice that the divisor register DLL (divisor latch least) and DLM (divisor
	 * latch most) have the same base address as the receiver/transmitter and the
	 * interrupt enable register. To change what the base address points to, we
	 * open the "divisor latch" by writing 1 into the Divisor Latch Access Bit
	 * (DLAB), which is bit index 7 of the Line Control Register (LCR).
	 *
	 * Regarding the baud rate value, see [1] "BAUD RATE GENERATOR PROGRAMMING TABLE".
	 * We use 38.4K when 1.8432 MHZ crystal, so the corresponding value is 3.
	 * And due to the divisor register is two bytes (16 bits), so we need to
	 * split the value of 3(0x0003) into two bytes, DLL stores the low byte,
	 * DLM stores the high byte.
	 */
	// 设置波特率
	uint8_t lcr = uart_read_reg(LCR);
	uart_write_reg(LCR, lcr | (1 << 7)); 				// 把第7位设置为1,这样dll和dlm就可以用来设置波特率了
	uart_write_reg(DLL, 0x03);
	uart_write_reg(DLM, 0x00);

	/*
	 * Continue setting the asynchronous data communication format.
	 * - number of the word length: 8 bits
	 * - number of stop bits：1 bit when word length is 8 bits
	 * - no parity
	 * - no break control
	 * - disabled baud latch
	 */
	// 设置数据格式
	lcr = 0;
	uart_write_reg(LCR, lcr | (3 << 0));
}

int uart_putc(char ch)
{
	while ((uart_read_reg(LSR) & LSR_TX_IDLE) == 0);		// 等待发送缓冲区为空
	return uart_write_reg(THR, ch);							// 发送字符
}

void uart_puts(char *s)
{
	while (*s) {							// 如果字符串未结束
		uart_putc(*s++);					// 发送*s, 并将s指向下一个字符
	}
}

// 实现从串口读取一个字符
char uart_getc(void)
{
	while ((uart_read_reg(LSR) & LSR_RX_READY) == 0);		// 等待接收缓冲区非空
	return uart_read_reg(RHR);							    // 返回接收字符
}
