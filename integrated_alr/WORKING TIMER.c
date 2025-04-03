#define TIMER_BASE 0xFF202000
#define RED_LED_BASE 0xFF200000

void interrupt_handler(void);
void interval_timer_ISR(void);
void pushbutton_ISR(void);
void the_reset(void) __attribute__((section(".reset")));
void the_reset(void)

{
asm(".set noat"); /* Instruct the assembler NOT to use reg at (r1) as
* a temp register for performing optimizations */
asm(".set nobreak"); /* Suppresses a warning message that says that
* some debuggers corrupt regs bt (r25) and ba
* (r30)
*/
asm("movia r2, main"); // Call the C language main program
asm("jmp r2");
}
void the_exception(void) __attribute__((section(".exceptions")));
void the_exception(void)
{
asm("subi sp, sp, 128");
asm("stw et, 96(sp)");
asm("rdctl et, ctl4");
asm("beq et, r0, SKIP_EA_DEC"); // Interrupt is not external
asm("subi ea, ea, 4"); /* Must decrement ea by one instruction
* for external interupts, so that the
* interrupted instruction will be run */
asm("SKIP_EA_DEC:");
asm("stw r1, 4(sp)"); // Save all registers
asm("stw r2, 8(sp)");	
asm("stw r3, 12(sp)");
asm("stw r4, 16(sp)");
asm("stw r5, 20(sp)");
asm("stw r6, 24(sp)");
asm("stw r7, 28(sp)");
asm("stw r8, 32(sp)");
asm("stw r9, 36(sp)");
asm("stw r10, 40(sp)");
asm("stw r11, 44(sp)");
asm("stw r12, 48(sp)");
asm("stw r13, 52(sp)");
asm("stw r14, 56(sp)");
asm("stw r15, 60(sp)");
asm("stw r16, 64(sp)");
asm("stw r17, 68(sp)");
asm("stw r18, 72(sp)");
asm("stw r19, 76(sp)");
asm("stw r20, 80(sp)");
asm("stw r21, 84(sp)");
asm("stw r22, 88(sp)");
asm("stw r23, 92(sp)");
asm("stw r25, 100(sp)"); // r25 = bt (skip r24 = et, because it is saved
// above)
asm("stw r26, 104(sp)"); // r26 = gp
// skip r27 because it is sp, and there is no point in saving this
asm("stw r28, 112(sp)"); // r28 = fp
asm("stw r29, 116(sp)"); // r29 = ea
asm("stw r30, 120(sp)"); // r30 = ba
asm("stw r31, 124(sp)"); // r31 = ra
asm("addi fp, sp, 128");
asm("call interrupt_handler"); // Call the C language interrupt handler
asm("ldw r1, 4(sp)"); // Restore all registers
asm("ldw r2, 8(sp)");
asm("ldw r3, 12(sp)");
asm("ldw r4, 16(sp)");
asm("ldw r5, 20(sp)");
asm("ldw r6, 24(sp)");
asm("ldw r7, 28(sp)");
asm("ldw r8, 32(sp)");
asm("ldw r9, 36(sp)");
asm("ldw r10, 40(sp)");
asm("ldw r11, 44(sp)");
asm("ldw r12, 48(sp)");
asm("ldw r13, 52(sp)");
asm("ldw r14, 56(sp)");
asm("ldw r15, 60(sp)");
asm("ldw r16, 64(sp)");
asm("ldw r17, 68(sp)");
asm("ldw r18, 72(sp)");
asm("ldw r19, 76(sp)");
asm("ldw r20, 80(sp)");
asm("ldw r21, 84(sp)");
asm("ldw r22, 88(sp)");
asm("ldw r23, 92(sp)");
asm("ldw r24, 96(sp)");
asm("ldw r25, 100(sp)"); // r25 = bt
asm("ldw r26, 104(sp)"); // r26 = gp
// skip r27 because it is sp, and we did not save this on the stack
asm("ldw r28, 112(sp)"); // r28 = fp
asm("ldw r29, 116(sp)"); // r29 = ea
asm("ldw r30, 120(sp)"); // r30 = ba
asm("ldw r31, 124(sp)"); // r31 = ra
asm("addi sp, sp, 128");
asm("eret");
}
int count=0;
int main(void) {
volatile int * interval_timer_ptr =(int *)TIMER_BASE; // interal timer base address
int unsigned counter = 50000000; // 1/(50 MHz) x (2500000) = 50 msec
*(interval_timer_ptr + 0x2) = (counter & 0xFFFF);
*(interval_timer_ptr + 0x3) = (counter >> 16) & 0xFFFF;
*(interval_timer_ptr + 1) = 0x7; // STOP = 0, START = 1, CONT = 1, ITO = 1
//*(KEY_ptr + 2) = 0x3; // enable interrupts for all pushbuttons
 __builtin_wrctl(3, 0b1);
__builtin_wrctl(0, 1); // enable Nios II interrupts
int* leds = (int*) RED_LED_BASE;

while (1){} // main program simply idles
}


void interrupt_handler(void) {
	int ipending;
	ipending = __builtin_rdctl(4);
		
	if (ipending & 0x1) // interval timer is interrupt level 0
	{
	interval_timer_ISR();
	}	// else, ignore the interrupt
	return;
}

void interval_timer_ISR() {
	volatile unsigned int* interval_timer_ptr = 0xFF202000;
	*(interval_timer_ptr) = 0; // clear the interrupt
	int* leds = (int*) RED_LED_BASE;
	count++;
	*leds=count;
	if (count>=100){
		count = 0;
	}

	
}
