obj-m           := phonebook.o
# https://stackoverflow.com/questions/12069457/how-to-change-the-extension-of-each-file-in-a-list-with-multiple-extensions-in-g
phonebook-objs  := $(srcs:.c=.o)

EXTRA_CFLAGS := -I$(mkfile_dir)/include
