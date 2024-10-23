// These a probably host system headers. But system calls
//   we're going to use (read, write, open, close) are in the
//   linux kernel for a long time. So numbers should be the same.
// https://filippo.io/linux-syscall-table/
#define _GNU_SOURCE
#include <unistd.h>

#include "phonebook/phonebook.h"

// static const long SYS_OPEN  = 2;
// static const long SYS_CLOSE = 3;
// static const long SYS_WRITE = 1;
// static const long SYS_READ  = 0;

// Exits all threads in current thread group (kind of like a process in windows).
//   Exit syscall would exit just the current one (which works for us, but
//   let's be closer to libc..)
// static const long SYS_EXIT_GROUP = 231;

static const long STDOUT_FD = 1; // Это и из опыта с командной оболочкой знаем.

// https://stackoverflow.com/questions/65206651/numerical-values-of-open-arguments-in-linux
static const long OPEN_RDWR = 02;

typedef unsigned long size_t;

int open(const char* filename, int flags, unsigned long mode) {
	// https://stackoverflow.com/questions/69515893/when-does-linux-x86-64-syscall-clobber-r8-r9-and-r10
	long result = 0;
	__asm__ __volatile__ (
		"movq $0x02, %%rax\n"
		"movq %1,    %%rdi\n"
		"movq %2,    %%rsi\n"
		"movq %3,    %%rdx\n"
		"syscall\n"
		"movq %%rax, %0\n"
		: "=m"(result)
		: "m"(filename), "m"(flags), "m"(mode)
		: "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
	);
	return result;
}

ssize_t write(int fd, const void* buf, size_t count) {
	ssize_t result = 0;
	__asm__ __volatile__ (
		"movq $0x01, %%rax\n"
		"movl %1,    %%edi\n"
		"movq %2,    %%rsi\n"
		"movq %3,    %%rdx\n"
		"syscall\n"
		"movq %%rax, %0\n"
		: "=m"(result)
		: "m"(fd), "m"(buf), "m"(count)
		: "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
	);
	return result;
}

ssize_t read(int fd, void* buf, size_t count) {
	ssize_t result = 0;
	__asm__ __volatile__ (
		"movq $0x00, %%rax\n"
		"movl %2,    %%edi\n"
		"movq %1,    %%rsi\n"
		"movq %3,    %%rdx\n"
		"syscall\n"
		"movq %%rax, %0\n"
		: "=m"(result), "=m"(buf)
		: "m"(fd), "m"(count)
		: "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
	);
	return result;
}

__attribute__((noreturn))
void exit_group(int exit_code) {
	__asm__ __volatile__ (
		"movq $0xe7, %%rax\n"
		"movl %0,    %%edi\n"
		"syscall\n"
		:
		: "r"(exit_code)
		: "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
	);
	__builtin_unreachable();
}


size_t strlen(const char* str) {
	size_t len = 0;
	while (*str != '\0') {
		len += 1;
		str += 1;
	}

	return len;
}

void* memset(void* dst, int src, size_t len) {
	for (size_t i = 0; i < len; ++i) {
		((char*) dst)[i] = (char) src;
	}
	return dst;
}


int strncmp(const char* lhs, const char* rhs, size_t length) {
	for (size_t i = 0; i < length; ++i) {
		// Сравниваем не более первых length байт.
		unsigned char ch1 = (unsigned char) lhs[i];
		unsigned char ch2 = (unsigned char) rhs[i];
		if (ch1 < ch2) {
			return -1;
		} else if (ch1 > ch2) {
			return 1;
		}

		// Не нашли различия, продолжаем, если одна из строк не закончилась.
		//   Но в данном случае если закончится одна, то закончится и другая.
		// assert(ch1 == ch2);
		// Укажем, что этот случай редкий. Можно ожидать, что это значение
		//   равно нулю. Чаще всего будем продолжать итерирование, когда
		//   строки длинные.
		if (__builtin_expect(ch1 == 0 || ch2 == 0, 0)) {
			break;
		}
	}
	return 0; // Не нашли различий.
}

#define write_str(fd, str)   write((fd), (str), strlen(str))
void write_int(int fd, long long value) {
	char buffer[32] = {0};
	size_t num_digits = 0;
	char* digit_start = buffer;
	unsigned long long abs_value = 0;
	if (value < 0) {
		*digit_start = '-';
		digit_start += 1;

		// Работает даже в случае, если у нас был LLONG_MIN. Просто с минусом
		//   в типе long long не получится, т.к. отрицательная часть целых
		//   чисел одной ширины больше, чем положительная. Т.е. минимальное
		//   отрицательное не поместится в положительную часть.
		abs_value = ~((unsigned long long) value) + 1;
	} else {
		abs_value = (unsigned long long) value;
	}
	if (abs_value == 0) {
		*digit_start = '0';
		num_digits += 1;
	} else {
		while (abs_value > 0) {
			digit_start[num_digits] = '0' + (char) (abs_value % 10);
			num_digits += 1;
			abs_value /= 10;
		}
		// Reverse the string, we got little-endian order of digits.
		for (size_t pos = 0; pos < num_digits / 2; ++pos) {
			// swap digit_start[pos] and digit_start[num_digits - 1 - pos]
			char tmp = digit_start[pos];
			digit_start[pos] = digit_start[num_digits - 1 - pos];
			digit_start[num_digits - 1 - pos] = tmp;
		}
	}
	write_str(fd, buffer);
}

#define ENOENT 2

// I create a separate function, because if we'd write code in _start,
//   that would break instrumentation. Compiler doesn't know that
//   syscall with parameter SYS_EXIT_GROUP doesn't return. And it
//   may result in it saying we use uninitialized values or etc.
//   It's just what I image, I saw gcc complains 
int main() {
	// 1. Let's open the phonebook chardev file.
	int fd = open(pb_path, OPEN_RDWR, 0);
	if (fd < 0) {
		// Some error.
		int unused = write_str(STDOUT_FD, "failed to open the phonebook chardev file.\n");
		(void) unused;
		return 1;
	}

	// 2. Add a person.

	struct pb_user_data avictorov_ud = {
		.first_name = "Александр",
		.last_name  = "Викторов",
		.age        = 21,
		.telnum     = "+79818181111",
		.email      = "alexander.victorov@pushkapochta.ru"
	};
	size_t avictorov_id = 0;

	if (write(fd, &PB_OPERATION_ADD, sizeof(PB_OPERATION_ADD)) < 0) {
		return 2;
	}

	if (write(fd, &avictorov_ud, sizeof(avictorov_ud)) < 0) {
		return 3;
	}

	if (read(fd, &avictorov_id, sizeof(avictorov_id)) < (ssize_t) sizeof(avictorov_id)) {
		return 4;
	}

	// 3. Add another person with the same last name.

	struct pb_user_data vvictorov_ud = {
		.first_name = "Виктор",
		.last_name  = "Викторов",
		.age        = 22,
		.telnum     = "+79818181111",
		.email      = "victor.victorov@pushkapochta.ru"
	};
	size_t vvictorov_id = 0;

	if (write(fd, &PB_OPERATION_ADD, sizeof(PB_OPERATION_ADD)) < 0) {
		return 5;
	}

	if (write(fd, &vvictorov_ud, sizeof(vvictorov_ud)) < 0) {
		return 6;
	}

	if (read(fd, &vvictorov_id, sizeof(vvictorov_id)) < (ssize_t) sizeof(vvictorov_id)) {
		return 7;
	}

	// 4. Add a person with a fresh last name.

	struct pb_user_data vpavlovich_ud = {
		.first_name = "Виктор",
		.last_name  = "Павлович",
		.age        = 23,
		.telnum     = "+79818181111",
		.email      = "victor.pavlovich@pushkapochta.ru"
	};
	size_t vpavlovich_id = 0;

	if (write(fd, &PB_OPERATION_ADD, sizeof(PB_OPERATION_ADD)) < 0) {
		return 8;
	}

	if (write(fd, &vpavlovich_ud, sizeof(vpavlovich_ud)) < 0) {
		return 9;
	} 

	if (read(fd, &vpavlovich_id, sizeof(vpavlovich_id)) < (ssize_t) sizeof(vpavlovich_id)) {
		return 10;
	}

	// 5. Query a person with min id, but with specific lastname.

	int op = PB_OPERATION_FIND_BY_LAST_NAME;
	if (write(fd, &op, sizeof(op)) < 0) {
		return 11;
	}

	size_t id_start = 0;
	if (write(fd, &id_start, sizeof(id_start)) < 0) {
		return 12;
	}

	if (write(fd, avictorov_ud.last_name, sizeof(avictorov_ud.last_name)) < 0) {
		return 13;
	}

	for (
		size_t i = sizeof(op) + sizeof(id_start) + sizeof(avictorov_ud.last_name);
		i < PB_MSG_BUFFER_LEN;
	) {
		if (i + sizeof(unsigned long) <= PB_MSG_BUFFER_LEN) {
			unsigned long value = 0;
			if (write(fd, &value, sizeof(value)) < 0) {
				return 14;
			}
			i += sizeof(value);
		} else {
			unsigned char value = 0;
			if (write(fd, &value, sizeof(value)) < 0) {
				return 15;
			}
			i += sizeof(value);
		}
	}

	char returned_data[sizeof(size_t) + sizeof(struct pb_user_data)] = {0};
	size_t* found_id = (size_t*) returned_data;
	struct pb_user_data* found_ud = (struct pb_user_data*) (returned_data + sizeof(size_t));

	if (read(fd, returned_data, sizeof(returned_data)) < (ssize_t) sizeof(returned_data)) {
		return 16;
	}

	if (*found_id != avictorov_id || pb_ud_cmp(&avictorov_ud, found_ud) != 0) {
		return 17;
	}

	// 6. Query a person with specific id, but the second one in list.

	op = PB_OPERATION_FIND_BY_LAST_NAME;
	if (write(fd, &op, sizeof(op)) < 0) {
		return 18;
	}

	id_start = avictorov_id + 1;
	if (write(fd, &id_start, sizeof(id_start)) < 0) {
		return 19;
	}

	if (strncmp(avictorov_ud.last_name, vvictorov_ud.last_name, sizeof(avictorov_ud.last_name)) != 0) {
		return 20;
	}

	if (write(fd, vvictorov_ud.last_name, sizeof(vvictorov_ud.last_name)) < 0) {
		return 21;
	}

	for (
		size_t i = sizeof(op) + sizeof(id_start) + sizeof(vvictorov_ud.last_name);
		i < PB_MSG_BUFFER_LEN;
	) {
		if (i + sizeof(unsigned long) <= PB_MSG_BUFFER_LEN) {
			unsigned long value = 0;
			if (write(fd, &value, sizeof(value)) < 0) {
				return 22;
			}
			i += sizeof(value);
		} else {
			unsigned char value = 0;
			if (write(fd, &value, sizeof(value)) < 0) {
				return 23;
			}
			i += sizeof(value);
		}
	}

	// char returned_data[sizeof(size_t) + sizeof(struct pb_user_data)] = {0};
	memset(returned_data, 0, sizeof(returned_data));
	found_id = (size_t*) returned_data;
	found_ud = (struct pb_user_data*) (returned_data + sizeof(size_t));

	if (read(fd, returned_data, sizeof(returned_data)) < (ssize_t) sizeof(returned_data)) {
		return 24;
	}

	if (*found_id != vvictorov_id || pb_ud_cmp(&vvictorov_ud, found_ud) != 0) {
		return 25;
	}

	// 7. Check get by id works.

	op = PB_OPERATION_FIND_BY_ID;
	if (write(fd, &op, sizeof(op)) < 0) {
		return 26;
	}

	size_t wanted_id = vpavlovich_id;
	if (write(fd, &wanted_id, sizeof(wanted_id)) < 0) {
		return 27;
	}

	for (
		size_t i = sizeof(op) + sizeof(wanted_id);
		i < PB_MSG_BUFFER_LEN;
	) {
		if (i + sizeof(unsigned long) <= PB_MSG_BUFFER_LEN) {
			unsigned long value = 0;
			if (write(fd, &value, sizeof(value)) < 0) {
				return 28;
			}
			i += sizeof(value);
		} else {
			unsigned char value = 0;
			if (write(fd, &value, sizeof(value)) < 0) {
				return 29;
			}
			i += sizeof(value);
		}
	}

	// char returned_data[sizeof(size_t) + sizeof(struct pb_user_data)] = {0};
	memset(returned_data, 0, sizeof(returned_data));
	found_id = (size_t*) returned_data;
	found_ud = (struct pb_user_data*) (returned_data + sizeof(size_t));

	if (read(fd, returned_data, sizeof(returned_data)) < (ssize_t) sizeof(returned_data)) {
		return 30;
	}

	if (*found_id != vpavlovich_id || pb_ud_cmp(&vpavlovich_ud, found_ud) != 0) {
		return 31;
	}

	// 8. Check delete by id works.

	op = PB_OPERATION_DELETE;
	if (write(fd, &op, sizeof(op)) < 0) {
		return 32;
	}

	size_t deleted_id = vpavlovich_id;
	if (write(fd, &deleted_id, sizeof(deleted_id)) < 0) {
		return 33;
	}

	for (
		size_t i = sizeof(op) + sizeof(deleted_id);
		i < PB_MSG_BUFFER_LEN;
	) {
		if (i + sizeof(unsigned long) <= PB_MSG_BUFFER_LEN) {
			unsigned long value = 0;
			if (write(fd, &value, sizeof(value)) < 0) {
				return 34;
			}
			i += sizeof(value);
		} else {
			unsigned char value = 0;
			if (write(fd, &value, sizeof(value)) < 0) {
				return 35;
			}
			i += sizeof(value);
		}
	}

	op = PB_OPERATION_FIND_BY_ID;
	if (write(fd, &op, sizeof(op)) < 0) {
		return 36;
	}

	wanted_id = vpavlovich_id;
	if (write(fd, &wanted_id, sizeof(wanted_id)) < 0) {
		return 37;
	}

	for (
		size_t i = sizeof(op) + sizeof(wanted_id);
		i < PB_MSG_BUFFER_LEN;
	) {
		if (i + sizeof(unsigned long) <= PB_MSG_BUFFER_LEN) {
			unsigned long value = 0;
			if (write(fd, &value, sizeof(value)) < 0) {
				return 38;
			}
			i += sizeof(value);
		} else {
			unsigned char value = 0;
			if (write(fd, &value, sizeof(value)) < 0) {
				return 39;
			}
			i += sizeof(value);
		}
	}


	// char returned_data[sizeof(size_t) + sizeof(struct pb_user_data)] = {0};
	memset(returned_data, 0, sizeof(returned_data));
	found_id = (size_t*) returned_data;
	found_ud = (struct pb_user_data*) (returned_data + sizeof(size_t));

	if (read(fd, returned_data, sizeof(returned_data)) != -ENOENT) {
		return 40;
	}

	return 0;
}

// https://stackoverflow.com/questions/63543127/return-values-in-main-vs-start
//   Saw that in a course my friends had, where they had to write in assembly
//   for linux. They'd define _start and call exit syscall. That's because
//   it's probably (and now we know it's true) necessary, if we return from
//   executable entrypoint.
__attribute__((force_align_arg_pointer))
void _start() {
	int return_code = main();
	exit_group(return_code);
}

#undef ENOENT
#undef write_str
#undef write_int
