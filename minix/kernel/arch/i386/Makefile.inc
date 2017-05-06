# Makefile for arch-dependent kernel code
.include <bsd.own.mk>

HERE=${.CURDIR}/arch/${MACHINE_ARCH}
.PATH:	${HERE}

# LSC: putchar and kputc have the same signature. A bit hackish.
CPPFLAGS.subr_prf.c+= -Dputchar=kputc

# Activate optional support, may be deactivated.
CPPFLAGS.subr_prf.c+= -DLIBSA_PRINTF_LONGLONG_SUPPORT -DLIBSA_PRINTF_WIDTH_SUPPORT

# some object files we give a symbol prefix (or namespace) of __k_unpaged_
# that must live in their own unique namespace.
#
.for unpaged_obj in head.o

CLEANFILES+=${unpaged_obj}.bin
.if ${USE_BITCODE:Uno} == "yes"
unpaged_${unpaged_obj}: ${unpaged_obj}
	if file -b ${.OBJDIR}/${unpaged_obj} | grep -q '^LLVM'; then \
		${LLC} -O1 -march=x86 -mcpu=i586 -filetype=obj -o ${.OBJDIR}/${unpaged_obj}.bin ${.OBJDIR}/${unpaged_obj}; \
	else \
		cp ${.OBJDIR}/${unpaged_obj} ${.OBJDIR}/${unpaged_obj}.bin; \
	fi
	${OBJCOPY} --prefix-symbols=__k_unpaged_ ${.OBJDIR}/${unpaged_obj}.bin $@
.else
unpaged_${unpaged_obj}: ${unpaged_obj}
	${OBJCOPY} --prefix-symbols=__k_unpaged_ ${.OBJDIR}/${unpaged_obj} $@
.endif
UNPAGED_OBJS += unpaged_${unpaged_obj}
ORIG_UNPAGED_OBJS += ${unpaged_obj}
.endfor

CLEANFILES+= ${ORIG_UNPAGED_OBJS}

SRCS+=	mpx.S arch_clock.c arch_do_vmctl.c arch_system.c \
	klib.S memory.c protect.c direct_tty_utils.c arch_reset.c pg_utils.c \
	pre_init.c

SRCS+=	do_iopenable.c do_readbios.c do_sdevio.c exception.c i8259.c oxpcie.c \
	usermapped_data_arch.c \
	io_inb.S io_inl.S io_intr.S io_inw.S io_outb.S io_outl.S io_outw.S \
	usermapped_glo_ipc.S

OBJS.kernel+=	${UNPAGED_OBJS}

.ifdef CONFIG_SMP
SRCS += arch_smp.c trampoline.S
.endif

.if ${USE_ACPI} != "no"
SRCS+= acpi.c
CPPFLAGS+= -DUSE_ACPI
.endif

.if ${USE_APIC} != "no"
SRCS+= apic.c apic_asm.S
CPPFLAGS+= -DUSE_APIC
.endif

.if ${USE_DEBUGREG} != "no"
SRCS+=  breakpoints.c debugreg.S
.endif

.if ${USE_WATCHDOG} != "no"
SRCS+= arch_watchdog.c
CPPFLAGS+= -DUSE_WATCHDOG
.endif

klib.o mpx.o head.o: procoffsets.h

SRCS+= procoffsets.h

PROCOFFSETSCF=procoffsets.cf

.PATH: ${NETBSDSRCDIR}/minix/include/arch/${MACHINE_ARCH}/include

procoffsets.h: ${PROCOFFSETSCF} kernel.h proc.h stackframe.h archtypes.h
	${_MKTARGET_CREATE}
	${TOOL_CAT} ${HERE}/${PROCOFFSETSCF} | \
		${TOOL_GENASSYM} -- ${CC} ${CFLAGS:N-Wa,*} \
		${CPPFLAGS} ${PROF} ${${USE_BITCODE:Uno} == "yes":? -fno-lto:} \
		${GENASSYM_CPPFLAGS} >$@.tmp && \
	  mv -f $@.tmp $@ 

sconst.h: procoffsets.h
apic_asm.o head.o klib.o mpx.o: sconst.h

