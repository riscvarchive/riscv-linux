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
	unsigned long addr, fault;
	unsigned int write, flags;

	write = (epc == EXC_STORE_ACCESS);
	flags = FAULT_FLAG_ALLOW_RETRY | FAULT_FLAG_KILLABLE
		| (write ? FAULT_FLAG_WRITE : 0);

	addr = (cause == EXC_INST_ACCESS) ? epc : badvaddr;
	tsk = current;
	mm = tsk->mm;
	vma = find_vma(mm, addr);

	/* Check for fault in kernel space */
	if (unlikely(addr >= TASK_SIZE)) {
		if (vmalloc_fault(addr) >= 0)
			return;
	}

	if (unlikely(!vma)) {
		panic("bad vm area");
	}
	if (write && !(vma->vm_flags & VM_WRITE)) {
		panic("write but vm not writable");
	}
	
	fault = 0;
	if (likely(vma->vm_start <= addr)) {
		fault = handle_mm_fault(mm, vma, addr, flags);
		printk(KERN_DEBUG "handle_mm_fault: returned 0x%lx"
			" for address 0x%p\n", fault, (void *)addr);
	}

	if (fault & (VM_FAULT_RETRY | VM_FAULT_ERROR)) {
		panic("inescapable page fault at 0x%p, pc 0x%p\n",
			(void *)addr, (void *)epc);	
	}
}
