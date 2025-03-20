#include "tictactoe_internal.h"
#include "tictactoe_game.h"
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/printk.h>
#include <linux/minmax.h>
#include <linux/mutex.h>

#define TICTACTOE_DEVICE_NAME		"tictactoe"
#define TICTACTOE_MODULE_NAME		"tictactoe"
#define TICTACTOE_MODULE_DESC		TICTACTOE_DEVICE_NAME " device driver"
#define TICTACTOE_PRINT_GAME_BUF_SIZE	256
#define TICTACTOE_RESET_COMMAND		"reset"
#define TICTACTOE_RESET_COMMAND_LEN	(sizeof(TICTACTOE_RESET_COMMAND) - 1)

MODULE_AUTHOR("Niklas Elsbrock");
MODULE_DESCRIPTION(TICTACTOE_MODULE_DESC);
MODULE_LICENSE("Dual MIT/GPL");

static ttt_game_t *tictactoe_current_game;
DEFINE_MUTEX(tictactoe_mutex_current_game);

static int tictactoe_init_annotated_game_string(ttt_astring_t *string)
{
	int out_len;

	string->buf = kmalloc(TICTACTOE_PRINT_GAME_BUF_SIZE, GFP_KERNEL);
	if (!string->buf) {
		tictactoe_error("failed to allocate buffer memory\n");
		return -1;
	}

	mutex_lock(&tictactoe_mutex_current_game);
	out_len = tictactoe_game_snprint(tictactoe_current_game, string->buf,
					 TICTACTOE_PRINT_GAME_BUF_SIZE);
	mutex_unlock(&tictactoe_mutex_current_game);

	if (out_len < 0) {
		kfree(string->buf);
		tictactoe_error("failed to print game\n");
		return -1;
	}

	string->len = out_len;
	return 0;
}

static ssize_t tictactoe_read(struct file *file, char __user *buf, size_t count,
			      loff_t *ppos)
{
	ttt_astring_t *string;
	size_t read_len;

	if (count == 0)
		return 0;

	if (*ppos < 0)
		return -EINVAL;

	string = (ttt_astring_t *)file->private_data;
	mutex_lock(&string->lock);

	if (!string->buf) {
		if (tictactoe_init_annotated_game_string(string)) {
			mutex_unlock(&string->lock);
			return -EFAULT;
		}
	}

	if (*ppos >= string->len) {
		mutex_unlock(&string->lock);
		return 0;
	}

	read_len = min(count, string->len - (size_t)*ppos);
	if (copy_to_user(buf, string->buf + *ppos, read_len)) {
		mutex_unlock(&string->lock);
		tictactoe_error("copy_to_user failed\n");
		return -EFAULT;
	}

	mutex_unlock(&string->lock);

	*ppos += read_len;
	return read_len;
}

static ssize_t tictactoe_write(struct file *file, const char __user *buf,
			       size_t count, loff_t *ppos)
{
	char *command_buf;
	size_t count_without_lf;
	size_t x, y;

	if (count == 0)
		return 0;

	if (*ppos < 0)
		return -EINVAL;

	command_buf = kmalloc(count, GFP_KERNEL);
	if (!command_buf) {
		tictactoe_error("failed to allocate buffer memory\n");
		return -EFAULT;
	}

	if (copy_from_user(command_buf, buf + *ppos, count)) {
		kfree(command_buf);
		tictactoe_error("failed to copy write buffer\n");
		return -EFAULT;
	}

	if (command_buf[count - 1] == '\n') {
		count_without_lf = count - 1;
	} else {
		count_without_lf = count;
	}

	// reset command
	if (count_without_lf == TICTACTOE_RESET_COMMAND_LEN &&
	    strncmp(TICTACTOE_RESET_COMMAND, command_buf,
		    TICTACTOE_RESET_COMMAND_LEN) == 0) {
		mutex_lock(&tictactoe_mutex_current_game);
		tictactoe_game_init(tictactoe_current_game);
		mutex_unlock(&tictactoe_mutex_current_game);
		kfree(command_buf);
		return count;
	}

	if (count_without_lf != 2) {
		kfree(command_buf);
		pr_notice("invalid command length\n");
		return -EINVAL;
	}

	for (int i = 0; i < 2; i++) {
		if (command_buf[i] < '1' || command_buf[i] > '3') {
			kfree(command_buf);
			pr_notice("invalid coordinates\n");
			return -EINVAL;
		}
	}

	x = (size_t)(command_buf[0] - '1');
	y = (size_t)(command_buf[1] - '1');

	kfree(command_buf);

	mutex_lock(&tictactoe_mutex_current_game);

	if (tictactoe_current_game->winner) {
#ifdef CONFIG_TICTACTOE_AUTO_RESET
		tictactoe_game_init(tictactoe_current_game);
#else
		mutex_unlock(&tictactoe_mutex_current_game);
		pr_notice("the game is finished, use '" TICTACTOE_RESET_COMMAND
			  "' to start a new one\n");
		return -EINVAL;
#endif
	}

	if (tictactoe_game_make_turn(tictactoe_current_game, x, y)) {
		mutex_unlock(&tictactoe_mutex_current_game);
		return -EINVAL;
	}

	mutex_unlock(&tictactoe_mutex_current_game);
	return count;
}

static int tictactoe_open(struct inode *inode, struct file *file)
{
	ttt_astring_t *string;

	file->private_data = kmalloc(sizeof(ttt_astring_t), GFP_KERNEL);
	if (!file->private_data) {
		tictactoe_error(
			"failed to allocate private data buffer memory\n");
		return -EFAULT;
	}

	string = (ttt_astring_t *)file->private_data;
	mutex_init(&string->lock);
	string->buf = NULL;

	return 0;
}

static int tictactoe_release(struct inode *inode, struct file *file)
{
	ttt_astring_t *string;
	string = (ttt_astring_t *)file->private_data;
	if (string->buf)
		kfree(string->buf);
	kfree(string);
	return 0;
}

static const struct file_operations tictactoe_fops = {
	.owner		= THIS_MODULE,
	.read		= tictactoe_read,
	.write		= tictactoe_write,
	.open		= tictactoe_open,
	.release	= tictactoe_release,
};

static struct miscdevice tictactoe_dev = {
	MISC_DYNAMIC_MINOR,
	TICTACTOE_DEVICE_NAME,
	&tictactoe_fops,
};

static int __init tictactoe_init(void)
{
	int ret;

	mutex_lock(&tictactoe_mutex_current_game);
	tictactoe_current_game = kmalloc(sizeof(ttt_game_t), GFP_KERNEL);
	if (!tictactoe_current_game) {
		mutex_unlock(&tictactoe_mutex_current_game);
		tictactoe_error("failed to allocate game memory\n");
		return -EFAULT;
	}
	ret = tictactoe_game_init(tictactoe_current_game);
	if (ret) {
		mutex_unlock(&tictactoe_mutex_current_game);
		tictactoe_error("failed to initialize game\n");
		return -EFAULT;
	}
	mutex_unlock(&tictactoe_mutex_current_game);

	ret = misc_register(&tictactoe_dev);
	if (ret)
		tictactoe_error("Unable to register misc device\n");
	else
		pr_info(TICTACTOE_MODULE_DESC "\n");
	return ret;
}

static void __exit tictactoe_exit(void)
{
	mutex_lock(&tictactoe_mutex_current_game);
	kfree(tictactoe_current_game);
	mutex_unlock(&tictactoe_mutex_current_game);

	misc_deregister(&tictactoe_dev);
}

module_init(tictactoe_init);
module_exit(tictactoe_exit);
