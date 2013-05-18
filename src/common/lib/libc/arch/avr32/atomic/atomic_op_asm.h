#ifndef _ATOMIC_OP_ASM_H_
#define	_ATOMIC_OP_ASM_H_

#include <machine/asm.h>

#if defined(_KERNEL)

#define	ATOMIC_OP_ALIAS(a,s)	STRONG_ALIAS(a,s)

#else /* _KERNEL */

#define	ATOMIC_OP_ALIAS(a,s)	WEAK_ALIAS(a,s)

#endif /* _KERNEL */

#endif /* _ATOMIC_OP_ASM_H_ */
