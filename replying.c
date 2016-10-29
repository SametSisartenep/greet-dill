#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <libdill.h>
#include <dsock.h>

#define PORT 5555
#define BUFLEN 256

int
main(int argc, char *argv[])
{
	ipaddr addr;
	uint16_t port = PORT;
	int rc, ls, s;
	char bufin[BUFLEN], bufout[BUFLEN];

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

	while (1) {
		s = tcp_accept(ls, NULL, -1);
		assert(s >= 0);

		printf("New connection\n");

		s = crlf_start(s);
		assert(s >= 0);

		rc = msend(s, "What's your name?", 17, -1);
		if (rc != 0)
			goto cleanup;

		ssize_t sz = mrecv(s, bufin, sizeof bufin, -1);
		if (sz < 0)
			goto cleanup;

		bufin[sz] = 0;
		rc = snprintf(bufout, sizeof bufout, "Hello, %s!", bufin);
		rc = msend(s, bufout, rc, -1);
		if (rc != 0)
			goto cleanup;

cleanup:
		rc = hclose(s);
		assert(rc == 0);
	}

	return EXIT_SUCCESS;
}
