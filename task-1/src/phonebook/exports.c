#include "phonebook/exports.h"

#include <linux/fs.h>
#include <linux/kernel.h>

#include "globals.h"
#include "phonebook/phonebook.h"

// Имя get_user, видимо, уже занято макросом из ядра. Придется выбрать другое,
//   хотя в задании и требуется именно это имя..
/*
/home/valeriy/linux-course/task-1/src/phonebook/exports.c:9:95: error: macro "get_user" passed2
    9 |  get_user(const char* last_name, unsigned int len, struct pb_user_data* output_data) {
      |                                                                                    ^

In file included from ./include/linux/uaccess.h:12,
                 from ./include/linux/sched/task.h:13,
                 from ./include/linux/sched/signal.h:9,
                 from ./include/linux/rcuwait.h:6,
                 from ./include/linux/percpu-rwsem.h:7,
                 from ./include/linux/fs.h:33,
                 from /home/valeriy/linux-course/task-1/src/phonebook/exports.c:3:
./arch/x86/include/asm/uaccess.h:108:9: note: macro "get_user" defined here
  108 | #define get_user(x,ptr) ({ might_fault(); do_get_user_call(get_user,x,ptr); })
      |         ^~~~~~~~
/home/valeriy/linux-course/task-1/src/phonebook/exports.c:9:97: error: expected ‘=’, ‘,’, ‘;’,n
    9 |  get_user(const char* last_name, unsigned int len, struct pb_user_data* output_data) {
*/

long pb_get_user(const char* last_name, unsigned int len, struct pb_user_data* output_data) {
	size_t id = 0;
	if (!pb_get_geq_id_ud_with_last_name(0, last_name, &id, output_data)) {
		return -ENOENT;
	}

	// Помещается в long, т.к. размер телефонной книги ограничен 256.
	//   PB_PHONEBOOK_SIZE.
	return (long) id;
}

bool pb_check_ud_has_nullbytes(const struct pb_user_data* ud);
long pb_add_user(struct pb_user_data* ud) {
	if (!pb_check_ud_has_nullbytes(ud)) {
		if (debug) {
			printk(KERN_INFO "phonebook got user_data without null bytes in strings!\n");
		}

		return -EINVAL;
	}

	if (debug) {
		PB_PRINT_USER_DATA(printk, *ud, KERN_INFO "phonebook got user_data: ", ".\n");
	}

	size_t id = 0;
	if (!pb_add(&id, ud)) {
		return -EINVAL;
	}

	// Помещается в long, т.к. размер телефонной книги ограничен 256.
	//   PB_PHONEBOOK_SIZE.
	return (long) id;
}

// Удаляет всех пользователей с такой фамилией.
long pb_del_user(const char* last_name, unsigned int len) {
	char* last_name_copy = kzalloc(len + 1, GFP_KERNEL);
	if (last_name_copy == NULL) {
		return -ENOMEM;
	}

	strncpy(last_name_copy, last_name, len);
	size_t result = pb_remove_by_last_name(last_name_copy);
	
	kfree(last_name_copy);
	// Помещается в long, т.к. размер книги не более 256, что-то такое.
	//   PB_PHONEBOOK_SIZE.
	return (long) result;
}

EXPORT_SYMBOL(pb_get_user);
EXPORT_SYMBOL(pb_add_user);
EXPORT_SYMBOL(pb_del_user);
