# Модуль ядра tsulab

Модуль печатает заданные строки в журнал ядра при загрузке и выгрузке. Пока он загружен, файл `/proc/tsulab` показывает количество процессоров, находящихся онлайн на момент чтения.

Сборка на Ubuntu требует `make`, компилятор C и заголовки **для запущенного ядра**. Если их нет, установите:

```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)
```

В каталоге с `tsulab.c` и `Makefile` выполните:

```bash
make
```

Появится `tsulab.ko`. Для проверки на своей Linux-машине или виртуальной машине:

```bash
sudo insmod ./tsulab.ko
```

```bash
cat /proc/tsulab
```

```bash
sudo dmesg | tail -n 20
```

После проверки выгрузите модуль:

```bash
sudo rmmod tsulab
```

```bash
sudo dmesg | tail -n 20
```

После выгрузки `/proc/tsulab` исчезнет. Сборочные файлы удаляются командой `make clean`.
