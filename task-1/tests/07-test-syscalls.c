// These a probably host system headers. But system calls
//   we're going to use (read, write, open, close) are in the
//   linux kernel for a long time. So numbers should be the same.
// https://filippo.io/linux-syscall-table/
#define _GNU_SOURCE
#include <unistd.h>

#include "phonebook/phonebook.h"

// Exits all threads in current thread group (kind of like a process in windows).
//   Exit syscall would exit just the current one (which works for us, but
//   let's be closer to libc..)
// static const long SYS_EXIT_GROUP = 231;

// Может понадобится для отладки. Пусть будет.
__attribute__((unused))
static const long STDOUT_FD = 1; // Это и из опыта с командной оболочкой знаем.

typedef unsigned long size_t;

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

static long add_user(const struct pb_user_data* ud) {
	ssize_t result = 0;
	__asm__ __volatile__ (
		"movq $463, %%rax\n"
		"movq %1,   %%rdi\n"
		"syscall\n"
		"movq %%rax, %0\n"
		: "=m"(result)
		: "m"(ud)
		: "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
	);
	return result;
}

static long get_user(const char* last_name, size_t len, struct pb_user_data* ud) {
	ssize_t result = 0;
	__asm__ __volatile__ (
		"movq $464,  %%rax\n"
		"movq %1,    %%rdi\n"
		"movq %2,    %%rsi\n"
		"movq %3,    %%rdx\n"
		"syscall\n"
		"movq %%rax, %0\n"
		: "=m"(result)
		: "m"(last_name), "m"(len), "m"(ud)
		: "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
	);
	return result;
}

static long del_user(const char* last_name, size_t len) {
	ssize_t result = 0;
	__asm__ __volatile__ (
		"movq $465,  %%rax\n"
		"movq %1,    %%rdi\n"
		"movq %2,    %%rsi\n"
		"syscall\n"
		"movq %%rax, %0\n"
		: "=m"(result)
		: "m"(last_name), "m"(len)
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
	// 1. Add a person.

	struct pb_user_data avictorov_ud = {
		.first_name = "Александр",
		.last_name  = "Викторов",
		.age        = 21,
		.telnum     = "+79818181111",
		.email      = "alexander.victorov@pushkapochta.ru"
	};
	size_t avictorov_id = 0;

	long result = 0;

	result = add_user(&avictorov_ud);
	if (result < 0) {
		return 1;
	}
	avictorov_id = (size_t) result;

	// 2. Add another person with the same last name.

	struct pb_user_data vvictorov_ud = {
		.first_name = "Виктор",
		.last_name  = "Викторов",
		.age        = 22,
		.telnum     = "+79818181111",
		.email      = "victor.victorov@pushkapochta.ru"
	};
	__attribute__((unused))
	size_t vvictorov_id = 0;

	result = add_user(&vvictorov_ud);
	if (result < 0) {
		return 1;
	}
	vvictorov_id = (size_t) result;

	// 3. Add a person with a fresh last name.
 
	struct pb_user_data vpavlovich_ud = {
		.first_name = "Виктор",
		.last_name  = "Павлович",
		.age        = 23,
		.telnum     = "+79818181111",
		.email      = "victor.pavlovich@pushkapochta.ru"
	};
	size_t vpavlovich_id = 0;

	result = add_user(&vpavlovich_ud);
	if (result < 0) {
		return 3;
	}
	vpavlovich_id = (size_t) result;

	// 4. Ambiguously query a person by last name. Should get the earliest added.
	//   It has the lowest id.

	struct pb_user_data found_ud = {0};

	result = get_user(avictorov_ud.last_name, sizeof(avictorov_ud.last_name), &found_ud);
	if (result < 0) {
		return 4;
	}
	size_t found_id = (size_t) result;

	if (found_id != avictorov_id || pb_ud_cmp(&found_ud, &avictorov_ud) != 0) {
		return 5;
	}

	// 5. Unambiguosly query a person by last name.

	// struct pb_user_data found_ud = {0};
	memset(&found_ud, 0, sizeof(found_ud));
	found_id = 0;

	// Специально sizeof напишу, чтобы в конце были лишние нулевые байты.
	result = get_user(vpavlovich_ud.last_name, sizeof(vpavlovich_ud.last_name), &found_ud);
	if (result < 0) {
		return 6;
	}
	// size_t found_id = (size_t) result;
	found_id = (size_t) result;

	if (found_id != vpavlovich_id || pb_ud_cmp(&found_ud, &vpavlovich_ud) != 0) {
		return 7;
	}

	// 6. Delete by last name. Should delete all people with that last name.

	result = del_user(avictorov_ud.last_name, sizeof(avictorov_ud.last_name));
	if (result < 0) {
		return 8;
	}
	size_t num_deleted = (size_t) result;

	if (num_deleted != 2 || get_user(vvictorov_ud.last_name, strlen(vvictorov_ud.last_name), &found_ud) != -ENOENT) {
		return 9;
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
