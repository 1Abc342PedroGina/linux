/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_CLOCK_H
#define _LINUX_SCHED_CLOCK_H

#include <linux/smp.h>

/*
 * HÍBRIDO: Adicionando suporte a múltiplos domínios de tempo
 * Agora temos clock para:
 * - Kernel space (modo privilegiado)
 * - User space servers (servidores em user mode)
 * - IPC timing (para comunicação entre servidores)
 */

/* Estrutura para tracking de tempo em servidores userspace */
struct hybrid_server_time {
    u64 user_time;      /* Tempo executando em user space */
    u64 kernel_time;    /* Tempo em syscalls */
    u64 ipc_wait_time;  /* Tempo esperando IPC */
    u64 server_id;      /* ID do servidor */
};

/* Clock isolado para servidores em user mode */
extern u64 hybrid_server_clock(u64 server_id);

/* Clock para IPC entre servidores */
extern u64 hybrid_ipc_clock(void);

/* Clock original do kernel (modo monolítico) */
extern u64 sched_clock(void);

#if defined(CONFIG_ARCH_WANTS_NO_INSTR) || defined(CONFIG_GENERIC_SCHED_CLOCK)
extern u64 sched_clock_noinstr(void);
#else
static __always_inline u64 sched_clock_noinstr(void)
{
	return sched_clock();
}
#endif

/*
 * HÍBRIDO: running_clock() agora diferencia entre:
 * - Processos tradicionais (monolíticos)
 * - Servidores híbridos (userspace)
 */
extern u64 running_clock(void);
extern u64 sched_clock_cpu(int cpu);

/* HÍBRIDO: Nova função para alternar entre modos */
extern void hybrid_switch_domain(int cpu, int domain);

extern void sched_clock_init(void);

#ifndef CONFIG_HAVE_UNSTABLE_SCHED_CLOCK
static inline void sched_clock_tick(void)
{
}

static inline void clear_sched_clock_stable(void)
{
}

static inline void sched_clock_idle_sleep_event(void)
{
}

static inline void sched_clock_idle_wakeup_event(void)
{
}

static inline u64 cpu_clock(int cpu)
{
	return sched_clock();
}

static __always_inline u64 local_clock_noinstr(void)
{
	return sched_clock_noinstr();
}

static __always_inline u64 local_clock(void)
{
	return sched_clock();
}

/* HÍBRIDO: Fallback para sistemas sem suporte híbrido */
static inline u64 hybrid_server_clock(u64 server_id)
{
    return sched_clock();
}

static inline u64 hybrid_ipc_clock(void)
{
    return sched_clock();
}

#else
extern int sched_clock_stable(void);
extern void clear_sched_clock_stable(void);
extern u64 __sched_clock_offset;

extern void sched_clock_tick(void);
extern void sched_clock_tick_stable(void);
extern void sched_clock_idle_sleep_event(void);
extern void sched_clock_idle_wakeup_event(void);

static inline u64 cpu_clock(int cpu)
{
	return sched_clock_cpu(cpu);
}

extern u64 local_clock_noinstr(void);
extern u64 local_clock(void);

/* HÍBRIDO: Implementações para sistemas com suporte híbrido */
extern u64 hybrid_server_clock(u64 server_id);
extern u64 hybrid_ipc_clock(void);
extern void hybrid_record_context_switch(u64 from_server, u64 to_server);
#endif

#ifdef CONFIG_IRQ_TIME_ACCOUNTING
extern void enable_sched_clock_irqtime(void);
extern void disable_sched_clock_irqtime(void);
#else
static inline void enable_sched_clock_irqtime(void) {}
static inline void disable_sched_clock_irqtime(void) {}
#endif

/* HÍBRIDO: Nova configuração para ativar modo híbrido */
#ifdef CONFIG_HYBRID_KERNEL
extern bool hybrid_mode_enabled(void);
extern void hybrid_enter_user_server(u64 server_id);
extern void hybrid_exit_user_server(u64 server_id);
#else
static inline bool hybrid_mode_enabled(void) { return false; }
static inline void hybrid_enter_user_server(u64 server_id) { }
static inline void hybrid_exit_user_server(u64 server_id) { }
#endif

#endif /* _LINUX_SCHED_CLOCK_H */
