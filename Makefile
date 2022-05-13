
.PHONY: build
build: umka-lang/build/libumka.a
	$(CC) main.c umka-lang/build/libumka.a -lm -ldl -o umprof

.PHONY: clean
clean:
	rm umprof
