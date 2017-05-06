#include "kernel/kernel.h" /* configures the kernel */

/* sections */

#include <machine/asm.h>
#include <machine/multiboot.h>
#include <machine/vm.h>
#include <machine/specialreg.h>

#ifdef CONFIG_SMP
#include "kernel/smp.h"
#endif

.text
.global MINIX
MINIX:
/*
 * This is the entry point for the MINIX kernel.
 *
 * The multiboot specification guarantees us, among other things, the following:
 *  * EAX: must be 0x2BADB002
 *  * EBX: address of the multiboot information structure
 *  * CRO: paging disabled, protected mode enabled
 *  * EFLAGS: interrupt enable flag cleared
 *  * we're loaded at 0x00200000
 *
 * We want to set up basic paging, a stack and jump into C code. Memory will be
 * identity-mapped everywhere, except for the highly-mapped kernel starting at
 * 0xFxxxxxxx for 32 MiB, so the kernel shouldn't be bigger than 30 MiB. This
 * page table is temporary as the C code will make its own page table later on.
 *
 * We assume PAE is available, so the kernel requires an i686-class processor.
 * Unless somebody tries to run this on a mid-90s Pentium processor or earlier,
 * we're fine.
 */

	mov $0, %ecx        /* ecx is out PDPE counter */
	mov $initial_paging_pdt, %esi
	mov $initial_paging_pdpt, %edi

pdpe_setup_loop:
	mov $0, %edx
	mov %edx, 4(%edi)   /* clear upper 4 bytes of table entry */
	mov %esi, %edx      /* grab address of PDE */
	or $0x1, %edx       /* set Present bit */
	mov %edx, (%edi)    /* store into lower 4 bytes of table entry */

	add $0x1000, %esi   /* add offset to the 512th next PDE */
	add $8, %edi        /* add offset to the next PDPE */
	inc %ecx
	cmp $4, %ecx
	jne pdpe_setup_loop

	mov $0, %ecx        /* ecx is out PDE counter */
	mov $0x00000000, %esi
	mov $initial_paging_pdt, %edi

pte_setup_loop:
	mov $0, %edx
	mov %edx, 4(%edi)  /* clear upper 4 bytes of table entry */
	mov %esi, %edx     /* grab physical address */

	cmp $0xF0000000, %edx
	jb pte_setup_physical_address
	cmp $0xF2000000, %edx
	ja pte_setup_physical_address
	and $0x0FFFFFFF, %edx /* clear top bits to fall back to kernel */
pte_setup_physical_address:
	or $0x83, %edx      /* set Present, R/W and Page Size bits */
	mov %edx, (%edi)   /* store into lower 4 bytes of table entry */

	add $0x200000, %esi /* add offset to the next 2 MiB */
	add $8, %edi        /* add offset to the next PTE */
	inc %ecx
	cmp $2048, %ecx
	jne pte_setup_loop

/* Page table are set, enable paging */

	mov $initial_paging_pdpt, %ecx
	mov %ecx, %cr3               /* load PDPE address */

	mov %cr4, %ecx
	or $0x20, %ecx               /* set Physical-Address Extension bit */
	mov %ecx, %cr4

	mov %cr0, %ecx
	or $0x80000000, %ecx         /* set Paging Enable bit */
	mov %ecx, %cr0

/* Paging is enabled, set up stack and flags */

	mov $0, %ebp                 /* no meaningful ebp for us */
	mov $k_initial_stktop, %esp /* set stack pointer */

	push $0                      /* terminate stack */
	push %ebx                   /* multiboot information struct */
	push %eax                   /* multiboot magic number */

	push $0                      /* set flags to known good state */
	popf

/* Jump into kernel */

	call _C_LABEL(pre_init)
	push %eax                   /* push kinfo_t structure address returned */
	call _C_LABEL(kmain)

	/* not reached */
hang:
	jmp hang

/*
 * Multiboot header
 */

.balign 8

multiboot_header:
	.long MULTIBOOT_HEADER_MAGIC
/* flags */
#define MULTIBOOT_FLAGS (MULTIBOOT_HEADER_WANT_MEMORY | MULTIBOOT_HEADER_MODS_ALIGNED)
	.long MULTIBOOT_FLAGS
/* checksum */
	.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_FLAGS)


.data

/*
 * Page tables
 */

/* PDT (level 2) */
.balign 4096
initial_paging_pdt:
	.space 4*512*8

/* PDPT (level 3) */
.balign 8
initial_paging_pdpt:
	.space 4*8
