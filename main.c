#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h> // Just for sleep funciton
#include "sserial.h"

#define TAG "Main"
#include "log.h"

#define OK_BUFF_LENGTH (5)

#if 0
void SleepMs(unsigned nDelayMs) {
	struct timespec trest;
	struct timespec tneed;
	tneed.tv_sec = 0;
	tneed.tv_nsec = nDelayMs * 1000000;
	nanosleep(&tneed, &trest);
}
#else
#define SleepMs(nDelayMs) usleep(nDelayMs*1000)
#endif

/**
 * Pinout:
 *  nReset    -> nRTS (out)
 *  AT/nData  -> nDTR (out)
 *  Connected -> nCTS (in)
 */

void resetBluetooth(struct sserial_props *pProps) {
	SetRts(pProps);
	SleepMs(100);
	ClrRts(pProps);
	SleepMs(10);
}

ssize_t readLine(struct sserial_props *pProps, char *pBuff, ssize_t nLength) {
	ssize_t nReadTotal = 0;

	if (nLength < 2) {
		LOGE("[%s] Buffer too short (%ld)", __func__, nLength);
		return -1;
	}

	// Reset first two symbols to avoid problems on the first pass of "while" loop
	memset(pBuff, 0, 2);

	while (nReadTotal < 2 || strncmp(pBuff + (nReadTotal - 2), "\x0a\x0a", 2) != 0) {
		if (nReadTotal == nLength) {
			LOGE("[%s] Buffer too short for this line(%ld) - CRLF not find", __func__, nLength);
			return -1;
		}

		ssize_t nRead = ReadPort(pProps, (void*)(pBuff + nReadTotal), 1);
		if (nRead == -1) {
			LOGE("[%s] Reading from port failed (nReadTotal = %ld)", __func__, nReadTotal);
			return -1;
		}

#ifdef DEBUG
		static int b = 0;
		if (b != 0 && nRead == 0) {
			LOGW(">> Nothing to read");
			b = 1;
		}

		LOGV(">> 0x%.2X", pBuff[nReadTotal]);
#else
		if (nRead == 0) {
			LOGW(">> Nothing to read");
			return -1;
		}
#endif

		nReadTotal += nRead;
	}

#ifdef DEBUG
	LOGV(">> Done = %s", pBuff);
#endif

	return nReadTotal;
}

int initBluetooth(struct sserial_props *pProps) {
	char pBuff[OK_BUFF_LENGTH] = {0};

	LOGI("> Triger reset pin");
	resetBluetooth(pProps);

	LOGI("> Switch device to AT-mode");
	ClrDtr(pProps);
	SleepMs(1000);

#if 0
	LOGV("> Reset to default");
	WritePort(pProps, "AT+ORGL" CRLF, strlen("AT+ORGL" CRLF));
	SleepMs(1000);
#endif

	LOGV("> Switch to Slave mode");
	WritePort(pProps, "AT+ROLE=0" CRLF, strlen("AT+ROLE=0" CRLF));
	if (readLine(pProps, pBuff, OK_BUFF_LENGTH) <= 0 && strncasecmp(pBuff, "ok", 2)) {
		LOGE("AT+ROLE failed to set (%s)", pBuff);
		return 0;
	}

	LOGV("> Set name");
	WritePort(pProps, "AT+NAME=ww-test" CRLF, strlen("AT+NAME=qq-test" CRLF));
	if (readLine(pProps, pBuff, OK_BUFF_LENGTH) <= 0 && strncasecmp(pBuff, "ok", 2)) {
		LOGE("AT+NAME failed to set (%s)", pBuff);
		return 0;
	}

	LOGV("> Set password");
	WritePort(pProps, "AT+PSWD=1237" CRLF, strlen("AT+PSWD=1235" CRLF));
	if (readLine(pProps, pBuff, OK_BUFF_LENGTH) <= 0 && strncasecmp(pBuff, "ok", 2)) {
		LOGE("AT+PSWD failed to set (%s)", pBuff);
		return 0;
	}

	LOGI("> Switch device to Data-mode");
	SetDtr(pProps);
	SleepMs(10);

	resetBluetooth(pProps);

	return 1;
}

int main(void) {
	struct sserial_props *pProps = OpenPort("/dev/ttyUSB0", 38400);
	if (pProps == NULL) {
		LOGE("Port init failed");
		return 1;
	}

	if (!initBluetooth(pProps)) {
		LOGE("[%s] Bluetooth init failed", __func__);
		goto exit;
	}

	char ch='\0';
	// print all until "a" will be be recieved
	while (ch!='a') {
		int bDisconnected = GetCts(pProps);
		// It will be true until someone connected
		if (bDisconnected != 0) {
			continue;
		}

		// skip empty and line endings
		if (ReadPort(pProps, &ch, 1) <= 0 ||
				ch == '\r' || ch == '\n')
			continue;
		LOGV(">> %c", ch);
	}

exit:
	ClosePort(pProps);
	return 0;
}
