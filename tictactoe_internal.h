#ifndef __TICTACTOE_INTERNAL_H__
#define __TICTACTOE_INTERNAL_H__

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/types.h>
#include <linux/printk.h>
#include <linux/mutex.h>

#define tictactoe_error(format, ...) \
	pr_err("%s: " format, __func__, ##__VA_ARGS__)

typedef struct annotated_string {
	struct mutex lock;
	size_t len;
	char *buf;
} ttt_astring_t;

#endif // __TICTACTOE_INTERNAL_H__
