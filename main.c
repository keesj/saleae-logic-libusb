// vim: sw=8:ts=8:noexpandtab
#include "slogic.h"
#include "usbutil.h"

#include <assert.h>
#include <libusb.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Command line arguments */

struct slogic_sample_rate *sample_rate = NULL;
const char *output_file_name = NULL;
size_t n_samples = 0;
size_t transfer_buffer_size = 0;
size_t n_transfer_buffers = 0;
size_t libusb_debug_level = 0;
unsigned int transfer_timeout = 0;

/* */
const char *me = "main";

void short_usage(const char *message, ...)
{
	char p[1024];
	va_list ap;

	va_start(ap, message);
	(void)vsnprintf(p, 1024, message, ap);
	va_end(ap);
	fprintf(stderr, "Error: %s\n", p);
}

void full_usage()
{
	int i;
	struct slogic_sample_rate *sample_rates;
	size_t n_sample_rates;

	slogic_available_sample_rates(&sample_rates, &n_sample_rates);

	fprintf(stderr, "usage: %s -f <output file> -r <sample rate> [-n <number of samples>]\n", me);
	fprintf(stderr, "\n");
	fprintf(stderr, " -n: Number of samples to record\n");
	fprintf(stderr, "     Defaults to one second of samples for the specified sample rate\n");
	fprintf(stderr, " -f: The output file. Using '-' means that the bytes will be output to stdout.\n");
	fprintf(stderr, " -r: Select sample rate for the Logic.\n");
	fprintf(stderr, "     Available sample rates:\n");
	fprintf(stderr, " -h: This help message.\n");
	for (i = 0; i < n_sample_rates; i++, sample_rates++) {
		fprintf(stderr, "      o %s\n", sample_rates->text);
	}
	fprintf(stderr, "\n");
	fprintf(stderr, "Advanced options:\n");
	fprintf(stderr, " -b: Transfer buffer size.\n");
	fprintf(stderr, " -t: Number of transfer buffers.\n");
	fprintf(stderr, " -o: Transfer timeout.\n");
	fprintf(stderr, " -u: libusb debug level: 0 to 3, 3 is most verbose. Defaults to '0'.\n");
	fprintf(stderr, "\n");
}

/* Returns true if everything was OK */
bool parse_args(int argc, char **argv)
{
	char c;
	char *endptr;
	/* TODO: Add a -d flag to turn on internal debugging */
	while ((c = getopt(argc, argv, "n:f:r:hb:t:o:u:")) != -1) {
		switch (c) {
		case 'n':
			n_samples = strtol(optarg, &endptr, 10);
			if (*endptr != '\0' || n_samples <= 0) {
				short_usage("Invalid number of samples, must be a positive integer: %s", optarg);
				return false;
			}
			break;
		case 'f':
			output_file_name = optarg;
			break;
		case 'r':
			sample_rate = slogic_parse_sample_rate(optarg);
			if (!sample_rate) {
				short_usage("Invalid sample rate: %s", optarg);
				return false;
			}
			break;
		case 'h':
			full_usage();
			return false;
		case 'b':
			transfer_buffer_size = strtol(optarg, &endptr, 10);
			if (*endptr != '\0' || transfer_buffer_size <= 0) {
				short_usage("Invalid transfer buffer size, must be a positive integer: %s", optarg);
				return false;
			}
			break;
		case 't':
			n_transfer_buffers = strtol(optarg, &endptr, 10);
			if (*endptr != '\0' || n_transfer_buffers <= 0) {
				short_usage("Invalid transfer buffer count, must be a positive integer: %s", optarg);
				return false;
			}
			break;
		case 'o':
			transfer_timeout = strtol(optarg, &endptr, 10);
			if (*endptr != '\0' || transfer_timeout <= 0) {
				short_usage("Invalid transfer timeout, must be a positive integer: %s", optarg);
				return false;
			}
			break;
		case 'u':
			libusb_debug_level = strtol(optarg, &endptr, 10);
			if (*endptr != '\0' || libusb_debug_level < 0 || libusb_debug_level > 3) {
				short_usage("Invalid libusb debug level, must be a positive integer between "
						"0 and 3: %s", optarg);
				return false;
			}
			break;
		default:
		case '?':
			short_usage("Unknown argument: %c. Use %s -h for usage.", optopt, me);
			return false;
		}
	}

	if (!output_file_name) {
		short_usage("An output file has to be specified.", optarg);
		return false;
	}

	if (!sample_rate) {
		short_usage("A sample rate has to be specified.", optarg);
		return false;
	}

	if (!n_samples) {
		n_samples = sample_rate->samples_per_second;
	}

	return true;
}

int count = 0;
int sum = 0;
bool on_data_callback(uint8_t *data, size_t size, void *user_data)
{
	bool more = sum < 24 * 1024 * 1024;
	fprintf(stderr, "Got sample: size: %zu, #samples: %d, aggregate size: %d, more: %d\n", size, count, sum, more);
	count++;
	sum += size;
	return more;
}

int main(int argc, char **argv)
{
	struct slogic_recording recording;

	if (!parse_args(argc, argv)) {
		exit(EXIT_FAILURE);
	}

/* KEJO: the slogic_open should not perform the firmware uploading
the hanlding should still be here */
/* Trygve: Why? It would be a pain for every user to have to handle
the uploading of the firmware.  To be able to use the rest of the
API (which basically are the sampling methods) you would need a
(compatible) firmware to be uploaded before usage. */

/* does firmware upload work for you? I guess not. to upload the
firmware we need to connmect to an other VENDOR/PRODUCT. it really
deserves special hanling also because we currently fail to first
upload and continue */
	struct slogic_handle *handle = slogic_open();
	assert(handle);

	slogic_tune(handle, stderr, transfer_buffer_size, n_transfer_buffers, transfer_timeout, libusb_debug_level);

	slogic_fill_recording(&recording, sample_rate, on_data_callback, NULL);
	recording.debug_file = stderr;

	slogic_execute_recording(handle, &recording);

	slogic_close(handle);

	exit(EXIT_SUCCESS);
}
