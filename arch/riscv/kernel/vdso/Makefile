# Derived from arch/{arm64,tile}/kernel/vdso/Makefile

obj-vdso := sigreturn.o

# Build rules
targets := $(obj-vdso) vdso.so vdso.so.dbg
obj-vdso := $(addprefix $(obj)/, $(obj-vdso))

#ccflags-y := -shared -fno-common -fno-builtin
#ccflags-y += -nostdlib -Wl,-soname=linux-vdso.so.1 \
		$(call cc-ldoption, -Wl$(comma)--hash-style=sysv)

CFLAGS_vdso.so = $(c_flags)
CFLAGS_vdso.so.dbg = -shared -s -Wl,-soname=linux-vdso.so.1 \
	$(call cc-ldoption, -Wl$(comma)--hash-style=sysv)
CFLAGS_vdso_syms.o = -r

obj-y += vdso.o

# We also create a special relocatable object that should mirror the symbol
# table and layout of the linked DSO.  With ld -R we can then refer to
# these symbols in the kernel code rather than hand-coded addresses.
extra-y += vdso.lds vdso-syms.o
$(obj)/built-in.o: $(obj)/vdso-syms.o
$(obj)/built-in.o: ld_flags += -R $(obj)/vdso-syms.o

CPPFLAGS_vdso.lds += -P -C -U$(ARCH)

# Force dependency
$(obj)/vdso.o : $(obj)/vdso.so

# Link rule for the *.so file; *.lds must be first
$(obj)/vdso.so.dbg: $(src)/vdso.lds $(obj-vdso)
	$(call if_changed,vdsold)
$(obj)/vdso-syms.o: $(src)/vdso.lds $(obj-vdso)
	$(call if_changed,vdsold)

# Strip rule for the *.so file
$(obj)/%.so: OBJCOPYFLAGS := -S
$(obj)/%.so: $(obj)/%.so.dbg FORCE
	$(call if_changed,objcopy)

# Assembly rules for the *.S files
$(obj-vdso): %.o: %.S
	$(call if_changed_dep,vdsoas)

# Actual build commands
quiet_cmd_vdsold = VDSOLD  $@
      cmd_vdsold = $(CC) $(c_flags) -nostdlib $(CFLAGS_$(@F)) -Wl,-n -Wl,-T $^ -o $@
quiet_cmd_vdsoas = VDSOAS  $@
      cmd_vdsoas = $(CC) $(a_flags) -c -o $@ $<

# Install commands for the unstripped file
quiet_cmd_vdso_install = INSTALL $@
      cmd_vdso_install = cp $(obj)/$@.dbg $(MODLIB)/vdso/$@

vdso.so: $(obj)/vdso.so.dbg
	@mkdir -p $(MODLIB)/vdso
	$(call cmd,vdso_install)

vdso_install: vdso.so
