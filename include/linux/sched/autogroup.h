/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_AUTOGROUP_H
#define _LINUX_SCHED_AUTOGROUP_H

/*
 * KERNEL HÍBRIDO - Autogroup Management
 * 
 * Abordagem Híbrida:
 * - Mantém núcleo monolítico para performance crítica
 * - Permite que políticas e gerenciamento rodem em userspace
 * - Drivers/gestores podem ser carregados em qualquer camada
 * - Chamadas diretas, não mensagens
 */

struct signal_struct;
struct task_struct;
struct task_group;
struct seq_file;

/* ============================================ */
/* CAMADA 1: NÚCLEO MONOLÍTICO (obrigatório)    */
/* ============================================ */
/* Funcionalidades essenciais SEMPRE no kernel  */
/* - Agendamento básico                         */
/* - Context switching                          */
/* - Interrupções                               */

/* Estrutura base - sempre no kernel */
struct autogroup_core {
    int id;                      /* ID do grupo */
    struct task_group *tg;       /* Task group (kernel) */
    atomic_t refcount;           /* Referências */
    unsigned long flags;         /* Flags de estado */
};

/* Funções CORE - sempre em kernel */
extern void sched_autogroup_create_attach(struct task_struct *p);
extern void sched_autogroup_detach(struct task_struct *p);
extern void sched_autogroup_fork(struct signal_struct *sig);
extern void sched_autogroup_exit(struct signal_struct *sig);

/* ============================================ */
/* CAMADA 2: MODULAR (híbrida)                  */
/* ============================================ */
/* Partes que podem ser movidas para userspace  */
/* ou mantidas no kernel conforme necessidade   */

/* 
 * Configuração em tempo de compilação:
 * - CONFIG_AUTOGROUP_KERNEL: mantém no kernel
 * - CONFIG_AUTOGROUP_USER: move para userspace
 */

#ifdef CONFIG_AUTOGROUP_KERNEL
/* Modo KERNEL: tudo no kernelspace (monolítico tradicional) */
static inline int autogroup_set_nice(struct task_struct *p, int nice)
{
    return sched_autogroup_set_nice(p, nice);
}
#endif

#ifdef CONFIG_AUTOGROUP_USER
/* Modo USER: política em userspace via syscalls */
extern int autogroup_set_nice_user(struct task_struct *p, int nice);
static inline int autogroup_set_nice(struct task_struct *p, int nice)
{
    return autogroup_set_nice_user(p, nice);
}
#endif

/* ============================================ */
/* CAMADA 3: DRIVERS HÍBRIDOS                   */
/* ============================================ */
/* Drivers podem ser implementados em userspace */
/* através de interfaces padronizadas           */

struct autogroup_operations {
    int (*set_nice)(struct task_struct *p, int nice);
    int (*get_info)(struct task_struct *p, void *info);
    int (*migrate)(struct task_struct *p, int new_group);
};

/* Registro de driver híbrido (pode ser userspace) */
extern int autogroup_register_driver(struct autogroup_operations *ops, int flags);
extern void autogroup_unregister_driver(int id);

/* ============================================ */
/* INTERFACE DE USERSPACE (syscalls híbridas)   */
/* ============================================ */
/* Syscalls que permitem implementação alternativa */

/* Syscall que pode ser roteada para userspace */
asmlinkage long sys_autogroup_setpolicy(int pid, int policy, int nice);

/* ============================================ */
/* MODO HÍBRIDO ATIVO                           */
/* ============================================ */

#ifdef CONFIG_SCHED_AUTOGROUP
#ifdef CONFIG_PROC_FS
extern void proc_sched_autogroup_show_task(struct task_struct *p, struct seq_file *m);
extern int proc_sched_autogroup_set_nice(struct task_struct *p, int nice);
#endif
#else
static inline void sched_autogroup_create_attach(struct task_struct *p) { }
static inline void sched_autogroup_detach(struct task_struct *p) { }
static inline void sched_autogroup_fork(struct signal_struct *sig) { }
static inline void sched_autogroup_exit(struct signal_struct *sig) { }
#endif

#ifdef CONFIG_CGROUP_SCHED
extern struct task_group root_task_group;
#endif

#endif /* _LINUX_SCHED_AUTOGROUP_H */
