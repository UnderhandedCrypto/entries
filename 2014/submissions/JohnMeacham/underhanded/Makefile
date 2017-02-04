CC=gcc
TARGET= aesctr
SRC= $(TARGET).c tinyaesctr.c aes_tables.c
OBJS:=	$(patsubst %.c,obj/%.o,$(SRC))
DEPFILES:=$(patsubst %.o,%.d,$(OBJS))
LDLIBS=
CFLAGS= -fno-common -Wall -g -std=gnu99 -I. # -DNDEBUG
all: $(TARGET) aesctrtest exploit/exploit

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

aesctrtest: obj/aesctrtest.o obj/tinyaesctr.o obj/aes_tables.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

exploit/exploit: obj/exploit/exploit.o  obj/tinyaesctr.o obj/aes_tables.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

obj/%.o : %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
obj/t/%.o : %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@ -DTESTING

.PHONY : clean
clean:
	rm -f -- $(OBJS) $(DEPFILES) $(TARGET)
-include $(DEPFILES)
