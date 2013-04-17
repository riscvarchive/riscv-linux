#include <linux/mm.h>
#include <linux/kernel.h>

#include <asm/pgalloc.h>
#include <asm/pcr.h>

/* Handle a fault in the vmalloc area */
static int vmalloc_fault(unsigned long addr)
{
	return -1;
}

void do_page_fault(unsigned long cause, unsigned long epc,
	unsigned long badvaddr)
{
	struct task_struct *tsk;
	struct vm_area_struct *vma;
	struct mm_struct *mm;
	unsigned long ksp;
	struct pt_regs *regs;
	unsigned long addr, fault;
	unsigned int write, flags;
	siginfo_t info;

	write = (cause == EXC_STORE_ACCESS);
	flags = FAULT_FLAG_ALLOW_RETRY | FAULT_FLAG_KILLABLE
		| (write ? FAULT_FLAG_WRITE : 0);

	addr = (cause == EXC_INST_ACCESS) ? epc : badvaddr;
	tsk = current;
	mm = tsk->mm;

	down_read(&mm->mmap_sem);
	vma = find_vma(mm, addr);

	/* Check for fault in kernel space */
	if (unlikely(addr >= TASK_SIZE)) {
		if (vmalloc_fault(addr) >= 0)
			return;
	}

	if (unlikely(!vma)) {
		panic("bad vm area, address was 0x%0lx, epc was 0x%0lx", addr, epc);
	}
	if (write && !(vma->vm_flags & VM_WRITE)) {
		panic("write but vm not writable");
	}
	
	fault = 0;
	if (likely(vma->vm_start <= addr)) {
		unsigned long cycle;
		__asm__ volatile ("rdcycle %0" : "=r" (cycle));
		fault = handle_mm_fault(mm, vma, addr, flags);
//		printk(KERN_DEBUG "handle_mm_fault: returned 0x%lx"
//			" for address 0x%p, pc 0x%p, cycle %lu\n", fault, (void *)addr, (void *)epc, cycle);
		goto good_area;
	}
	if (unlikely(!(vma->vm_flags & VM_GROWSDOWN))) {
		goto bad_area;
	}

	fault = expand_stack(vma, addr);
	if (fault) {
		printk(KERN_DEBUG "expand_stack: returned %ld\n", fault);
		goto bad_area;
	}

	if (fault & (VM_FAULT_RETRY | VM_FAULT_ERROR)) {
		goto bad_area;
	}
	goto good_area;

bad_area:
	/* Obtain pt_regs of process */
	ksp = (unsigned long)task_stack_page(tsk) + THREAD_SIZE;
	regs = (struct pt_regs *)ksp - 1;

	if (user_mode(regs)) {
		info.si_signo = SIGSEGV;
		info.si_errno = 0;
		info.si_addr = (void *)addr;
//		force_sig_info(SIGSEGV, &info, tsk);
//		return;
	}

	panic("inescapable page fault at 0x%p, pc 0x%p\n",
		(void *)addr, (void *)epc);	

good_area:
	up_read(&mm->mmap_sem);
}
