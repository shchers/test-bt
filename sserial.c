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
#include "sserial.h"

struct sserial_props *OpenPort(const char *portName) {
	struct sserial_props *pProps;
	struct termios attr;

	pProps = (struct sserial_props *)malloc(sizeof(struct sserial_props));
	if (pProps == NULL) {
		fprintf(stderr, "[%s]: Allocating pProps failed with error \"%s\"\n",
				__func__, strerror(errno));
		return NULL;
	}

	//pProps->mtx = PTHREAD_MUTEX_INITIALIZER;

	fprintf(stderr, "Port name: %s\n", portName);
	if ((pProps->fd = open(portName, O_RDWR)) == -1) {
		fprintf(stderr, "[%s]: open() failed with error \"%s\"\n",
				__func__, strerror(errno));
		goto error_open;
	}

	if (tcgetattr(pProps->fd, &pProps->otinfo) == -1) {
		fprintf(stderr, "[%s]: tcgetattr() failed with error \"%s\"\n",
				__func__, strerror(errno));
		goto error_init;
	}

	attr = pProps->otinfo;
	attr.c_cflag |= /*CRTSCTS |*/ CLOCAL;
	attr.c_oflag = 0;
	if (tcflush(pProps->fd, TCIOFLUSH) == -1) {
		fprintf(stderr, "[%s]: tcflush() failed with error \"%s\"\n",
				__func__, strerror(errno));
		goto error_init;
	}

	if (tcsetattr(pProps->fd, TCSANOW, &attr) == -1) {
		fprintf(stderr, "[%s]: tcsetattr() failed with error \"%s\"\n",
				__func__, strerror(errno));
		goto error_init;
	}

	return pProps;

error_init:
	if (close(pProps->fd)) {
		fprintf(stderr, "[%s]: Closing port file descriptor (%d) failed with error \"%s\"\n",
				__func__, pProps->fd, strerror(errno));
	}

error_open:
	free(pProps);

	return NULL;
}

void ClosePort(struct sserial_props *pProps) {
	if (pProps == NULL) {
		fprintf(stderr, "[%s]: pProps is NULL\n", __func__);
		return;
	}

	if (pProps->fd < 0) {
		fprintf(stderr, "[%s]: Wrong file descriptor (%d)\n",
				__func__, pProps->fd);
		return;
	}

	tcsetattr(pProps->fd, TCSANOW, &pProps->otinfo);
	if (close(pProps->fd)) {
		fprintf(stderr, "[%s]: Closing port file descriptor (%d) failed with error \"%s\"\n",
				__func__, pProps->fd, strerror(errno));
	}

	free(pProps);
}

int UpdateRts(struct sserial_props *pProps, int bSet) {
	int status;

	pthread_mutex_lock(&pProps->mtx);

	if (ioctl(pProps->fd, TIOCMGET, &status) == -1) {
		fprintf(stderr, "[%s]: TIOCMGET failed with error \"%s\"\n",
				__func__, strerror(errno));
		pthread_mutex_unlock(&pProps->mtx);
		return 0;
	}

	if (bSet)
		status |= TIOCM_RTS;
	else
		status &= ~TIOCM_RTS;

	if (ioctl(pProps->fd, TIOCMSET, &status) == -1) {
		fprintf(stderr, "[%s]: TIOCMSET failed with error \"%s\"\n",
				__func__, strerror(errno));
		pthread_mutex_unlock(&pProps->mtx);
		return 0;
	}

	pthread_mutex_unlock(&pProps->mtx);
	return 1;
}

int UpdateDtr(struct sserial_props *pProps, int bSet) {
	int status;

	pthread_mutex_lock(&pProps->mtx);

	if (ioctl(pProps->fd, TIOCMGET, &status) == -1) {
		fprintf(stderr, "[%s]: TIOCMGET failed with error \"%s\"\n",
				__func__, strerror(errno));
		pthread_mutex_unlock(&pProps->mtx);
		return 0;
	}

	if (bSet)
		status |= TIOCM_DTR;
	else
		status &= ~TIOCM_DTR;

	if (ioctl(pProps->fd, TIOCMSET, &status) == -1) {
		fprintf(stderr, "[%s]: TIOCMSET failed with error \"%s\"\n",
				__func__, strerror(errno));
		pthread_mutex_unlock(&pProps->mtx);
		return 0;
	}

	pthread_mutex_unlock(&pProps->mtx);
	return 1;
}
