/**
 * Simple serial port module
 * (c) Sergey Shcherbakov <shchers@gmail.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

// Storage of the original terminal properties
static struct termios origTermInfo;

int OpenPort(const char *portName) {
	int fd;
	struct termios attr;

	fprintf(stderr, "Port name: %s\n", portName);
	if ((fd = open(portName, O_RDWR)) == -1) {
		fprintf(stderr, "[%s]: open() failed with error \"%s\"\n",
				__func__, strerror(errno));
		return -1;
	}
	if (tcgetattr(fd, &origTermInfo) == -1) {
		fprintf(stderr, "[%s]: tcgetattr() failed with error \"%s\"\n",
				__func__, strerror(errno));
		goto error;
	}
	attr = origTermInfo;
	attr.c_cflag |= /*CRTSCTS |*/ CLOCAL;
	attr.c_oflag = 0;
	if (tcflush(fd, TCIOFLUSH) == -1) {
		fprintf(stderr, "[%s]: tcflush() failed with error \"%s\"\n",
				__func__, strerror(errno));
		goto error;
	}

	if (tcsetattr(fd, TCSANOW, &attr) == -1) {
		fprintf(stderr, "[%s]: tcsetattr() failed with error \"%s\"\n",
				__func__, strerror(errno));
		goto error;
	}

	return fd;

error:
	if (close(fd)) {
		fprintf(stderr, "[%s]: Closing port file descriptor (%d) failed with error \"%s\"\n",
				__func__, fd, strerror(errno));
	}

	return -1;
}

void ClosePort(int fd) {
	tcsetattr(fd, TCSANOW, &origTermInfo);
	if (close(fd)) {
		fprintf(stderr, "[%s]: Closing port file descriptor (%d) failed with error \"%s\"\n",
				__func__, fd, strerror(errno));
	}
}

int updateRts(int fd, int bSet) {
	int status;

	if (ioctl(fd, TIOCMGET, &status) == -1) {
		fprintf(stderr, "[%s]: TIOCMGET failed with error \"%s\"\n",
				__func__, strerror(errno));
		return 0;
	}

	if (bSet)
		status |= TIOCM_RTS;
	else
		status &= ~TIOCM_RTS;

	if (ioctl(fd, TIOCMSET, &status) == -1) {
		fprintf(stderr, "[%s]: TIOCMSET failed with error \"%s\"\n",
				__func__, strerror(errno));
		return 0;
	}

	return 1;
}

int updateDtr(int fd, int bSet) {
	int status;

	if (ioctl(fd, TIOCMGET, &status) == -1) {
		fprintf(stderr, "[%s]: TIOCMGET failed with error \"%s\"\n",
				__func__, strerror(errno));
		return 0;
	}

	if (bSet)
		status |= TIOCM_DTR;
	else
		status &= ~TIOCM_DTR;

	if (ioctl(fd, TIOCMSET, &status) == -1) {
		fprintf(stderr, "[%s]: TIOCMSET failed with error \"%s\"\n",
				__func__, strerror(errno));
		return 0;
	}

	return 1;
}
