// vim: sw=8:ts=8:noexpandtab
#include <assert.h>
#include <libusb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "slogic.h"
#include "usbutil.h"

/* Command line arguments */
struct slogic_sample_rate *sample_rate = NULL;
const char *output_file_name = NULL;
size_t n_samples = 0;
size_t transfer_buffer_size = 0;
int n_transfer_buffers = 0;
int libusb_debug_level = 0;

const char *me = "main";

int usage(const char *message, ...)
{
	int i;
	struct slogic_sample_rate *sample_rates;
	size_t n_sample_rates;

	slogic_available_sample_rates(&sample_rates, &n_sample_rates);

	fprintf(stderr, "usage: %s -f <output file> -r <sample rate> " +
		"[-n <number of samples>]\n", me);
	fprintf(stderr, "\n");
	fprintf(stderr, " -n: Number of samples to record\n");
	fprintf(stderr, "     Defaults to one second of samples for the " +
		"specified sample rate\n");
	fprintf(stderr, " -f: The output file. Using '-' means that the " +
		"bytes will be output to stdout.\n");
	fprintf(stderr, " -r: Select sample rate for the Logic.\n");
	fprintf(stderr, "     Available sample rates:\n");
	for (i = 0; i < n_sample_rates; i++, sample_rates++) {
		fprintf(stderr, "      o %s\n", sample_rates->text);
	}
	fprintf(stderr, "\n");
	fprintf(stderr, "Advanced options:\n");
	fprintf(stderr, " -b: Transfer buffer size.\n");
	fprintf(stderr, " -t: Number of transfer buffers.\n");
	fprintf(stderr, " -u: libusb debug level: 0 to 3, 3 is most verbose. " +
		"Defaults to '0'.\n");
	fprintf(stderr, "\n");
	if (message) {
		char p[1024];
		va_list ap;

		va_start(ap, message);
		(void)vsnprintf(p, 1024, message, ap);
		va_end(ap);
		fprintf(stderr, "Error: %s\n", p);
	}

	return 0;
}

int parse_args(int argc, char **argv)
{
	const char *msg;	/* The indentation gets insane */
	char c;
	while ((c = getopt(argc, argv, "r:n:f:b:t:u:")) != -1) {
		switch (c) {
		case 'r':
			sample_rate = slogic_parse_sample_rate(optarg);
			if (!sample_rate) {
				return usage("Invalid sample rate: %s", optarg);
			}
			break;
		case 'f':
			output_file_name = optarg;
			break;
		case 'n':
			n_samples = strtol(optarg, NULL, 10);
			if (n_samples <= 0) {
				return usage("Invalid number of samples, " +
					     "must be a positive integer: %s",
					     optarg);
			}
			break;
		case 'b':
			transfer_buffer_size = strtol(optarg, NULL, 10);
			if (transfer_buffer_size <= 0) {
				return usage("Invalid transfer buffer size, " +
					     "must be a positive integer: %s",
					     optarg);
			}
			break;
		case 't':
			n_transfer_buffers = strtol(optarg, NULL, 10);
			if (n_transfer_buffers <= 0) {
				return usage("Invalid transfer buffer size, " +
					     "must be a positive integer: %s",
					     optarg);
			}
			break;
		case 'u':
			libusb_debug_level = strtol(optarg, NULL, 10);
			if (libusb_debug_level < 0 || libusb_debug_level > 3) {
				return usage("Invalid libusb debug level, " +
					     "must be a positive integer between 0 and 3: %s",
					     optarg);
			}
			break;
		default:
		case '?':
			return usage("Unknown argument: %c", optopt);
		}
	}

	if (!output_file_name) {
		return usage("An output file has to be specified.", optarg);
	}

	if (!sample_rate) {
		return usage("A sample rate has to be specified.", optarg);
	}

	if (!n_samples) {
		n_samples = sample_rate->samples_per_second;
	}

	return 1;
}

int main(int argc, char **argv)
{
	if (!parse_args(argc, argv)) {
		return EXIT_FAILURE;
	}

	struct slogic_handle *handle = slogic_open();
	assert(handle);

	slogic_tune(handle, transfer_buffer_size, n_transfer_buffers,
		    libusb_debug_level);

#if 0
	/* apparently one need to at least read once before the driver continues */
	printf("Reading byte\n");
	unsigned char b;
	int ret = slogic_readbyte(handle, &b);
	assert(ret == 0);
	printf("ret = %d, byte = 0x02%x\n", ret, b);
#endif
	uint8_t *buffer = malloc(n_samples);
	assert(buffer);

	printf("slogic_read_samples\n");
	slogic_read_samples(handle, sample_rate, buffer, n_samples);

	slogic_close(handle);

	return 0;
}
