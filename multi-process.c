#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <libdill.h>
#include <dsock.h>

#define PORT 5555
#define NPROC 1
#define BUFLEN 256

enum {
	CONN_ESTABLISHED = 1,
	CONN_SUCCEEDED,
	CONN_FAILED
};

coroutine void dialogue(int, int);
coroutine void statistics(int);
coroutine void accepter(int);

coroutine void
dialogue(int s, int ch)
{
	int op = CONN_ESTABLISHED;
	int rc = chsend(ch, &op, sizeof op, -1);
	assert(rc == 0);

	char bufin[BUFLEN], bufout[BUFLEN];
	int64_t d = now() + 10000;

	rc = msend(s, "What's your name?", 17, -1);
	if (rc != 0)
		goto cleanup;

	ssize_t sz = mrecv(s, bufin, sizeof bufin, d);
	if (sz < 0)
		goto cleanup;

	bufin[sz] = 0;
	rc = snprintf(bufout, sizeof bufout, "Hello, %s!", bufin);
	rc = msend(s, bufout, rc, -1);
	if (rc != 0)
		goto cleanup;

cleanup:
	op = errno == 0 ? CONN_SUCCEEDED : CONN_FAILED;
	rc = chsend(ch, &op, sizeof op, -1);
	assert(rc == 0);
	rc = hclose(s);
	assert(rc == 0);
}

coroutine void
statistics(int ch)
{
	int active = 0;
	int succeeded = 0;
	int failed = 0;

	while (1) {
		int op;
		int rc = chrecv(ch, &op, sizeof op, -1);
		assert(rc == 0);

		switch (op) {
		case CONN_ESTABLISHED:
			++active;
			break;
		case CONN_SUCCEEDED:
			--active;
			++succeeded;
			break;
		case CONN_FAILED:
			--active;
			++failed;
			break;
		}

		printf("pid: %-5d active: %-5d | succeeded: %-5d |"
			" failed: %-5d\n", (int)getpid(), active,
			succeeded, failed);
	}
}

coroutine void
accepter(int ls)
{
	int ch = channel(sizeof(int), 0);
	assert(ch >= 0);
	int cr = go(statistics(ch));
	assert(cr >= 0);
	
	while (1) {
		int s = tcp_accept(ls, NULL, -1);
		assert(s >= 0);
		s = crlf_start(s);
		assert(s >= 0);
		
		cr = go(dialogue(s, ch));
		assert(cr >= 0);
	}
}

int
main(int argc, char *argv[])
{
	ipaddr addr;
	int i, rc, ls;
	uint16_t port = PORT;

	if (argc > 1) {
		int tmp = atoi(argv[1]);
		if (tmp > 0 && tmp < 65536) {
			port = tmp;
		} else {
			fprintf(stderr,
			        "Invalid port. Using default at %d\n",
			        PORT);
		}
	}

	rc = ipaddr_local(&addr, NULL, port, 0);
	assert(rc == 0);

	ls = tcp_listen(&addr, 10);
	if (ls < 0) {
		perror("Can't open listening socket");
		return EXIT_FAILURE;
	}
	
	for (i = 0; i < NPROC - 1; i++) {
		int cr = proc(accepter(ls));
		assert(cr >= 0);
	}

	accepter(ls);

	return EXIT_SUCCESS;
}
