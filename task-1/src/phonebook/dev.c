#include <linux/fs.h>
#include <linux/kernel.h>

#include "globals.h"
#include "phonebook/phonebook.h"

// Device operations.
static int     pb_dev_open(struct inode*, struct file*);
static int     pb_dev_release(struct inode*, struct file*);
static ssize_t pb_dev_read(struct file*, char*, size_t, loff_t*);
static ssize_t pb_dev_write(struct file*, const char*, size_t, loff_t*);
static ssize_t handle_message(void);

struct file_operations fops = {
	.owner = THIS_MODULE,

	.open    = pb_dev_open,
	.read    = pb_dev_read,
	.write   = pb_dev_write,
	.release = pb_dev_release,
};


// https://stackoverflow.com/questions/46877632/simple-character-device-not-working-as-expected

// Нужно уметь вносить данные сначала, потому первым делом напишем
//   write.
// https://tldp.org/LDP/lkmpg/2.4/html/c577.htm
// https://stackoverflow.com/questions/9713112/understanding-loff-t-offp-for-file-operations
//   Параметр offset указывает на переменную, являющуюся смещением в файле. Указатель на нее
//   передается вызывающей стороной. Мы всегда либо 
static char msg_buffer[PB_MSG_BUFFER_LEN] = {0};
static size_t msg_buffer_pos = 0;
static bool has_error = false;
static enum {
	NO_OPERATION,
	RETURN_ID,           // After PB_OPERATION_ADD.
	RETURN_BY_LAST_NAME,
	RETURN_BY_ID,
} next_read;
static size_t wanted_id_start = 0;
static size_t wanted_id       = 0;
static size_t returned_id     = 0;
static char wanted_last_name[sizeof(((struct pb_user_data*) 0)->last_name)];

int pb_dev_open(struct inode* inodep, struct file* filep) {
	// Только один процесс может работать с файлом в каждый момент времени...
	//   Потому что мы дописываем в буфер. И один и тот же для каждого открытия.
	//   Т.е. один и тот же процесс может, например, два раза файл открыть на
	//   запись.. Теоретически. Можно ли на самом деле это сделать в линуксе, не
	//   знаю. Не слышал о том, что нельзя. Но можно ожидать, что в файле будет
	//   каша, конечно, потому так никто не делает. По-хорошему нужно по своему
	//   буферу на каждое открытие. Будем наполнять его операциями для открытия.
	mutex_lock(&pb_chardev_mutex);

	if (debug) {
		printk(KERN_INFO "phonebook chardev is open, filep = %p.\n", filep);
	}

	msg_buffer_pos = 0;
	has_error = false;
	next_read = NO_OPERATION;
	wanted_id_start = 0;
	wanted_id = 0;
	returned_id = 0;

	return 0;
}


// https://stackoverflow.com/questions/2615203/is-sizeof-in-c-evaluated-at-compilation-time-or-run-time
//   У нас массив со статической длиной, потому размер вычислится во время комплияции,
//   прохода по нулевому указателю не будет, важны лишь типы.

bool pb_check_ud_has_nullbytes(const struct pb_user_data* ud);
bool pb_check_ud_has_nullbytes(const struct pb_user_data* ud) {
	/* From man strnlen.
	   RETURN VALUE
       The strnlen() function returns strlen(s), if that is less than maxlen, or
       maxlen if there is no null terminating ('\0') among the first maxlen
       characters pointed to by s.
       It never checks positions equal or greater to maxlen.
	*/
	// Т.е. нулевой байт встречается в позиции до длины буфера. Или
	//   предполагается, что он за пределами буфера сразу же. Но нам в этой
	//   функции такой случай нужно отловить, что нулевого байта нет.
	#define CHECK_STR(char_array) \
		if (strnlen(char_array, sizeof(char_array) / sizeof(char)) == \
		    sizeof(char_array) / sizeof(char)) { return false; }

	CHECK_STR(ud->first_name);
	CHECK_STR(ud->last_name);
	CHECK_STR(ud->telnum);
	CHECK_STR(ud->email);	

	#undef CHECK_STR

	return true;
}

static ssize_t handle_message(void) {
	int op = 0;
	struct pb_user_data ud;

	memcpy(&op, msg_buffer, sizeof(op));

	switch (op) {
	case PB_OPERATION_ADD: {
		memcpy(&ud, msg_buffer + sizeof(op), sizeof(ud));
		if (!pb_check_ud_has_nullbytes(&ud)) {
			if (debug) {
				printk(KERN_INFO "phonebook got user_data without null bytes in strings!\n");
			}

			return -EINVAL;
		}

		if (debug) {
			PB_PRINT_USER_DATA(printk, ud, KERN_INFO "phonebook got user_data: ", ".\n");
		}

		size_t id = 0;
		if (!pb_add(&id, &ud)) {
			// Возвращать более гранулярные ошибки. На все случаи возвращать
			//   EINVAL не очень хорошо, ведь может быть нужно понять, есть
			//   ли такой контакт, если не получилось добавить. Или действительно
			//   произошла ошибка, закончилась ли телефонная книга. Есть
			//   недостатки архитектурные, можно улучшать.. Была бы возможность,
			//   тут важно, чтобы можно было какое-то версионирование сделать.
			return -EINVAL;
		}

		next_read = RETURN_ID;
		returned_id = id;

		break;
	}

	case PB_OPERATION_FIND_BY_LAST_NAME: {
		memcpy(&wanted_id_start, msg_buffer + sizeof(op), sizeof(wanted_id_start));		
		memcpy(
			&wanted_last_name,
			msg_buffer + sizeof(op) + sizeof(wanted_id_start),
			sizeof(wanted_last_name));
		next_read = RETURN_BY_LAST_NAME;

		break;
	}

	case PB_OPERATION_FIND_BY_ID: {
		memcpy(&wanted_id, msg_buffer + sizeof(op), sizeof(wanted_id));		
		next_read = RETURN_BY_ID;
		
		break;
	}

	case PB_OPERATION_DELETE: {
		size_t id = 0;
		memcpy(&id, msg_buffer + sizeof(op), sizeof(id));

		if (!pb_remove_by_id(id)) {
			return -EINVAL;
		}

		break;
	}

	default: {
		if (debug) {
			printk(KERN_INFO "phonebook chardev: user supplied invalid operation %d.\n", op);
		}

		return -EINVAL;
	}
	}

	return 0;
}

static ssize_t pb_dev_write(struct file* filep, const char __user * buffer, size_t len, loff_t* offset) {
	if (debug) {
		printk(KERN_INFO "phonebook chardev: got a write attempt.\n");			
	}

	if (has_error) {
		// Как только один раз произошла ошибка, при этом открытии всегда
		//   будет ошибка. Т.к. пользователь может попробовать записать
		//   заново, тогда то ли буфер продолжать, то ли заново начинать.
		// Пока не попросит писать с начала, ничего не делаем.
		return -EINVAL;
	}

	ssize_t num_consumed = 0;
	while (len > 0) {
		// Дописываем к буферу и пытаемся трактовать как операцию, если получается.
		size_t num_copied = min(PB_MSG_BUFFER_LEN - msg_buffer_pos, len);

		unsigned long error_count = copy_from_user(msg_buffer + msg_buffer_pos, buffer, num_copied);
		num_consumed += num_copied;
		len -= num_copied;
		buffer += num_copied;
		msg_buffer_pos += num_copied;

		if (debug) {
			printk(KERN_INFO "phonebook chardev: copied %zu bytes in pb_dev_write.\n", num_copied);			
		}

		if (error_count != 0) {
			if (debug) {
				printk(KERN_INFO "phonebook chardev: failed to copy from user in pb_dev_write.\n");
			}
			return -EFAULT;
		}

		if (debug) {
			printk(KERN_INFO "phonebook chardev: msg_buffer has %zu out of %zu bytes.\n", msg_buffer_pos, PB_MSG_BUFFER_LEN);			
		}

		if (msg_buffer_pos == PB_MSG_BUFFER_LEN) {
			msg_buffer_pos = 0;
			int msg_result = handle_message();

			if (msg_result == 0) {
				return num_consumed;
			} else {
				has_error = true;
				return msg_result;
			}
		}
	}

	// Возможно, написали перевод строки. А мы считаем его за символ сообщения.
	//   Но если он лишь один, то все в порядке. Я думаю, xxd, если пишет байты
	//   с опцией -r, не должен передавать переводы строки в конце, например.
	//   Т.к. его не просили это делать. Иначе в байтовом выводе мусор
	//   текстового характера.
	return num_consumed;
}
#undef BUFFER_LEN

static int pb_dev_release(struct inode* inodep, struct file* filep) {
	if (debug) {
		printk(KERN_INFO "phonebook chardev is closed, filep = %p.\n", filep);
	}

	mutex_unlock(&pb_chardev_mutex);

	return 0;
}


static ssize_t pb_dev_read(struct file* filep, char* buffer, size_t len, loff_t* offset) {
	if (debug) {
		printk(KERN_INFO "phonebook chardev: got a read attempt.\n");
	}

	switch (next_read) {
	default:
	case NO_OPERATION: {
		return 0;
	}

	case RETURN_ID: {
		if (debug) {
			printk(KERN_INFO "phonebook chardev: read for RETURN_ID.\n");
		}

		if (len < sizeof(returned_id)) {
			return -EFBIG;
		}

		unsigned long error_count = copy_to_user(buffer, &returned_id, sizeof(returned_id));
		if (error_count != 0) {
			return -EFAULT;
		}

		return sizeof(returned_id);
	}

	case RETURN_BY_LAST_NAME: {
		struct pb_user_data ud = {0};

		size_t id = 0;
		if (!pb_get_geq_id_ud_with_last_name(wanted_id_start, wanted_last_name, &id, &ud)) {
			return -ENOENT;
		}

		if (len < sizeof(id) + sizeof(ud)) {
			return -EFBIG;
		}

		unsigned long error_count = copy_to_user(buffer, &id, sizeof(id));
		if (error_count != 0) {
			return -EFAULT;
		}

		error_count = copy_to_user(buffer + sizeof(id), &ud, sizeof(ud));
		if (error_count != 0) {
			return -EFAULT;
		}

		return sizeof(id) + sizeof(ud);
	}

	case RETURN_BY_ID: {
		struct pb_user_data ud = {0};

		if (!pb_get_by_id(wanted_id, &ud)) {
			return -ENOENT;
		}

		if (len < sizeof(wanted_id) + sizeof(ud)) {
			return -EFBIG;
		}

		unsigned long error_count = 0;

		error_count = copy_to_user(buffer, &wanted_id, sizeof(wanted_id));
		if (error_count != 0) {
			return -EFAULT;
		}

		error_count = copy_to_user(buffer + sizeof(wanted_id), &ud, sizeof(ud));
		if (error_count != 0) {
			return -EFAULT;
		}

		return sizeof(wanted_id) + sizeof(ud);
	}
	}

	return 0;
}
