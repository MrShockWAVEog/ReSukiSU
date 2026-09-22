/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef KSU_FEATURE_TOOLKIT_H
#define KSU_FEATURE_TOOLKIT_H

#include <linux/types.h>
#include <linux/uaccess.h>

#ifdef CONFIG_KSU_TOOLKIT_SUPPORT
void ksu_toolkit_log(u8 symbol, u32 uid);
int ksu_toolkit_dump_log(void __user *arg);
#else
static inline void ksu_toolkit_log(u8 symbol, u32 uid) {}
#endif

#endif
