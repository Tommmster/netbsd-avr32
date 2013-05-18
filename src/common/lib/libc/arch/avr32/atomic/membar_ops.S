#include "atomic_op_asm.h"

	.text
	.align  1
	.globl  _membar_sync
	.type   _membar_sync, @function
_membar_sync:
	stm     --sp, r7, lr
	ldm     sp++, r7, pc
	.size   _membar_sync, . - _membar_sync

ATOMIC_OP_ALIAS(membar_sync,_membar_sync)
ATOMIC_OP_ALIAS(membar_enter,_membar_sync)
STRONG_ALIAS(_membar_enter,_membar_sync)
ATOMIC_OP_ALIAS(membar_exit,_membar_sync)
STRONG_ALIAS(_membar_exit,_membar_sync)
ATOMIC_OP_ALIAS(membar_producer,_membar_sync)
STRONG_ALIAS(_membar_producer,_membar_sync)
ATOMIC_OP_ALIAS(membar_consumer,_membar_sync)
STRONG_ALIAS(_membar_consumer,_membar_sync)
