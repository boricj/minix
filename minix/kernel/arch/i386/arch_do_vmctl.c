/* The kernel call implemented in this file:
 *   m_type:	SYS_VMCTL
 *
 * The parameters for this kernel call are:
 *   	SVMCTL_WHO	which process
 *    	SVMCTL_PARAM	set this setting (VMCTL_*)
 *    	SVMCTL_VALUE	to this value
 */

#include "kernel/system.h"
#include <assert.h>
#include <minix/type.h>

#include "arch_proto.h"

extern phys_bytes video_mem_vaddr;

extern char *video_mem;

u64_t pagedirtables[NR_TASKS + NR_PROCS][4] __aligned(32);

static void setcr3(struct proc *p, u32_t cr3, u32_t *v)
{
	int i;

	/*
	 * Fill in the page directory pointer tables, because we fold them into the
	 * page directory tables to work around VM's two-layer page layout.
	 */
	for (i = 0; i < 4; i++) {
		pagedirtables[p->p_nr][i] = (cr3+i*4096) | I386_VM_PRESENT;
	}
	cr3 = (u32_t)&pagedirtables[p->p_nr];

	/* Set process CR3. */
	p->p_seg.p_cr3 = cr3;
	assert(p->p_seg.p_cr3);
	p->p_seg.p_cr3_v = v;
	if(p == get_cpulocal_var(ptproc)) {
		write_cr3(p->p_seg.p_cr3);
	}
	if(p->p_nr == VM_PROC_NR) {
		if (arch_enable_paging(p) != OK)
			panic("arch_enable_paging failed");
	}
	RTS_UNSET(p, RTS_VMINHIBIT);
}

/*===========================================================================*
 *				arch_do_vmctl				     *
 *===========================================================================*/
int arch_do_vmctl(
  register message *m_ptr,	/* pointer to request message */
  struct proc *p
)
{
  switch(m_ptr->SVMCTL_PARAM) {
	case VMCTL_GET_PDBR:
		/* Get process page directory base reg (CR3). */
		m_ptr->SVMCTL_VALUE = p->p_seg.p_cr3;
		return OK;
	case VMCTL_SETADDRSPACE:
		setcr3(p, m_ptr->SVMCTL_PTROOT, (u32_t *) m_ptr->SVMCTL_PTROOT_V);
		return OK;
	case VMCTL_FLUSHTLB:
	{
		reload_cr3();
		return OK;
	}
	case VMCTL_I386_INVLPG:
	{
		i386_invlpg(m_ptr->SVMCTL_VALUE);
		return OK;
	}
  }



  printf("arch_do_vmctl: strange param %d\n", m_ptr->SVMCTL_PARAM);
  return EINVAL;
}
