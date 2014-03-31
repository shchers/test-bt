/**
 * Simple serial port module
 * (c) Sergey Shcherbakov <shchers@gmail.com>
 */

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/serial.h>
#include "sserial.h"

#ifndef CRTSCTS
#define CRTSCTS  020000000000		/* flow control */
#endif

#if 0
static int SetBaudrate(struct sserial_props *pProps, int nBaudrate) {
	struct serial_struct ser_info;

	if (ioctl(pProps->fd, TIOCGSERIAL, &ser_info) == -1) {
		fprintf(stderr, "[%s]: TIOCGSERIAL failed with error \"%s\"\n",
				__func__, strerror(errno));
		return 0;
	}

	ser_info.flags = ASYNC_SPD_CUST | ASYNC_LOW_LATENCY;
	ser_info.custom_divisor = ser_info.baud_base / nBaudrate;

	if (ioctl(pProps->fd, TIOCSSERIAL, &ser_info) == -1) {
		fprintf(stderr, "[%s]: TIOCGSERIAL failed with error \"%s\"\n",
				__func__, strerror(errno));
		return 0;
	}

	return 1;
}
#endif

struct sserial_props *OpenPort(const char *portName, int nBaudrate) {
	struct sserial_props *pProps;
	struct termios attr;

	pProps = (struct sserial_props *)malloc(sizeof(struct sserial_props));
	if (pProps == NULL) {
		fprintf(stderr, "[%s]: Allocating pProps failed with error \"%s\"\n",
				__func__, strerror(errno));
		return NULL;
	}

	pthread_mutex_init(&pProps->mtx_ctrl, NULL);
	pthread_mutex_init(&pProps->mtx_rx, NULL);
	pthread_mutex_init(&pProps->mtx_tx, NULL);

	fprintf(stderr, "Port name: %s Baudrate: %d\n", portName, nBaudrate);
	if ((pProps->fd = open(portName, O_RDWR | O_NOCTTY | O_NDELAY)) == -1) {
		fprintf(stderr, "[%s]: open() failed with error \"%s\"\n",
				__func__, strerror(errno));
		goto error_open;
	}

	fcntl(pProps->fd, F_SETFL, /*FNDELAY*/0);

	if (tcgetattr(pProps->fd, &pProps->otinfo) == -1) {
		fprintf(stderr, "[%s]: tcgetattr() failed with error \"%s\"\n",
				__func__, strerror(errno));
		goto error_init;
	}

	attr = pProps->otinfo;

	// Set baudrate
	cfsetospeed (&attr, (speed_t)B38400);
	cfsetispeed (&attr, (speed_t)B38400);

	attr.c_cflag &= ~PARENB;
	attr.c_cflag &= ~CSTOPB;
	attr.c_cflag &= ~CSIZE;
	attr.c_cflag &= ~CRTSCTS;
	attr.c_cflag |= CS8 | CREAD | CLOCAL;
	attr.c_oflag &= ~OPOST;
	attr.c_iflag &= ~(IXON | IXOFF | IXANY);
	attr.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	attr.c_cc[VMIN] = 0;
	attr.c_cc[VTIME] = 150;
	if (tcflush(pProps->fd, TCIOFLUSH) == -1) {
		fprintf(stderr, "[%s]: tcflush() failed with error \"%s\"\n",
				__func__, strerror(errno));
		goto error_init;
	}

	//cfmakeraw(&attr);
	if (tcsetattr(pProps->fd, TCSANOW, &attr) == -1) {
		fprintf(stderr, "[%s]: tcsetattr() failed with error \"%s\"\n",
				__func__, strerror(errno));
		goto error_init;
	}

#if 0
	if (!SetBaudrate(pProps, nBaudrate)) {
		fprintf(stderr, "[%s]: Configuring baudrate failed\n", __func__);
		goto error_init;
	}
#endif

	// Flush port I/O queue
	tcflush(pProps->fd,TCIOFLUSH);

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

	pthread_mutex_lock(&pProps->mtx_ctrl);

	if (ioctl(pProps->fd, TIOCMGET, &status) == -1) {
		fprintf(stderr, "[%s]: TIOCMGET failed with error \"%s\"\n",
				__func__, strerror(errno));
		pthread_mutex_unlock(&pProps->mtx_ctrl);
		return 0;
	}

	if (bSet)
		status |= TIOCM_RTS;
	else
		status &= ~TIOCM_RTS;

	if (ioctl(pProps->fd, TIOCMSET, &status) == -1) {
		fprintf(stderr, "[%s]: TIOCMSET failed with error \"%s\"\n",
				__func__, strerror(errno));
		pthread_mutex_unlock(&pProps->mtx_ctrl);
		return 0;
	}

	pthread_mutex_unlock(&pProps->mtx_ctrl);
	return 1;
}

int UpdateDtr(struct sserial_props *pProps, int bSet) {
	int status;

	pthread_mutex_lock(&pProps->mtx_ctrl);

	if (ioctl(pProps->fd, TIOCMGET, &status) == -1) {
		fprintf(stderr, "[%s]: TIOCMGET failed with error \"%s\"\n",
				__func__, strerror(errno));
		pthread_mutex_unlock(&pProps->mtx_ctrl);
		return 0;
	}

	if (bSet)
		status |= TIOCM_DTR;
	else
		status &= ~TIOCM_DTR;

	if (ioctl(pProps->fd, TIOCMSET, &status) == -1) {
		fprintf(stderr, "[%s]: TIOCMSET failed with error \"%s\"\n",
				__func__, strerror(errno));
		pthread_mutex_unlock(&pProps->mtx_ctrl);
		return 0;
	}

	pthread_mutex_unlock(&pProps->mtx_ctrl);
	return 1;
}

int GetCts(struct sserial_props *pProps) {
	int status;

	pthread_mutex_lock(&pProps->mtx_ctrl);

	if (ioctl(pProps->fd, TIOCMGET, &status) == -1) {
		fprintf(stderr, "[%s]: TIOCMGET failed with error \"%s\"\n",
				__func__, strerror(errno));
		pthread_mutex_unlock(&pProps->mtx_ctrl);
		return -1;
	}

	status &= TIOCM_CTS;

	pthread_mutex_unlock(&pProps->mtx_ctrl);
	return status > 0;
}

ssize_t ReadPort(struct sserial_props *pProps, void *pBuff, size_t nSize) {
	if (pthread_mutex_trylock(&pProps->mtx_rx) == EBUSY) {
		fprintf(stderr, "[%s]: Another read process ongoing\n", __func__);
		return -1;
	}

	ssize_t nRead = read(pProps->fd, pBuff, nSize);
#if DEBUG
	if (nRead == -1) {
		fprintf(stderr, "[%s]: Detected error on read %d = \"%s\"\n", __func__,
				errno, strerror(errno));
	}
#endif
	pthread_mutex_unlock(&pProps->mtx_rx);
	return nRead;
}

ssize_t WritePort(struct sserial_props *pProps, void *pBuff, size_t nSize) {
	if (pthread_mutex_trylock(&pProps->mtx_tx) == EBUSY) {
		fprintf(stderr, "[%s]: Another read process ongoing\n", __func__);
		return -1;
	}

	ssize_t nWritten = write(pProps->fd, pBuff, nSize);
#if DEBUG
	if (nWritten == -1) {
		fprintf(stderr, "[%s]: Detected error on write %d = \"%s\"\n", __func__,
				errno, strerror(errno));
	}
#endif
	pthread_mutex_unlock(&pProps->mtx_tx);
	return nWritten;
}
