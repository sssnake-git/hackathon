CFLAGS := -g -fPIC

all : test.bin

VADOBJ := vad.o

test.bin : test_vad.c libvad.a
	gcc $(CFLAGS) -L./ $< -o $@ -lvad

libvad.a : $(VADOBJ)
	ar -r $@ $^

# %.o : %.c
# 	gcc $(CFLAGS) -c $< -o $@
$(VADOBJ) : vad.c
	gcc $(CFLAGS) -c $^ -o $@

.PHONY : all
clean:
	$(RM) *.o *.a *.bin
