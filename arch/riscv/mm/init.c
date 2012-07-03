#include <linux/init.h>
#include <linux/mm.h>

unsigned long empty_zero_page;

void __init mem_init(void)
{
}

void free_initmem(void)
{
}

#define __page_aligned(order) \
	__attribute__((__aligned__(PAGE_SIZE << (order))))

pgd_t swapper_pg_dir[PTRS_PER_PGD] __page_aligned(0);
