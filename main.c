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


/* KEJO: I tend to like to put all those global variable in a struct that can be passed around 
this makes it possible to create threaded application as there will be no shared data in between */
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
int usage(const char *message, ...)
{
	int i;
	struct slogic_sample_rate *sample_rates;
	size_t n_sample_rates;

	slogic_available_sample_rates(&sample_rates, &n_sample_rates);

/* using the + is a c++/java thingy we let the pre processor do it */
	fprintf(stderr, "usage: %s -f <output file> -r <sample rate> "
		"[-n <number of samples>]\n", me);
	fprintf(stderr, "\n");
	fprintf(stderr, " -n: Number of samples to record\n");
	fprintf(stderr, "     Defaults to one second of samples for the "
		"specified sample rate\n");
	fprintf(stderr, " -f: The output file. Using '-' means that the "
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
	fprintf(stderr, " -u: libusb debug level: 0 to 3, 3 is most verbose. "
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

/* KEJO: It's not up to the user method to return a correct error code */
	return 0;
}

int parse_args(int argc, char **argv)
{
/* KEJO: compiler wanring that msg is not used */
	const char *msg;	/* The indentation gets insane */
	char c;
	while ((c = getopt(argc, argv, "r:n:f:b:t:u:")) != -1) {
		switch (c) {
		case 'r':
			sample_rate = slogic_parse_sample_rate(optarg);
			if (!sample_rate) {
/* KEJO: in the c/unix world returning 0 is "success" it would be good to keep consistent with that */
				return usage("Invalid sample rate: %s", optarg);
			}
			break;
		case 'f':
			output_file_name = optarg;
			break;
		case 'n':
			n_samples = strtol(optarg, NULL, 10);
			if (n_samples <= 0) {
				return usage("Invalid number of samples, "
					     "must be a positive integer: %s",
					     optarg);
			}
			break;
		case 'b':
			transfer_buffer_size = strtol(optarg, NULL, 10);
			if (transfer_buffer_size <= 0) {
				return usage("Invalid transfer buffer size, "
					     "must be a positive integer: %s",
					     optarg);
			}
			break;
		case 't':
			n_transfer_buffers = strtol(optarg, NULL, 10);
			if (n_transfer_buffers <= 0) {
				return usage("Invalid transfer buffer size, "
					     "must be a positive integer: %s",
					     optarg);
			}
			break;
		case 'u':
			libusb_debug_level = strtol(optarg, NULL, 10);
			if (libusb_debug_level < 0 || libusb_debug_level > 3) {
				return usage("Invalid libusb debug level, "
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
/* KEJO: read man 3 exit, EXIT_FAILURE is to be passed as argument to the exit() method */
		return EXIT_FAILURE;
	}

/* KEJO: the slogic_open should not perform the firmware uploading the hanlding should still be here */
	struct slogic_handle *handle = slogic_open();
	assert(handle);

	slogic_tune(handle, transfer_buffer_size, n_transfer_buffers,
		    libusb_debug_level);


/* KEJO: A c program would do this differently. just like Joe "c++" api one would expect to be able to register a callback
when data is ready, this should not be the full buffer but just one transfer. The API should make it possible to stream data.
*/
	uint8_t *buffer = malloc(n_samples);
	assert(buffer);

	printf("slogic_read_samples\n");
/* KEJO  can we make one single struct containting the major settings when sampling.
this should include the callback function. I think the n_samples should not be pass
but the callback function should return a 1 when we need to stop sampling */
	slogic_read_samples(handle, sample_rate, buffer, n_samples);

	slogic_close(handle);

/* KEJO if we use EXIT_FAILURE above lets use exit(EXIT_SECCESS) here */
	return 0;
}
