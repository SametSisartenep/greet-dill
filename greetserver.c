#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <libdill.h>
#include <dsock.h>

#define PORT 5555

int
main(int argc, char *argv[])
{
	ipaddr addr;
	uint16_t port = PORT;
	int rc, ls, s;

	if (argc > 1) {
		int tmp = atoi(argv[1]);
		if (tmp > 0 && tmp < 65536) {
			port = tmp;
		} else {
			fprintf(stderr, "Invalid port. Using default at %d\n", PORT);
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
		rc = hclose(s);
		assert(rc == 0);
	}

	return EXIT_SUCCESS;
}
