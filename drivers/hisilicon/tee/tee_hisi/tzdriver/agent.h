/*
 * FileName:
 * Author:         v00297436 
 * Description:
 * Version:
 * Function List:
 */

#ifndef _AGENT_H_
#define _AGENT_H__

#include "teek_ns_client.h"

int agent_init(void);
int agent_exit(void);
unsigned int agent_process_work(TC_NS_SMC_CMD *smc_cmd,
				unsigned int agent_id);
int is_agent_alive(unsigned int agent_id);

int TC_NS_register_agent(TC_NS_DEV_File* dev_file, unsigned int agent_id,
			 TC_NS_Shared_MEM* shared_mem);
int TC_NS_unregister_agent(unsigned long agent_id);
int TC_NS_unregister_agent_client(TC_NS_DEV_File* dev_file);

unsigned int TC_NS_incomplete_proceed(TC_NS_SMC_CMD *smc_cmd,
				      unsigned int agent_id,
				      uint8_t flags);
int TC_NS_wait_event(unsigned long agent_id);
int TC_NS_send_event_reponse(unsigned long agent_id);
int TC_NS_alloc_exception_mem(void);
int TC_NS_store_exception_info(void);

struct __smc_event_data* find_event_control(unsigned long agent_id);

//for secure agent
struct __smc_event_data{
    unsigned int agent_id;
    struct mutex work_lock;
    wait_queue_head_t wait_event_wq;
    int ret_flag;
    wait_queue_head_t send_response_wq;
    int send_flag;
    struct list_head head;
    TC_NS_SMC_CMD cmd;
    TC_NS_DEV_File* owner;
    void* buffer;
};
//struct __smc_event_data* smc_event_data;

struct tee_agent_kernel_ops {
    const char *agent_name; // MUST NOT be NULL
    unsigned int agent_id; // MUST NOT be zero
    int (*tee_agent_init)(struct tee_agent_kernel_ops* agent_instance);
    int (*tee_agent_run)(struct tee_agent_kernel_ops* agent_instance);
    int (*tee_agent_work)(struct tee_agent_kernel_ops* agent_instance); // MUST NOT be NULL
    int (*tee_agent_stop)(struct tee_agent_kernel_ops* agent_instance);
    int (*tee_agent_exit)(struct tee_agent_kernel_ops* agent_instance);
    int (*tee_agent_crash_work)(struct tee_agent_kernel_ops* agent_instance, TC_NS_ClientContext *context);
    struct task_struct *agent_thread;
    void *agent_data;
    TC_NS_Shared_MEM *agent_buffer;
    struct list_head list;
};

int tee_agent_clear_work(TC_NS_ClientContext *context);
int tee_agent_kernel_register(struct tee_agent_kernel_ops* new_agent);
#endif //_AGENT_H_
