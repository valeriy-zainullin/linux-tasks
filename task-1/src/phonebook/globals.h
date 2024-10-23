#pragma once

#include <linux/fs.h>
#include <linux/mutex.h>

#include "phonebook/phonebook.h"

// Обращения к файлу могут быть из разных потоков.
//   Несколько потоков могут сделать syscall open или
//   write на наш символьный файл, перейдут в режим
//   ядра и попадут в ядро, затем попадут в наш
//   драйвер. Надо защититься от конкурентного
//   обращения к переменным. По умолчанию обращение
//   к ним из нескольких потоков не потокобезопасно.
//   Будет data race (и он является частным случаем
//   race condition).
extern struct mutex pb_chardev_mutex;

// Mutex на структуру данных с телефонной книгой.
extern struct mutex pb_list_mutex;

extern bool* phonebook_item_used;
extern size_t* phonebook_items;

extern int debug;

extern struct file_operations fops;

bool   pb_add(size_t* id, const struct pb_user_data* ud);
bool   pb_get_by_id(size_t id, struct pb_user_data* ud);
bool   pb_get_geq_id_ud_with_last_name(size_t id_start, const char* last_name, size_t *id, struct pb_user_data* ud);
bool   pb_remove_by_id(size_t id);
size_t pb_remove_by_last_name(const char* last_name);


// Со static тут такая ситуация, что этот .c файл будет
//   собран

