lib-y	:= delay.o memcpy_$(BITS).o memset_$(BITS).o uaccess_$(BITS).o

ifeq ($(CONFIG_64BIT),)
lib-y += ashldi3.o ashrdi3.o lshrdi3.o
endif
