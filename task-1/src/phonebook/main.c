#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mutex.h>

#include "globals.h"
#include "phonebook/phonebook.h"

// Ссылки.
// https://stackoverflow.com/a/29454071
// https://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device/
// https://www.kernel.org/doc/html/latest/core-api/kernel-api.html
// https://www.kernel.org/doc/html/latest/search.html?q=device_create

// Хороший ресурс, но я поздно нашел, почти не воспользовался.
//   https://ruvds.com/wp-content/uploads/2022/10/The-Linux-Kernel-Module-Programming-Guide-ru.pdf

MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");

int debug = 0;
module_param(debug, int, 0);
MODULE_PARM_DESC(debug, "print debugging information");

// Сага о лицензии в license-sage.txt.

static __init int  pb_init(void);
static __exit void pb_cleanup(void);

static const char* const DEVICE_CLASS_NAME = "phonebook";
static const char* const DEVICE_NAME       = "pbchar";    // Device will appear as /dev/pbchar

static int major_number = 0;
static struct class* device_class = NULL;
static struct device* device = NULL;

static __init int pb_init(void) {
  /* From docs for __register_chrdev.
     The name of this device has nothing to do with the name of
     the device in /dev. It only helps to keep track of the
     different owners of devices. If your module name has only
     one type of devices it’s ok to use e.g. the name of the
     module here. */
  // So we're registering a type of devices! phoebook character
  //   devices..
  // Во второй ссылке говорят, что здесь выделяем major number.
  //   Не понятно, почему такой подход. Не одновременно с классом
  //   устройств указываем обработчики операций над устройством.
  // В нашем классе устройств будет одно устройство. Его номер
  //   будет начинаться с 0. Т.е. получится идентификатор устройства
  //   X.0, где X будет выдано ядром, называется major number для этих
  //   устройств.
  // Я сначала думал, что это одна и так же функция, не обратил внимание
  //   на подверкивания. Я зашел в функцию register_chrdev в sublime
  //   text (перешел к определению) и увидел, что это обертка над
  //   __register_chrdev с параметрами 0 256. Первый индекс начинается
  //   с нуля, всего 256 устройств можно создать. Ну, ок.
  major_number = register_chrdev(0, DEVICE_CLASS_NAME, &fops);
  if (major_number < 0) {
    printk(KERN_ALERT "phonebook failed to register a major number.\n");
    return major_number;
  }

  // Видимо, в новых версиях ядра THIS_MODULE передавать не надо..
  //   Возможно, теперь модуль, который вызвал функцию,
  //   определяется автоматически.
  // Ксатати, если интересно, где определяется THIS_MODULE, то это
  //   позволяет понять ошибка компиляции, с которой я столкнулся.
  //   Видимо, Kbuild, механизм сборки ядра, проставляет, какой
  //   это модуль. Либо это поле заполняет ядро после загрузки
  //   модуля.
  /*   In file included from ./include/linux/printk.h:6,
                      from ./include/linux/kernel.h:31,
                      from /home/user/linux-course/task-1//include/phonebook/phonebook.h:3,
                      from /home/user/linux-course/task-1/phonebook/main.c:1:
     /home/user/linux-course/task-1/phonebook/main.c: В функции «pb_init»:
     ./include/linux/init.h:180:22: ошибка: в передаче аргумента 1 «class_create»: несовместимый тип указателя [-Wincompatible-pointer-types]
       180 | #define THIS_MODULE (&__this_module)
           |                     ~^~~~~~~~~~~~~~~
           |                      |
           |                      struct module *
     /home/user/linux-course/task-1/phonebook/main.c:52:31: замечание: в расширении макроса «THIS_MODULE»
        52 |   device_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME);
       In file included from /home/user/linux-course/task-1/phonebook/main.c:3:
     ./include/linux/device/class.h:228:54: замечание: ожидался тип «const char *», но аргумент имеет тип «struct module *»
       228 | struct class * __must_check class_create(const char *name);  */
  device_class = class_create(DEVICE_CLASS_NAME);
  if (IS_ERR(device_class)) {
    /* /home/user/linux-course/task-1/phonebook/main.c:76:5: ошибка: слишком много аргументов в вызове функции «unregister_chrdev»
        76 |     unregister_chrdev(major_number, 0, 1, DEVICE_CLASS_NAME);
           |     ^~~~~~~~~~~~~~~~~
     In file included from ./include/linux/compat.h:17,
                      from ./arch/x86/include/asm/ia32.h:7,
                      from ./arch/x86/include/asm/elf.h:10,
                      from ./include/linux/elf.h:6,
                      from ./include/linux/module.h:19,
                      from /home/user/linux-course/task-1/phonebook/main.c:4:
     ./include/linux/fs.h:2779:20: замечание: объявлено здесь
      2779 | static inline void unregister_chrdev(unsigned int major, const char *name)
    */
    unregister_chrdev(major_number, DEVICE_CLASS_NAME);
    printk(KERN_ALERT "phonebook failed to register device class.\n");
    return PTR_ERR(device_class);
  }

  // По всей видимости класс нужен именно для удобства. Так-то мы уже выделили
  //   major_number исходя из функционала, как понимаю. Мы ведь туда передали
  //   обработчики операций.
  // В документации на device_create легким нажатием на ссылку на определение
  //   структуры class видим, что в ней есть имя, какие-то группы, обработчики
  //   операций питания. Скорее всего, так сделано из-за того, что почти
  //   всем устройствам приходиться думать о питании. Т.е. это все не просто
  //   так, видимо. Мы сделали функциональную работу, а еще бывает питание.
  //   И оно мб общее, а вот major_number мы получаем в определенной подсистеме.
  //   Я ожидаю, что сетевую карту что мы будем регистрировать у сетевого стека,
  //   указывать там обработчики, они для сетевушки другие должны быть. И там
  //   получим major_number.
  // По всей видимости при выделении major_number мы просто указали читаемое
  //   имя для устройств с таким функционалом. А конкретная реализация с особенностями
  //   по питанию, например, будет определяться device_class. Просто решили не создавать
  //   еще одно имя для устройств с таким функционалом, потому передаем DEVICE_CLASS_NAME
  //   в register_chrdev.

  device = device_create(device_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
  if (IS_ERR(device)) {
    // ErrorOr по сишному! Ядро уж может некоторые адреса в виртуальной памяти
    //   зарезервировать для ошибок :)
    //   Зато в таком случае не проверять на ошибки очень плохо. Т.к. можно ходить по
    //   невалидному указателю. С NULL так же. Но здесь усть мотивация внимательно
    //   смотреть на возвращаемое значение, т.к. выдается не просто NULL, а какая-то
    //   полезная информация в виде указателя.
    class_destroy(device_class);
    unregister_chrdev(major_number, DEVICE_CLASS_NAME);
    printk(KERN_ALERT "phonebook failed to create the device\n");
    return PTR_ERR(device); // Конвертируем указатель в числовой код ошибки.
  }

  // На самом деле PTR_ERR возвращает long, а не int, судя по документации на kernel.org.
  //   Потому по-хорошему нам нужно поменять тип возвращаемого значения из функции. Насколько
  //   знаю, на linux long зависит от архитектуры. Машинное слово всегда, на x86 32 бита, на
  //   x86_64 -- 64 бита. Возможно, модуль может возвращать long, а не int. Надо посмотреть.
  // Narrowing signed integer conversion in c is potentially unsafe and implementation defined!
  //   https://yandex.ru/search/?text=narrowing+signed+integer+conversion+c&from=os&clid=1836587&lr=213
  //   Мы не хотим зависеть от компилятора, архитектуры.. Надо разобраться.

  mutex_init(&pb_chardev_mutex);
  mutex_init(&pb_list_mutex);

  printk(KERN_INFO "phonebook is up.\n");

  return 0;
}

/* Из второй ссылки.
   The __exit macro notifies that if this code is used for a built-in driver (not a LKM) that
   this function is not required.
*/
static __exit void pb_cleanup(void) {
  device_destroy(device_class, MKDEV(major_number, 0));
  class_unregister(device_class);
  class_destroy(device_class);
  unregister_chrdev(major_number, DEVICE_CLASS_NAME);

  mutex_destroy(&pb_list_mutex);
  mutex_destroy(&pb_chardev_mutex);

  printk(KERN_INFO "phonebook is down.\n");
}

module_init(pb_init);
module_exit(pb_cleanup);
