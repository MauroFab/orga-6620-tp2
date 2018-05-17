tp: main.c cache.c cache.h
	@gcc -ggdb -std=c99 -Wall -Wpedantic -Werror main.c cache.c -o tp -lm
	@echo "LD tp"

clean:
	@rm -f tp

.PHONY: clean
