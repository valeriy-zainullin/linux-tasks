# Задачи по курсу "Linux"

Пишем модули для ядра, взаимодействуем с исходниками.

[Задачи](https://docs.google.com/document/d/1nhAlUEMkvF5_fIKIAgFUUdx5BREyQTs4YXZAkKnwmYw/edit?tab=t.0#heading=h.dg3c04n4bqry)

Следующая команда установит все зависимости и запустит тесты каждой задачи последовательно. Достаточно скопировать. Все для вашего удобства.
```
sudo apt install kvm build-essential git && git submodule update --init && ./install_kernel.bash && ./build_initramfs.bash && for dir in task-*; do make -C "$dir" test; done
```

Для того, чтобы протестировать решение задачи, достаточно выполнить
```bash
git submodule update --init && ./install_kernel.bash && ./build_initramfs.bash && make -C task-n test
```
Не забудьте заменить `n` на номер задачи (сейчас решены задачи `1`, `2`, `3`).

Убедитесь, что у вас установлены все зависимости: `qemu-system-x86_64`, `make`, `gcc`, `git`.
```bash
sudo apt install kvm build-essential git
```
