
ALL=test-allhit test-allmiss test-faultsame test-faultdiff
all: $(ALL)

test-allhit:    TEST=ALL_HIT_TLB
test-allmiss:   TEST=ALL_MISS_TLB
test-faultsame: TEST=FAULT_SAME_PAGE
test-faultdiff: TEST=FAULT_DIFFERENT_PAGE

test-%: main.c
	gcc -O3 -g -D$(TEST) $^ -o $@

run: all
	./test-allhit
	./test-allmiss
	./test-faultsame
	./test-faultdiff

clean:
	rm -f $(ALL)
