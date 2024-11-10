# https://lwn.net/Articles/21823/ (нашел поисками в интернете, чудом)
# https://lwn.net/Articles/21835/ (эту нашел в первой)

# Q: make get directory of the current makefile
# https://yandex.ru/search/?text=make+get+directory+of+the+current+makefile&from=os&clid=1836587&lr=213
# https://stackoverflow.com/questions/18136918/how-to-get-current-relative-directory-of-your-makefile
# https://stackoverflow.com/a/18137056
mkfile_path  := $(abspath $(lastword $(MAKEFILE_LIST)))

# Q: make notdir
# https://yandex.ru/search/?text=make+notdir&from=os&clid=1836587&lr=213
# https://stackoverflow.com/questions/16727021/understanding-a-makefile-with-basename-notdir
# https://stackoverflow.com/a/16727056
# Т.е. notdir не надо, видимо. Просто dir от mkfile_path взять и все (знаю, как некоторые
#   функции работают в make).
mkfile_dir   := $(patsubst %/,%,$(dir $(mkfile_path)))

# Kbuild requires paths relative to this makefile directory.
#   Yet working directory is different, that's why we used
#   absolute paths above.
# If we're just invoking from command line, then relative
#   paths are fine.
# We only need C files, because headers are included in
#   makefiles the compiler outputs.
srcs := $(subst $(mkfile_dir:=/),,                 \
  $(wildcard $(mkfile_dir)/src/*.c))

ifneq ($(KERNELRELEASE),)
include $(mkfile_dir)/kbuild.mk
else
include default.mk
endif

