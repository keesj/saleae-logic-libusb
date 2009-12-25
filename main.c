// vim: sw=8:ts=8:noexpandtab
#include "slogic.h"
#include "usbutil.h"

#include <assert.h>
#include <libusb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Command line arguments */

/* KEJO: I tend to like to put all those global variable in a struct that can be passed around 
this makes it possible to create threaded application as there will be no shared data in between */
/* Trygve: As this is a main() program I don't see the big benefit. It makes the
rest of the program easier to program as they all just manipulate the global
variables.

The interfacing with with the slogic API will be though the slogic_handle and
slogic_recording which should facilitate multi-threaded applications and
logical grouping of parameters. */

/* Ok  - KEJO */
struct slogic_sample_rate *sample_rate = NULL;
const char *output_file_name = NULL;
size_t n_samples = 0;
size_t transfer_buffer_size = 0;
int n_transfer_buffers = 0;
int libusb_debug_level = 0;

const char *me = "main";

/* KEJO: usage is way to long when there is an error. only show usage on some --help option or similar.
the normal output can be something like
usage: main -f <sample_rate> -r bla ....
Error: no file specified
*/
void usage(const char *message, ...)
{
	int i;
	struct slogic_sample_rate *sample_rates;
	size_t n_sample_rates;

	if (message) {
		char p[1024];
		va_list ap;

		va_start(ap, message);
		(void)vsnprintf(p, 1024, message, ap);
		va_end(ap);
		fprintf(stderr, "Error: %s\n", p);
	} else {
		slogic_available_sample_rates(&sample_rates, &n_sample_rates);

		fprintf(stderr, "usage: %s -f <output file> -r <sample rate> [-n <number of samples>]\n", me);
		fprintf(stderr, "\n");
		fprintf(stderr, " -n: Number of samples to record\n");
		fprintf(stderr, "     Defaults to one second of samples for the specified sample rate\n");
		fprintf(stderr, " -f: The output file. Using '-' means that the bytes will be output to stdout.\n");
		fprintf(stderr, " -r: Select sample rate for the Logic.\n");
		fprintf(stderr, "     Available sample rates:\n");
		for (i = 0; i < n_sample_rates; i++, sample_rates++) {
			fprintf(stderr, "      o %s\n", sample_rates->text);
		}
		fprintf(stderr, "\n");
		fprintf(stderr, "Advanced options:\n");
		fprintf(stderr, " -b: Transfer buffer size.\n");
		fprintf(stderr, " -t: Number of transfer buffers.\n");
		fprintf(stderr, " -u: libusb debug level: 0 to 3, 3 is most verbose. Defaults to '0'.\n");
		fprintf(stderr, "\n");
	}
}

int parse_args(int argc, char **argv)
{
	char c;
	while ((c = getopt(argc, argv, "r:n:f:b:t:u:")) != -1) {
		switch (c) {
		case 'r':
			sample_rate = slogic_parse_sample_rate(optarg);
			if (!sample_rate) {
				usage("Invalid sample rate: %s", optarg);
				return 1;
			}
			break;
		case 'f':
			output_file_name = optarg;
			break;
		case 'n':
			n_samples = strtol(optarg, NULL, 10);
			if (n_samples <= 0) {
				usage("Invalid number of samples, must be a positive integer: %s", optarg);
				return 1;
			}
			break;
		case 'b':
			transfer_buffer_size = strtol(optarg, NULL, 10);
			if (transfer_buffer_size <= 0) {
				usage("Invalid transfer buffer size, must be a positive integer: %s", optarg);
				return 1;
			}
			break;
		case 't':
			n_transfer_buffers = strtol(optarg, NULL, 10);
			if (n_transfer_buffers <= 0) {
				usage("Invalid transfer buffer size, must be a positive integer: %s", optarg);
				return 1;
			}
			break;
		case 'u':
			libusb_debug_level = strtol(optarg, NULL, 10);
			if (libusb_debug_level < 0 || libusb_debug_level > 3) {
				usage("Invalid libusb debug level, "
				      "must be a positive integer between 0 and 3: %s", optarg);
				return 1;
			}
			break;
		default:
		case '?':
			usage("Unknown argument: %c", optopt);
			return 1;
		}
	}

	if (!output_file_name) {
		usage("An output file has to be specified.", optarg);
		return 1;
	}

	if (!sample_rate) {
		usage("A sample rate has to be specified.", optarg);
		return 1;
	}

	if (!n_samples) {
		n_samples = sample_rate->samples_per_second;
	}

	return 0;
}

int main(int argc, char **argv)
{
	if (!parse_args(argc, argv)) {
		exit(EXIT_FAILURE);
	}

/* KEJO: the slogic_open should not perform the firmware uploading the hanlding should still be here */
/* Tryve: Why? It would be a pain for every user to have to handle the uploading of the firmware.
To be able to use the rest of the API (which basically are the sampling methods)
you would need a (compatible) firmware to be uploaded before usage. */

/* does firmware upload work for you? I guess not. to upload the firmware we need
to connmect to an other VENDOR/PRODUCT. it really deserves special hanling also because we currently fail 
to first upload and continue */
	struct slogic_handle *handle = slogic_open();
	assert(handle);

	slogic_tune(handle, transfer_buffer_size, n_transfer_buffers, libusb_debug_level);

	uint8_t *buffer = malloc(n_samples);
	assert(buffer);

	printf("slogic_read_samples\n");
	slogic_read_samples(handle, sample_rate, buffer, n_samples);

	slogic_close(handle);

	exit(EXIT_SUCCESS);
}
