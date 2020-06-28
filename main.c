#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#include "IT8951.h"
#include "measure.h"

static const char *img_path = NULL;
volatile int redrawing = 0;

static IT8951LdImgInfo stLdImgInfo = {0};
static IT8951AreaImgInfo stAreaImgInfo = {0};
static Device device = {0};
static BmpBuffer bmp = {0};
static uint8_t *tmp = NULL;

void cleanup(int sig) {
	printf("%s: Exiting on signal %d. Thread is %lu\n", 
		__FUNCTION__, sig, pthread_self());
	BmpBuffer_cleanup(&bmp);
	IT8951_Cancel(&device);
	free(tmp);
	exit(0);
}

static void prepare() {
	EPD_Clear(&device, 0xFF);

	//Setting Load image information
	stLdImgInfo.ulStartFBAddr    = (uint32_t)device.gpFrameBuf;
	stLdImgInfo.usEndianType     = IT8951_LDIMG_L_ENDIAN;
	stLdImgInfo.usPixelFormat    = IT8951_8BPP;
	stLdImgInfo.usRotate         = IT8951_ROTATE_0;
	stLdImgInfo.ulImgBufBaseAddr = device.gulImgBufAddr;
	//Set Load Area
	stAreaImgInfo.usX      = 0;
	stAreaImgInfo.usY      = 0;
	stAreaImgInfo.usWidth  = Device_width(&device);
	stAreaImgInfo.usHeight = Device_height(&device);
}

static void redraw() {
	if (redrawing) {
		return;
	}
	meas_start();
	redrawing = 1;
	
	EPD_Clear(&device, 0xFF);
	printf("Show %s\n", img_path);
	Show_bmp(&device, &bmp, img_path);

	IT8951WaitForDisplayReady();

	//Load Image from Host to IT8951 Image Buffer
	IT8951HostAreaPackedPixelWrite(&stLdImgInfo, &stAreaImgInfo, tmp);//Display function 2
	//Display Area ?V (x,y,w,h) with mode 2 for fast gray clear mode - depends on current waveform
	IT8951DisplayArea(0, 0, Device_width(&device), Device_height(&device), 2);
	
	meas_end();
	// remove to indicate file is unlocked
	printf("Deleting %s\n", img_path);
	remove(img_path);
	redrawing = 0;
}

void signaled(int sig) { // can be called asynchronously
	printf("%s: Signal received %d, thread is %lu\n", __FUNCTION__, sig, pthread_self());
	redraw();
}

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("Usage:\n\
%s\n\
 <path to BMP file>\n\
 [<autorefresh every N seconds>]\n\
", argv[0]);
		return 0;
	}

	uint32_t sleep_time = UINT_MAX;
	img_path = argv[1];


	if (argc >= 3) {
		sscanf(argv[2], "%u", &sleep_time);
	}

	printf("Started for image path: '%s'\n", img_path);
	printf("Sleep time: %u seconds\n", sleep_time);

	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);

	if (IT8951_Init(&device)) {
		printf("IT8951_Init error \n");
		return 1;
	}

	printf("%s: Thread is %lu\n", __FUNCTION__, pthread_self());
	signal(SIGUSR1, signaled);

	prepare();

	printf("%s: Waiting for SIGINT|SIGUSR1 or refresh with interval %u seconds\n", __FUNCTION__, sleep_time);
	printf("*** PID is:\n%d\n", getpid());
	while (1) {
		uint32_t retv = sleep(sleep_time);
		uint32_t slept = sleep_time - retv;
		printf("Slept %u seconds\n", slept);
		if (sleep_time != UINT_MAX && slept == sleep_time) {
			redraw();
		}
	}

	return 0;
}
