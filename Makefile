all: dodance
	logo runme

dodance: turtledance input
	./turtledance < input > runme

turtledance: turtledance.c
	$(CC) -std=c99 -g turtledance.c -o turtledance

clean:
	@-rm runme turtledance >/dev/null 2>&1
