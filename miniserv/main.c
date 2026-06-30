#include "stdlib.h"
#include "fcntl.h"
#include "sys/select.h"
#include "unistd.h"
#include "stdio.h"
#include "arpa/inet.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "sys/un.h"

int g_cxn = 0; // cnxn id
		
struct Args {
	short port;

	int sfd;
	int cxns[FD_SETSIZE];

	struct sockaddr_in addr;
	socklen_t addr_size;

	fd_set readfds;
	struct timeval tv;
};


int parse_args(struct Args* a, char**argv) {
	char *s = argv[1];
	int p = atoi(s);

	if (0 == p) return 1;
	if (p > 9999) return 1;
	a->port = p;
	return 0;
}

void broadcast(int cxns[FD_SETSIZE], char *msg, int except, struct Args* a) {
	for (int i = 0; i < FD_SETSIZE; i++) {
		if (cxns[i] > 0 && cxns[i] != except) {
			ssize_t n = send(cxns[i], msg, strlen(msg), MSG_DONTWAIT);
			if (n < 0) {
				close(fd);
				a->cxns[i] = 0;
			}
		}
	}
	write(2, msg, strlen(msg));
}

void handle_err(struct Args* a) {
	for (int i = 0; i < FD_SETSIZE; i++) {
		int fd = a->cxns[i];
		if (fd > 0) close(fd);
	}

	if (a->sfd) close(a->sfd);
}

int main (int argc, char** argv) {
	struct Args a = {0};
	a.addr_size = sizeof(a.addr);
	memset(&a.cxns, 0, sizeof(a.cxns));
	char buf[1024];
	memset(&buf, 0, sizeof(buf));
	int opt = 1;

	if (argc != 2) {
		write(2, "Wrong number of arguments\n",sizeof("Wrong number of arguments\n"));
		exit(1);
	}

	if (0 != parse_args(&a, argv)) goto jump_here;

	a.sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == a.sfd) goto jump_here;

	setsockopt(a.sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	a.addr.sin_family = AF_INET;
	a.addr.sin_addr.s_addr = INADDR_ANY;
	a.addr.sin_port = htons(a.port);

	if (0 != bind(a.sfd, (struct sockaddr *) &a.addr, sizeof(a.addr))) goto jump_here;
	if (0 != listen(a.sfd, 10)) goto jump_here;
	if (-1 == fcntl(a.sfd, F_SETFL, O_NONBLOCK)) goto jump_here;

	for (;;) {
		FD_ZERO(&a.readfds);
		FD_SET(a.sfd, &a.readfds);

		//get nfds sentinel
		int nfds = a.sfd;
		for (int i = 0; i < g_cxn; i++) {
			if (a.cxns[i] > nfds)
				nfds = a.cxns[i];
			FD_SET(a.cxns[i], &a.readfds);
		}
		++nfds;

		a.tv.tv_sec = 5;
		a.tv.tv_usec = 0;
		int n_ready = 0;
		fd_set copy = a.readfds;
		int fd;

		n_ready = select(nfds, &copy, NULL, NULL, &a.tv);
		if (n_ready < 0) goto jump_here;
		if (n_ready == 0) { continue;}

		if (FD_ISSET(a.sfd, &copy)) {
			while (1) {
				fd = accept(a.sfd, (struct sockaddr *) &a.addr, &a.addr_size); //must nonblock
				if (fd == -1) break; // no more to accept
				memset(&buf, 0, sizeof(buf));
				sprintf(buf, "server: biatch %d has joined as #%d\n", fd, ++g_cxn);
				broadcast(a.cxns, buf, -1);
				//store it
				for (int i = 0; i < FD_SETSIZE; i++) {
					if (a.cxns[i] == 0) {
						a.cxns[i] = fd;
						break;
					}
				}
				FD_SET(fd, &a.readfds);
			}
		}

		// check for msgs
		for (int i = 0; i < g_cxn; i++) {
			int fd = a.cxns[i];
			if (fd == 0) continue;
			if (FD_ISSET(fd, &copy)) {
				memset(&buf, 0, sizeof(buf));
				sprintf(buf, "client %d: ", i+1);
				int len = strlen(buf);
				ssize_t n = recv(fd, buf+len, sizeof(buf)-len, 0); // this checks for HUPS
				if (n == 0) { //HUP
					memset(&buf, 0, sizeof(buf));
					sprintf(buf, "server: client %d just left\n", i+1);
					broadcast(a.cxns, buf, -1);
					close(a->cxns[i]);
					a->cxns[i] = 0;
					continue;
				}
				if (n < 0) goto jump_here;
				broadcast(a.cxns, buf, fd);
			}
		}
	}

	jump_here:
		write(2, "Fatal error\n", sizeof("Fatal error\n"));
		handle_err(&a);
		exit(1);

	handle_err(&a);
	return 0;
}
