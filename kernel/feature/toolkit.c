// SPDX-License-Identifier: GPL-2.0-only
/* Timestamped log ABI used by backslashxx/ksu_toolkit. */
#include <linux/ktime.h>
#include <linux/math64.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
#include <linux/timekeeping.h>
#endif

#include "feature/toolkit.h"

#define TOOLKIT_LOG_ENTRIES 250

struct toolkit_log_entry {
    u32 seconds;
    u32 data; /* Low 24 bits: UID; high 8 bits: event symbol. */
};

struct toolkit_log_request {
    u64 index_ptr;
    u64 buf_ptr;
    u64 uptime_ptr;
};

static struct toolkit_log_entry toolkit_log[TOOLKIT_LOG_ENTRIES];
static u32 toolkit_log_next;
static DEFINE_SPINLOCK(toolkit_log_lock);

static u32 toolkit_uptime(void)
{
    return div_u64(ktime_to_ns(ktime_get_boottime()), NSEC_PER_SEC);
}

void ksu_toolkit_log(u8 symbol, u32 uid)
{
    struct toolkit_log_entry entry;
    unsigned long flags;

    entry.seconds = toolkit_uptime();
    entry.data = (uid & 0x00ffffff) | ((u32)symbol << 24);
    spin_lock_irqsave(&toolkit_log_lock, flags);
    toolkit_log[toolkit_log_next] = entry;
    toolkit_log_next = (toolkit_log_next + 1) % TOOLKIT_LOG_ENTRIES;
    spin_unlock_irqrestore(&toolkit_log_lock, flags);
}

int ksu_toolkit_dump_log(void __user *arg)
{
    struct toolkit_log_request request;
    struct toolkit_log_entry *snapshot;
    unsigned long flags;
    u32 next, uptime;
    int ret = 0;

    if (copy_from_user(&request, arg, sizeof(request)))
        return -EFAULT;
    if (!request.index_ptr || !request.buf_ptr || !request.uptime_ptr)
        return -EINVAL;

    snapshot = kmalloc(sizeof(toolkit_log), GFP_KERNEL);
    if (!snapshot)
        return -ENOMEM;

    spin_lock_irqsave(&toolkit_log_lock, flags);
    memcpy(snapshot, toolkit_log, sizeof(toolkit_log));
    next = toolkit_log_next;
    spin_unlock_irqrestore(&toolkit_log_lock, flags);
    uptime = toolkit_uptime();

    if (copy_to_user((void __user *)(unsigned long)request.index_ptr, &next, sizeof(next)) ||
        copy_to_user((void __user *)(unsigned long)request.buf_ptr, snapshot, sizeof(toolkit_log)) ||
        copy_to_user((void __user *)(unsigned long)request.uptime_ptr, &uptime, sizeof(uptime)))
        ret = -EFAULT;
    kfree(snapshot);
    return ret;
}
