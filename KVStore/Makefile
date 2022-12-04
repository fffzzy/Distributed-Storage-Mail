TARGETS = kvstore-test

kvstore-test: kvstore-test.cc kvstore.cc
	g++ $^ -o $@

clean::
	rm -fv $(TARGETS)