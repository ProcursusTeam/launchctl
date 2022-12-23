PREFIX  ?= 
DESTDIR ?=

SRC    := launchctl.c xpc_helper.c start_stop.c print.c env.c load.c
SRC    += enable.c reboot.c bootstrap.c error.c remove.c kickstart.c kill.c
LDLIBS := 

all: launchctl

launchctl: $(SRC:%.c=%.o)
	$(CC) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@
	ldid -Slaunchctl.xml -Cadhoc launchctl

clean:
	rm -f launchctl $(SRC:%.c=%.o)

install: launchctl
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m744 launchctl $(DESTDIR)$(PREFIX)/bin/launchctl

