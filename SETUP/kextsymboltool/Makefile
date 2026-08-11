export MakeInc_cmd=${SRCROOT}/makedefs/MakeInc.cmd
export MakeInc_def=${SRCROOT}/makedefs/MakeInc.def
export MakeInc_rule=${SRCROOT}/makedefs/MakeInc.rule
export MakeInc_dir=${SRCROOT}/makedefs/MakeInc.dir

include $(MakeInc_cmd)
include $(MakeInc_def)

OBJS = kextsymboltool.o

CFLAGS = -isysroot $(HOST_SDKROOT) -mmacosx-version-min=$(HOST_OS_VERSION) -g -O0 -I$(SOURCE) -I.
CFLAGS += $(PUREDARWIN_HOST_CFLAGS)
ifneq ($(PUREDARWIN_HOST_CCTOOLS_INCLUDE),)
CFLAGS += -I$(PUREDARWIN_HOST_CCTOOLS_INCLUDE) -I$(PUREDARWIN_HOST_CCTOOLS_INCLUDE)/foreign
endif

WARNFLAGS = -Wall

LDFLAGS = -isysroot $(HOST_SDKROOT) -mmacosx-version-min=$(HOST_OS_VERSION) -lstdc++
ifneq ($(PUREDARWIN_HOST_LIBMACHO),)
LDLIBS += $(PUREDARWIN_HOST_LIBMACHO) $(PUREDARWIN_HOST_LIBSTUFF) -luuid
endif

kextsymboltool: $(OBJS)
	@echo "$(ColorH)HOST_LD$(Color0)       $(ColorF)$@$(Color0)"
	$(_v)$(HOST_CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	@echo "$(ColorH)HOST_CODESIGN$(Color0) $(ColorF)$@$(Color0)"
	$(_v)env CODESIGN_ALLOCATE=$(HOST_CODESIGN_ALLOCATE) $(HOST_CODESIGN) -s - $@

%.o: %.c
	@echo "$(ColorH)HOST_CC$(Color0)       $(ColorF)$@$(Color0)"
	$(_v)$(HOST_CC) $(WARNFLAGS) $(CFLAGS) -c -o $@ $<

do_build_setup:: kextsymboltool

include $(MakeInc_rule)
include $(MakeInc_dir)
