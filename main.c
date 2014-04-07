#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h> // Just for sleep funciton
#include <unistd.h> // needed by getopt()
#include <getopt.h>
#include <stdlib.h> // Used by atoi()
#include "sserial.h"

#define TAG "Main"
#include "log.h"

#define OK_BUFF_LENGTH (5)
#define RX_BUFF_LENGTH (32)

#define DEFAULT_PORT_NAME   "/dev/ttyUSB0"
#define DEFAULT_BAUDRATE    (38400);
#define DEFAULT_BT_NAME     "Test-BT"
#define DEFAULT_BT_PIN      "0000"
// XXX: Max pin code length hardcoded to 4 (four) digits, intead of 16
#define DEFAULT_BT_PIN_MAX  (4)

#define OPT_PORT_NAME "port"
#define OPT_BAUDRATE  "baud"
#define OPT_BT_NAME   "name"
#define OPT_BT_PIN    "pin"
#define OPT_HELP      "help"

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

int initBluetooth(struct sserial_props *pProps, const char *btName, const char *btPin) {
	char pTxBuff[32];
	char pRxBuff[RX_BUFF_LENGTH] = {0};
	int bSame, nLength;

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
	if (readLine(pProps, pRxBuff, OK_BUFF_LENGTH) <= 0 &&
			strncasecmp(pRxBuff, "ok", 2)) {
		LOGE("AT+ROLE failed to set (%s)", pRxBuff);
		return 0;
	}

	WritePort(pProps, "AT+NAME?" CRLF, strlen("AT+NAME?" CRLF));
	nLength = readLine(pProps, pRxBuff, RX_BUFF_LENGTH);
	if (nLength) {
		pRxBuff[nLength - 2] = '\0';
		LOGE("Current name: \"%s\"", strchr(pRxBuff,':') + 1);
	}

	bSame = strncmp(strchr(pRxBuff,':') + 1, btName, strlen(btName)) == 0;

	// Get "OK"
	readLine(pProps, pRxBuff, OK_BUFF_LENGTH);

	if (bSame) {
		LOGW("Bluetooth name is same and will not be changed");
		goto skip_name;
	}

	LOGV("> Set name");
	nLength = sprintf(pTxBuff, "AT+NAME=%s" CRLF, btName);
	WritePort(pProps, pTxBuff, nLength);
	if (readLine(pProps, pRxBuff, OK_BUFF_LENGTH) <= 0 &&
			strncasecmp(pRxBuff, "ok", 2)) {
		LOGE("AT+NAME failed to set (%s)", pRxBuff);
		return 0;
	}

skip_name:
	WritePort(pProps, "AT+PSWD?" CRLF, strlen("AT+PSWD?" CRLF));
	nLength = readLine(pProps, pRxBuff, RX_BUFF_LENGTH);
	if (nLength) {
		pRxBuff[nLength - 2] = '\0';
		LOGE("Current pin code: \"%s\"", strchr(pRxBuff,':') + 1);
	}

	bSame = strncmp(strchr(pRxBuff,':') + 1, btPin, strlen(btPin)) == 0;

	// Get "OK"
	readLine(pProps, pRxBuff, OK_BUFF_LENGTH);

	if (bSame) {
		LOGW("Bluetooth pin code is same and will not be changed");
		goto skip_password;
	}

	LOGV("> Set password");
	nLength = sprintf(pTxBuff, "AT+PSWD=%s" CRLF, btPin);
	WritePort(pProps, pTxBuff, nLength);
	if (readLine(pProps, pRxBuff, OK_BUFF_LENGTH) <= 0 &&
			strncasecmp(pRxBuff, "ok", 2)) {
		LOGE("AT+PSWD failed to set (%s)", pRxBuff);
		return 0;
	}

skip_password:
	LOGI("> Switch device to Data-mode");
	SetDtr(pProps);
	SleepMs(10);

	resetBluetooth(pProps);

	return 1;
}

void usage(void) {
	fprintf(stderr, "\nSupported options\n\n"
		"\t-p,--" OPT_PORT_NAME "\t\tSet port name\n"
		"\t-b,--" OPT_BAUDRATE "\t\tSet port baudrate\n"
		"\t-n,--" OPT_BT_NAME "\t\tSet Bluetooth device name\n"
		"\t-c,--" OPT_BT_PIN "\t\tSet Bluetooth device access pin code (up to four digits)\n\n"
	);
}

int main(int argc, char *argv[]) {
	// Command line arguments
	const char *optString = "p:b:n:c:h?";
	const struct option longOpts[] = {
		{ OPT_PORT_NAME, required_argument, NULL, 'p' },
		{ OPT_BAUDRATE, required_argument, NULL, 'b' },
		{ OPT_BT_NAME, required_argument, NULL, 'n' },
		{ OPT_BT_PIN, required_argument, NULL, 'c' },
		{ OPT_HELP, no_argument, NULL, 'h' },
		{ NULL, no_argument, NULL, 0 }
	};

	// Port properties
	int nBaudrate = 0;
	const char *portName = NULL;

	// BT properties
	const char *btName = NULL;
	const char *btPin = NULL;

	int opt, longIndex;
	while ((opt = getopt_long( argc, argv, optString, longOpts, &longIndex)) != -1) {
		switch(opt) {
			case 'p':
				portName = optarg;
				break;
			case 'b':
				nBaudrate = atoi(optarg);
				break;
			case 'n':
				btName = optarg;
				break;
			case 'c':
				btPin = optarg;
				break;
			case 'h':
				usage();
				exit(0);
				break;
			// Case for long names
			case 0:
				if (strcmp(OPT_PORT_NAME, longOpts[longIndex].name) == 0) {
					portName = optarg;
				} else if (strcmp(OPT_BAUDRATE, longOpts[longIndex].name) == 0) {
					nBaudrate = atoi(optarg);
				} else if (strcmp(OPT_BT_NAME, longOpts[longIndex].name) == 0) {
					btName = optarg;
				} else if (strcmp(OPT_BT_PIN, longOpts[longIndex].name) == 0) {
					btPin = optarg;
				} else if (strcmp(OPT_HELP, longOpts[longIndex].name) == 0) {
					usage();
					exit(0);
				}
				break;
			default:
				usage();
				exit(0);
				break;
		}
	}

	if (portName == NULL) {
		portName = DEFAULT_PORT_NAME;
		LOGW("Port name not defined and will be used default \"%s\"", portName);
	}

	if (nBaudrate == 0) {
		nBaudrate = DEFAULT_BAUDRATE;
		LOGW("Baudrate not defined and will be used default \"%d\"", nBaudrate);
	}

	if (btName == NULL) {
		btName = DEFAULT_BT_NAME;
		LOGW("Bluetooth name not defined and will be used default \"%s\"", btName);
	}

	if (btPin == NULL || strlen(btPin) > DEFAULT_BT_PIN_MAX) {
		if (btPin == NULL) {
			LOGW("Bluetooth pin code not defined and will be used default \"%s\"", DEFAULT_BT_PIN);
		} else {
			LOGW("Bluetooth pin code too long (%lu), than should be (%d) and will be used default \"%s\"",
					strlen(btPin), DEFAULT_BT_PIN_MAX, btPin);
		}

		btPin = DEFAULT_BT_PIN;
	}

	LOGV("Port name = %s", portName);
	LOGV("Baudrate = %d", nBaudrate);
	LOGV("Bluetooth name = %s", btName);
	LOGV("Bluetooth pin code = %s", btPin);

	struct sserial_props *pProps = OpenPort(portName, nBaudrate);
	if (pProps == NULL) {
		LOGE("Port init failed");
		return 1;
	}

	if (!initBluetooth(pProps, btName, btPin)) {
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
