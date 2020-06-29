#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#include "IT8951.h"
#include "measure.h"

struct Opts {
	const char *filename;
	uint32_t sleep_time;
	uint8_t bpp_mode;
};

volatile int redrawing = 0;

static IT8951LdImgInfo stLdImgInfo = {0};
static IT8951AreaImgInfo stAreaImgInfo = {0};
static Device device = {0};
static BmpBuffer bmp = {0};
static Opts opts = {0};

void cleanup(int sig) {
	printf("%s: Exiting on signal %d. Thread is %lu\n",
		__FUNCTION__, sig, pthread_self());
	BmpBuffer_cleanup(&bmp);
	IT8951_Cancel(&device);
	exit(0);
}

static int prepare() {
	EPD_Clear(&device, 0xFF);

	//Setting Load image information
	stLdImgInfo.ulStartFBAddr    = (uint32_t)device.gpFrameBuf;
	if (opts.bpp_mode == 8) {
		stLdImgInfo.usEndianType     = IT8951_LDIMG_L_ENDIAN;
		stLdImgInfo.usPixelFormat    = IT8951_8BPP;
	} else if (opts.bpp_mode == 4) {
		stLdImgInfo.usEndianType     = IT8951_LDIMG_B_ENDIAN;
		stLdImgInfo.usPixelFormat    = IT8951_4BPP;
	} else {
		printf("Invalid mode\n");
		return 1;
	}
	stLdImgInfo.usRotate         = IT8951_ROTATE_0;
	stLdImgInfo.ulImgBufBaseAddr = device.gulImgBufAddr;
	//Set Load Area
	stAreaImgInfo.usX      = 0;
	stAreaImgInfo.usY      = 0;
	stAreaImgInfo.usWidth  = Device_width(&device);
	stAreaImgInfo.usHeight = Device_height(&device);
	return 0;
}

static void redraw() {
	if (redrawing) {
		return;
	}
	meas_start();
	redrawing = 1;

	EPD_Clear(&device, 0xFF);
	printf("Show %s\n", opts.filename);
	Show_bmp(&device, &bmp, opts.filename);

	IT8951WaitForDisplayReady();

	//Load Image from Host to IT8951 Image Buffer
	IT8951HostAreaPackedPixelWrite(&stLdImgInfo, &stAreaImgInfo, opts.bpp_mode);//Display function 2
	//Display Area ?V (x,y,w,h) with mode 2 for fast gray clear mode - depends on current waveform
	IT8951DisplayArea(0, 0, Device_width(&device), Device_height(&device), 2);

	meas_end();
	// remove to indicate file is unlocked
	printf("Deleting %s\n", opts.filename);
	remove(opts.filename);
	redrawing = 0;
}

void signaled(int sig) { // can be called asynchronously
	printf("%s: Signal received %d, thread is %lu\n", __FUNCTION__, sig, pthread_self());
	redraw();
}

static void _show_help(char **argv) {
	printf("Usage:\n\
%s\n\
 -i <path to BMP file>\n\
 [-r <autorefresh every N seconds>]\n\
 [-m <mode={4,8} bpp>]\n\
", argv[0]);
}

static int Opts_parse(Opts *opts, int argc, char **argv) {
	if (argc < 3) {
		_show_help(argv);
		return 0;
	}

	opts->filename = NULL;
	opts->sleep_time = UINT_MAX;
	opts->bpp_mode = 8;

	int opt, prevopt=' ';

    // put ':' in the starting of the
    // string so that program can
    //distinguish between '?' and ':'
    while((opt = getopt(argc, argv, ":i:r:m:")) != -1) {
        switch(opt) {
            case 'i':
                printf("Image: '%s'\n", optarg);
                opts->filename = optarg;
                break;
			case 'm':
				if (!strcmp("4", optarg)) {
					opts->bpp_mode = 4;
				} else if (!strcmp("8", optarg)) {
					opts->bpp_mode = 8;
				} else {
					_show_help(argv);
					printf("\nInvalid bpp mode '%s'\n", optarg);
					return -1;
				}
                break;
			case 'r':
				if (sscanf(optarg, "%u", &opts->sleep_time) != 1) {
					_show_help(argv);
					printf("\nInvalid sleep time '%s'\n", optarg);
					return -1;
				}
                break;
            case ':':
				_show_help(argv);
                printf("\nOption '%c' needs a value!\n", prevopt);
                return -1;
            case '?':
				_show_help(argv);
                printf("\nUnknown option: '%c'\n", optopt);
                return -1;
        }
        prevopt = opt;
    }
    printf("Started with options:\n\
filename:   '%s'\n\
sleep_time:  %u s\n\
bpp_mode:    %u\n",
	opts->filename, opts->sleep_time, opts->bpp_mode);
    
    return 0;
}

int main(int argc, char **argv) {
	
	if (Opts_parse(&opts, argc, argv) != 0) {
		return 1;
	}

	if (IT8951_Init(&device)) {
		printf("IT8951_Init error\n");
		return 1;
	}

	if (prepare() != 0) {
		printf("Failed to prepare\n");
		cleanup(0);
		return 1;
	}
	
	printf("%s: Thread is %lu\n", __FUNCTION__, pthread_self());
	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);
	signal(SIGUSR1, signaled);

	printf("%s: Waiting for SIGINT|SIGUSR1 or refresh with interval %u seconds\n", __FUNCTION__, opts.sleep_time);
	printf("*** PID is:\n%d\n", getpid());
	while (1) {
		uint32_t retv = sleep(opts.sleep_time);
		uint32_t slept = opts.sleep_time - retv;
		printf("Slept %u seconds\n", slept);
		if (opts.sleep_time != UINT_MAX && slept == opts.sleep_time) {
			redraw();
		}
	}

	return 0;
}
