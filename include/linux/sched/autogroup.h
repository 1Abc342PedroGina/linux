/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_AUTOGROUP_H
#define _LINUX_SCHED_AUTOGROUP_H

#include <linux/types.h>

struct signal_struct;
struct task_struct;
struct task_group;
struct seq_file;

/* HÍBRIDO: Novos tipos para servidores e domínios */
struct hybrid_server;
struct hybrid_domain;

/* HÍBRIDO: Tipos de grupos no kernel híbrido */
enum hybrid_group_type {
    HYBRID_GROUP_LEGACY = 0,    /* Grupo tradicional (monolítico) */
    HYBRID_GROUP_SERVER,         /* Servidor em userspace */
    HYBRID_GROUP_DRIVER,         /* Driver em userspace (futuro) */
    HYBRID_GROUP_IPC,            /* Grupo dedicado a IPC */
    HYBRID_GROUP_REALTIME,       /* Servidores de tempo real */
};

/* HÍBRIDO: Estrutura estendida para autogrupo híbrido */
struct hybrid_autogroup {
    struct task_group *tg;           /* Grupo de tarefas original */
    enum hybrid_group_type type;     /* Tipo do grupo */
    u64 server_id;                   /* ID único (para servidores) */
    u64 ipc_priority;                /* Prioridade para IPC */
    atomic_t server_refcnt;          /* Contagem de referências */
    struct list_head server_list;    /* Lista de servidores no grupo */
    
    /* HÍBRIDO: Métricas para scheduler de duas camadas */
    u64 total_user_time;              /* Tempo total em userspace */
    u64 total_kernel_time;            /* Tempo total em kernel */
    u64 ipc_wait_time;                /* Tempo aguardando IPC */
    u64 last_switch_time;             /* Última alternância de contexto */
    
    /* HÍBRIDO: Isolamento de recursos */
    u64 memory_limit;                 /* Limite de memória (servidores) */
    u64 cpu_reservation;              /* Reserva de CPU (microsegundos) */
    struct mutex lock;                /* Lock do grupo */
};

#ifdef CONFIG_SCHED_AUTOGROUP

/* Funções originais (mantidas para compatibilidade) */
extern void sched_autogroup_create_attach(struct task_struct *p);
extern void sched_autogroup_detach(struct task_struct *p);
extern void sched_autogroup_fork(struct signal_struct *sig);
extern void sched_autogroup_exit(struct signal_struct *sig);
extern void sched_autogroup_exit_task(struct task_struct *p);

/* HÍBRIDO: Novas funções para criar e gerenciar servidores */
extern struct hybrid_autogroup *hybrid_autogroup_create_server(
    const char *name, 
    u64 server_id,
    enum hybrid_group_type type,
    u64 cpu_reservation_ms
);

extern void hybrid_autogroup_destroy_server(struct hybrid_autogroup *hg);

/* HÍBRIDO: Funções para alternância entre domínios */
extern int hybrid_autogroup_attach_server(
    struct task_struct *p, 
    struct hybrid_autogroup *hg
);

extern void hybrid_autogroup_detach_server(struct task_struct *p);

/* HÍBRIDO: Controle de IPC entre servidores */
extern u64 hybrid_autogroup_get_ipc_priority(struct hybrid_autogroup *hg);
extern void hybrid_autogroup_set_ipc_priority(struct hybrid_autogroup *hg, u64 prio);

/* HÍBRIDO: Coleta de métricas para o scheduler de duas camadas */
extern void hybrid_autogroup_record_user_time(struct hybrid_autogroup *hg, u64 delta);
extern void hybrid_autogroup_record_kernel_time(struct hybrid_autogroup *hg, u64 delta);
extern void hybrid_autogroup_record_ipc_wait(struct hybrid_autogroup *hg, u64 delta);

/* HÍBRIDO: Sincronização entre servidores */
extern int hybrid_autogroup_wait_for_ipc(struct hybrid_autogroup *hg, u64 timeout_ns);
extern void hybrid_autogroup_wakeup_server(struct hybrid_autogroup *hg);

#ifdef CONFIG_PROC_FS
extern void proc_sched_autogroup_show_task(struct task_struct *p, struct seq_file *m);
extern int proc_sched_autogroup_set_nice(struct task_struct *p, int nice);

/* HÍBRIDO: Mostrar informações de servidores no /proc */
extern void proc_hybrid_server_show(struct hybrid_autogroup *hg, struct seq_file *m);
#endif

/* HÍBRIDO: Macro para verificar se é um servidor híbrido */
#define is_hybrid_server(hg) \
    (hg && (hg->type == HYBRID_GROUP_SERVER || hg->type == HYBRID_GROUP_DRIVER))

#else /* CONFIG_SCHED_AUTOGROUP */

/* Fallback para sistemas sem autogroup */
static inline void sched_autogroup_create_attach(struct task_struct *p) { }
static inline void sched_autogroup_detach(struct task_struct *p) { }
static inline void sched_autogroup_fork(struct signal_struct *sig) { }
static inline void sched_autogroup_exit(struct signal_struct *sig) { }
static inline void sched_autogroup_exit_task(struct task_struct *p) { }

/* HÍBRIDO: Fallbacks vazios quando autogroup desabilitado */
static inline struct hybrid_autogroup *hybrid_autogroup_create_server(
    const char *name, u64 server_id, enum hybrid_group_type type, u64 cpu_reservation_ms)
{
    return NULL;
}

static inline void hybrid_autogroup_destroy_server(struct hybrid_autogroup *hg) { }

static inline int hybrid_autogroup_attach_server(struct task_struct *p, 
                                                  struct hybrid_autogroup *hg)
{
    return -ENOSYS;
}

static inline void hybrid_autogroup_detach_server(struct task_struct *p) { }

#endif /* CONFIG_SCHED_AUTOGROUP */

#ifdef CONFIG_CGROUP_SCHED
extern struct task_group root_task_group;

/* HÍBRIDO: Integração com cgroups para isolamento de servidores */
extern int hybrid_autogroup_bind_cgroup(struct hybrid_autogroup *hg, 
                                         struct task_group *tg);
#endif /* CONFIG_CGROUP_SCHED */

#endif /* _LINUX_SCHED_AUTOGROUP_H */
