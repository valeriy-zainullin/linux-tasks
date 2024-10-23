#include "globals.h"

// Обращения к файлу могут быть из разных потоков.
//   Несколько потоков могут сделать syscall open или
//   write на наш символьный файл, перейдут в режим
//   ядра и попадут в ядро, затем попадут в наш
//   драйвер. Надо защититься от конкурентного
//   обращения к переменным. По умолчанию обращение
//   к ним из нескольких потоков не потокобезопасно.
//   Будет data race (и он является частным случаем
//   race condition).
DEFINE_MUTEX(pb_chardev_mutex);

DEFINE_MUTEX(pb_list_mutex);

// https://stackoverflow.com/questions/33932991/difference-between-mutex-init-and-define-mutex

// Со static тут такая ситуация, что этот .c файл будет
//   собран
