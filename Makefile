PRJ=webclient
#
PROGS=$(PRJ)
CC=gcc

all: $(PROGS)

$(PRJ): $(PRJ).c
	$(CC) -o $@ $(PRJ).c

clean:
	rm -f *.html *.png *.gif *.jpg $(PRJ)
#