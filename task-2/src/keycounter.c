#include <asm/io.h>
// #include <asm-generic/io.h> // А вот если этот подключить вместо <asm/io.h>,
//    то будут проблемы, хотя там тоже inb есть. Это мб инструкция inb, которую
//    нет прав исполнять в нашем коде..

#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/irqreturn.h>

MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");

int logging_period_ms = 1000;
module_param(logging_period_ms, uint, 0);
MODULE_PARM_DESC(logging_period_ms, "period of the logging in ms");

static const int KEYBOARD_IRQ = 1;

static atomic_t          key_press_counter = ATOMIC_INIT(0);
static struct timer_list logging_timer;

static const int state_started = 0;
static const int state_exiting = 1;
static const int state_exited  = 2;
static atomic_t  state = ATOMIC_INIT(state_started);

static irqreturn_t handle_keyboard_irq(int irq, void* dev) {
	(void) dev;

	if (unlikely(irq != KEYBOARD_IRQ)) {
		// Ошибка какая-то.. Такое в реальности не должно происходить, в
		//  каком-то смысле защита просто. Потому unlikely. 
		// Поискал определение IRQ_HANDLED (с помощью перехода к определению от
		//   sublime text, просто по токенам ищет, скорее всего).
		/*
		 * enum irqreturn - irqreturn type values
		 * @IRQ_NONE:		interrupt was not from this device or was not handled
		 * @IRQ_HANDLED:	interrupt was handled by this device
		 * @IRQ_WAKE_THREAD:	handler requests to wake the handler thread
		*/
		return IRQ_NONE;
	}

	// IRQ 1: прерывание клавиатуры.
	//   https://web.archive.org/web/20240822174557/http://wiki.osdev.org/Interrupts
	// В DOS нужно было читать порт в прерывании. Видимо, у нас так же,
	//   исторически сложилось. Там тоже был пор 0x60.
	char code = inb(0x60);

	// На занятии обсуждали это и еще из таблицы видим, что
	//   отпускания клавиши начинаются с 0x80.
	//   https://web.archive.org/web/20240910031309/https://wiki.osdev.org/PS/2_Keyboard#Scan_Code_Set_1

	if (code < 0x80) {
		// Это нажатие клавиши, а не отпускание. Увеличим счетчик.
		atomic_inc(&key_press_counter);
	}
	
	return IRQ_HANDLED;
}

static void log_counter(struct timer_list* timer) {
	(void) timer;

	int counter = atomic_xchg(&key_press_counter, 0);
	printk(KERN_INFO "keycounter: %d key presses!\n", counter);

	if (atomic_cmpxchg(&state, state_exiting, state_exited) == state_exiting) {
		// Был запрос на выход. Проставили, что запрос увидели.
		//   Не будем себя планировать на следующую итерацию.
	} else {
		// Запланируемся на следующий период.
		mod_timer(&logging_timer, jiffies + msecs_to_jiffies(logging_period_ms));
	}
}

static int __init kc_init(void) {
	if (logging_period_ms <= 200) {
		printk(KERN_WARNING "keycounter: invalid logging period %d ms", logging_period_ms);
		return -EINVAL;
	}

	int ret_value = 0;

	// https://docs.kernel.org/core-api/genericirq.html#c.request_irq

	// <linux-src/include/linux/interrupt.h
	//   * IRQF_SHARED - allow sharing the irq among several devices
	// Мы здесь притворяемся, что на прерывании клавиатуры сидит еще какое-то
	//   устройство. Видимо, такое бывает, что несколько устройств занимают
	//   одно прерывание. И устанавливаем свой обработчик. А потом бац и
	//   читаем все тот же порт клавиатуры. Никакого нового устройства нет..
	// Тут даже аргумент для дополнительных данных вызываемой функции называется
	//   `dev`. Т.е. какое-то описание устройства, мб структура.
	// Должны ли мы в таком случае возвращать IRQ_NONE, чтобы оригинальный
	//   обработчик вызывался? Вопрос.. Попробую посмотреть в коде. По идее,
	//   можно вводить в консоль с клавиатуры во время работы нашего модуля,
	//   оригинальному обработчику мы не мешаем..
	// Из реализации request_irq в `manage.c`.
	/*
	 * Sanity-check: shared interrupts must pass in a real dev-ID,
	 * otherwise we'll have trouble later trying to figure out
	 * which interrupt is which (messes up the interrupt freeing
	 * logic etc).
	 *
	 * Also shared interrupts do not go well with disabling auto enable.
	 * The sharing interrupt might request it while it's still disabled
	 * and then wait for interrupts forever.
	 *
	 * Also IRQF_COND_SUSPEND only makes sense for shared interrupts and
	 * it cannot be set along with IRQF_NO_SUSPEND.
	 */
	// if (((irqflags & IRQF_SHARED) && !dev_id) ||
	//     ((irqflags & IRQF_SHARED) && (irqflags & IRQF_NO_AUTOEN)) ||
	//     (!(irqflags & IRQF_SHARED) && (irqflags & IRQF_COND_SUSPEND)) ||
	//     ((irqflags & IRQF_NO_SUSPEND) && (irqflags & IRQF_COND_SUSPEND)))
	// 	return -EINVAL;
	// Т.е. видимо при удалении обработчика передается указатель устройства.
	//   Надеюсь, внутрь этого указателя не смотрят.. Т.к. иначе они не угадают,
	//   что внутри функция.
	ret_value = request_irq(KEYBOARD_IRQ, &handle_keyboard_irq, IRQF_SHARED, "PS/2 keyboard", &handle_keyboard_irq);

	if (ret_value != 0) {
		printk(KERN_WARNING "keycounter: failed to register irq handler.\n");
		return ret_value;
	}

	// https://stackoverflow.com/questions/10812858/how-to-use-timers-in-linux-kernel-device-drivers
	// https://www.google.com/search?channel=fs&client=ubuntu-sn&q=what+is+jiffies+linux
	/* The interval between two system timer interrupt ticks is known as a jiffy in
	     the Linux kernel. Think of them as the heartbeat of the kernel, marking
	     regular intervals at which the kernel performs essential timekeeping tasks.
	     The jiffies hold the number of ticks that have occurred since the system
	     booted.
	*/
	//   https://blogs.oracle.com/linux/post/jiffies-the-heartbeat-of-the-linux-operating-system

	// Инициализируем таймер.
	timer_setup(&logging_timer, log_counter, 0);

	// Заведем его.
	//   Ему передается номер jiffy, на котором ему надо сработать.
	mod_timer(&logging_timer, jiffies + msecs_to_jiffies(logging_period_ms));

	printk(KERN_INFO "keycounter is up, logging period is %d ms", logging_period_ms);

	return 0;
}

static void __exit kc_cleanup(void) {
	atomic_set(&state, state_exiting);
	while (atomic_read(&state) != state_exited) {
		/* крутимся на цикле, ждем следующей итерации таймера. */
	}
	// Если не сделать, возможно, модуль уже выгрузят, а таймер продолжит
	//   работать, будет page fault..

	// From <linux-src>/kernel/time/timer.c
	//   Thanks to sublime text, things are easy to find, just
	//   hover the token you want to find in the codebase.
	/**
	 * timer_delete - Deactivate a timer
	 * @timer:	The timer to be deactivated
	 *
	 * The function only deactivates a pending timer, but contrary to
	 * timer_delete_sync() it does not take into account whether the timer's
	 * callback function is concurrently executed on a different CPU or not.
	 * It neither prevents rearming of the timer.  If @timer can be rearmed
	 * concurrently then the return value of this function is meaningless.
	 *
	 * Return:
	 * * %0 - The timer was not pending
	 * * %1 - The timer was pending and deactivated
	 */
	// Деактивирует запланированный запуск, но если таймер сейчас работает на
	//   другом CPU, он все еще может быть запланирован. Если таймер работает
	//   на текущем CPU, а внутри есть атомик, исполняется ли таймер, то это
	//   может быть реализовано так: compare exchange timer_running на timer_stop
	//   в цикле. Тогда он дождется, пока таймер не будет исполняться и вместе с
	//   этим сразу же остановит все последующие итерации.
	timer_delete(&logging_timer);

	free_irq(KEYBOARD_IRQ, handle_keyboard_irq);
}

module_init(kc_init);
module_exit(kc_cleanup);
