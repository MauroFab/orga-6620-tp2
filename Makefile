tp: main.c cache.c cache.h
	@gcc --std=c99 -Wall -Wpedantic -Werror main.c cache.c -o tp
	@echo "LD tp"

clean:
	@rm -f tp

.PHONY: clean
