HEADERS=$(wildcard *.h)
SOURCES=$(wildcard *.c)

clox: $(SOURCES)
	clang -o $@ $^ 
