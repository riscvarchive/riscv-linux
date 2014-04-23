#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/perf_event.h>

#include <asm/pgalloc.h>
#include <asm/ptrace.h>
#include <asm/uaccess.h>

extern void die(char *, struct pt_regs *, long);

asmlinkage void do_page_fault(struct pt_regs *regs)
{
	struct task_struct *tsk;
	struct vm_area_struct *vma;
	struct mm_struct *mm;
	unsigned long addr, epc, cause, fault;
	unsigned int write, flags;
	siginfo_t info;

	cause = regs->cause;
	write = (cause == EXC_STORE_ACCESS);
	flags = FAULT_FLAG_ALLOW_RETRY | FAULT_FLAG_KILLABLE
		| (write ? FAULT_FLAG_WRITE : 0);
	epc = regs->epc;
	addr = (cause == EXC_INST_ACCESS) ? epc : regs->badvaddr;

	info.si_code = SEGV_MAPERR;

	tsk = current;
	mm = tsk->mm;

	/*
	 * Fault-in kernel-space virtual memory on-demand.
	 * The 'reference' page table is init_mm.pgd.
	 *
	 * NOTE! We MUST NOT take any locks for this case. We may
	 * be in an interrupt or a critical region, and should
	 * only copy the information from the master page table,
	 * nothing more.
	 */
	if (unlikely((addr >= VMALLOC_START) && (addr <= VMALLOC_END)))
		goto vmalloc_fault;

	/* Do not take the fault if within an interrupt if SR_EA is disabled
	   or if lacking a user context */
	if (!mm || (in_atomic() && !(regs->status & SR_EA)))
		goto no_context;

retry:
	down_read(&mm->mmap_sem);
	vma = find_vma(mm, addr);
	if (unlikely(!vma))
		goto bad_area;
	if (likely(vma->vm_start <= addr))
		goto good_area;
	if (unlikely(!(vma->vm_flags & VM_GROWSDOWN)))
		goto bad_area;
	if (unlikely(expand_stack(vma, addr)))
		goto bad_area;

good_area:
	info.si_code = SEGV_ACCERR;

	if (unlikely(write && (!(vma->vm_flags & VM_WRITE)))) {
		goto bad_area;
	}
	if (unlikely((cause == EXC_INST_ACCESS)
		&& (!(vma->vm_flags & VM_EXEC)))) {
		goto bad_area;
	}

	/*
	 * If for any reason at all we could not handle the fault,
	 * make sure we exit gracefully rather than endlessly redo
	 * the fault.
	 */
	fault = handle_mm_fault(mm, vma, addr, flags);

	if ((fault & VM_FAULT_RETRY) && fatal_signal_pending(tsk))
		return;


	perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS, 1, regs, addr);
	if (unlikely(fault & VM_FAULT_ERROR)) {
		if (fault & VM_FAULT_OOM) {
			goto out_of_memory;
		} else if (fault & VM_FAULT_SIGBUS) {
			goto do_sigbus;
		}
		BUG();
	}

	if (flags & FAULT_FLAG_ALLOW_RETRY) {
		if (fault & VM_FAULT_MAJOR) {
			perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS_MAJ, 1, regs, addr);
			tsk->maj_flt++;
		} else {
			perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS_MIN, 1, regs, addr);
			tsk->min_flt++;
		}
		if (fault & VM_FAULT_RETRY) {
			flags &= ~(FAULT_FLAG_ALLOW_RETRY);
//			flags |= FAULT_FLAG_TRIED;

			/*
			 * No need to up_read(&mm->mmap_sem) as we would
			 * have already released it in __lock_page_or_retry
			 * in mm/filemap.c.
			 */
			goto retry;
		}
	}

	up_read(&mm->mmap_sem);
	return;

bad_area:
	up_read(&mm->mmap_sem);
	/* User mode accesses cause a SIGSEGV */
	if (user_mode(regs)) {
		info.si_signo = SIGSEGV;
		info.si_errno = 0;
		/* info.si_code has been set above */
		info.si_addr = (void __user *)addr;
		force_sig_info(SIGSEGV, &info, tsk);
		return;
	}

no_context:
	/* Are we prepared to handle this fault as an exception? */
	if (fixup_exception(regs, epc)) {
		return;
	}
	printk(KERN_ALERT "Unable to handle kernel paging request at "
		"virtual address 0x%016lx, epc=0x%016lx", addr, epc);
	die("Oops", regs, 0);

out_of_memory:
	up_read(&mm->mmap_sem);
	if (!user_mode(regs))
		goto no_context;
	pagefault_out_of_memory();
	return;

do_sigbus:
	up_read(&mm->mmap_sem);
	/* Send a SIGBUS regardless of kernel or user mode */
	info.si_signo = SIGBUS;
	info.si_errno = 0;
	info.si_code = BUS_ADRERR;
	info.si_addr = (void __user *)addr;
	force_sig_info(SIGBUS, &info, current);
	if (!user_mode(regs))
		goto no_context;
	return;

vmalloc_fault:
	if (user_mode(regs))
		goto bad_area;
	/* TODO */
	panic("do_page_fault: vmalloc_fault unimplemented");
	return;
}
