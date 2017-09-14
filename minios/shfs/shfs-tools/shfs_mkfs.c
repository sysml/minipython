/*
 * Simple hash filesystem (SHFS) tools
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2013-2017, NEC Europe Ltd., NEC Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <uuid/uuid.h>

#include "shfs_mkfs.h"

unsigned int verbosity = 0;
int force = 0;

/******************************************************************************
 * ARGUMENT PARSING                                                           *
 ******************************************************************************/
const char *short_opts = "h?vVfn:s:cb:e:xF:l:";

static struct option long_opts[] = {
	{"help",		no_argument,		NULL,	'h'},
	{"version",		no_argument,		NULL,	'V'},
	{"verbose",		no_argument,		NULL,	'v'},
	{"force",		no_argument,		NULL,	'f'},
	{"name",		required_argument,	NULL,	'n'},
	{"stripesize",		required_argument,	NULL,	's'},
	{"combined-striping",	no_argument,		NULL,	'c'},
	{"bucket-count",	required_argument,	NULL,	'b'},
	{"entries-per-bucket",	required_argument,	NULL,	'e'},
	{"erase",		no_argument,		NULL,	'x'},
	{"hash-function",	required_argument,	NULL,	'F'},
	{"hash-length",		required_argument,	NULL,	'l'},
	{NULL, 0, NULL, 0} /* end of list */
};

static inline void print_version()
{
	printf("%s v%u.%02u (built: %s %s)\n", STR_VERSION, SHFS_MAJOR, SHFS_MINOR,
	       __DATE__, __TIME__);
}

static void print_usage(char *argv0)
{
	printf("Usage: %s [OPTION]... [DEVICE]...\n", argv0);
	printf("Formats a device with SHFS.\n");
	printf("\n");
	printf("Mandatory arguments to long options are mandatory for short options too.\n");
	printf("\n");
	printf(" General option:\n");
	printf("  -h, --help                       displays this help and exit\n");
	printf("  -V, --version                    displays program version and exit\n");
	printf("  -v, --verbose                    increases verbosity level (max. %d times)\n", D_MAX);
	printf("  -f, --force                      suppresses user questions\n");
	printf("  -x, --erase                      erase volume area (full format)\n");
	printf("\n");
	printf(" Volume settings:\n");
	printf("  -n, --name [NAME]                sets volume name to NAME\n");
	printf("  -s, --stripesize [BYTES]         sets the stripesize for each volume member\n");
	printf("  -c, --combined-striping          enables combined striping for the volume\n");
	printf("\n");
	printf(" Hash table related configuration:\n");
	printf("  -b, --bucket-count [COUNT]       sets the total number of buckets\n");
	printf("  -e, --entries-per-bucket [COUNT] sets the number of entries for each bucket\n");
	printf("  -F, --hash-function [FUNCTION]   sets the object hashing function:\n");
	printf("                                    sha (default), crc, md5, haval, manual\n");
	printf("  -l, --hash-length [BYTES]        sets the the hash digest length in bytes\n");
	printf("                                    at least 1 (8 Bits), at most 64 (512 Bits)\n");
}

static inline void release_args(struct args *args)
{
	memset(args, 0, sizeof(*args));
}

static int parse_args(int argc, char **argv, struct args *args)
/*
 * Parse arguments on **argv (number of args on argc)
 * with GNUOPTS to *args
 *
 * This function will exit the program for itself
 * when -h or -V is parsed or on fatal errors
 * (such as ENOMEM)
 *
 * -EINVAL will be returned on parsing errors or
 * invalid options
 *
 * *args has to be passed in a cleared state
 */
{
	int opt, opt_index = 0;
	int tmp, ret;

	/*
	 * set default values
	 */
	args->volname[0]  = 'u';
	args->volname[1]  = 'n';
	args->volname[2]  = 'n';
	args->volname[3]  = 'a';
	args->volname[4]  = 'm';
	args->volname[5]  = 'e';
	args->volname[6]  = 'd';
	args->volname[7]  = '\0';
	args->volname[17] = '\0';
	args->stripesize = 16384;
	args->allocator = SALLOC_FIRSTFIT;
	args->bucket_count = 2048;
	args->entries_per_bucket = 8;
	args->fullerase = 0;
	args->combined_striping = 0;

	args->hashfunc = SHFUNC_SHA;
	args->hashlen = 0; /* set to default after parsing */

	/*
	 * Parse options
	 */
	while (1) {
		opt = getopt_long(argc, argv, short_opts, long_opts, &opt_index);

		if (opt == -1)    /* end of options */
			break;

		switch (opt) {
		case 'h':
		case '?': /* usage */
			print_usage(argv[0]);
			exit(EXIT_SUCCESS);
		case 'V': /* version */
			print_version();
			exit(EXIT_SUCCESS);
		case 'v': /* verbosity */
			if (verbosity < D_MAX)
				verbosity++;
			break;
		case 'f': /* force */
			force = 1;
			break;
		case 'n': /* name */
			strncpy(args->volname, optarg, sizeof(args->volname) - 1);
			break;
		case 's': /* stripesize */
			ret = parse_args_setval_int(&tmp, optarg);
			if (ret < 0 ||
			    tmp < 4096 ||
			    !POWER_OF_2(tmp) ||
			    tmp > 32768) {
				eprintf("Invalid stripe size (min. 4096, max. 32768, and has to be a power of two)\n");
				return -EINVAL;
			}
			args->stripesize = (uint32_t) tmp;
			break;
		case 'b': /* bucket-count */
			ret = parse_args_setval_int(&tmp, optarg);
			if (ret < 0 || tmp < 1) {
				eprintf("Invalid bucket count (min. 1)\n");
				return -EINVAL;
			}
			args->bucket_count = (uint32_t) tmp;
			break;
		case 'e': /* entries-per-bucket */
			ret = parse_args_setval_int(&tmp, optarg);
			if (ret < 0 || tmp < 1) {
				eprintf("Invalid number of entries per bucket (min. 1)\n");
				return -EINVAL;
			}
			args->entries_per_bucket = (uint32_t) tmp;
			break;
		case 'x': /* erase whole volume (full format) */
			args->fullerase = 1;
			break;
		case 'c': /* combined striping */
			args->combined_striping = 1;
			break;
		case 'F': /* hash function */
			if        (strcmp("sha", optarg) == 0) {
				args->hashfunc = SHFUNC_SHA;
			} else if (strcmp("crc", optarg) == 0) {
				args->hashfunc = SHFUNC_CRC;
			} else if (strcmp("md5", optarg) == 0) {
				args->hashfunc = SHFUNC_MD5;
			} else if (strcmp("haval", optarg) == 0) {
				args->hashfunc = SHFUNC_HAVAL;
			} else if (strcmp("manual", optarg) == 0) {
				args->hashfunc = SHFUNC_MANUAL;
			} else {
				eprintf("Unknown hash function specified\n");
				return -EINVAL;
			}
			break;
		case 'l': /* hash length */
			ret = parse_args_setval_int(&tmp, optarg);
			if (ret < 0 || tmp < 1 || tmp > 64) {
				eprintf("Invalid hash digest length (min. 1, max. 64)\n");
				return -EINVAL;
			}
			args->hashlen = (uint8_t) tmp;
			break;
		default:
			/* unknown option */
			return -EINVAL;
		}
	}

	/* set default hash length if it was not set yet */
	if (!args->hashlen) {
		switch(args->hashfunc) {
		case SHFUNC_SHA:
			args->hashlen = 32; /* 256 bits */
			break;
		case SHFUNC_CRC:
			args->hashlen =  4; /* 32 bits */
			break;
		case SHFUNC_MD5:
			args->hashlen = 16; /* 128 bits */
			break;
		case SHFUNC_HAVAL:
			args->hashlen = 32; /* 256 bits */
			break;
		case SHFUNC_MANUAL:
		default:
			args->hashlen = 16; /* 128 bits */
		}
	}

	/* hash length check */
	switch(args->hashfunc) {
	case SHFUNC_SHA:
		if (args->hashlen != 20 && /* 160 bits */
		    args->hashlen != 28 && /* 224 bits */
		    args->hashlen != 32 && /* 256 bits */
		    args->hashlen != 48 && /* 384 bits */
		    args->hashlen != 64) { /* 512 bits */
			printf("SHA supports only the following hash digest lengths: "
			       "20, 28, 32, 48, 64\n");
			return -EINVAL;
		}
		break;
	case SHFUNC_CRC:
		if (args->hashlen !=  4) { /* 32 bits */
			printf("CRC supports only the following hash digest lengths: "
			       "4\n");
			return -EINVAL;
		}
		break;
	case SHFUNC_MD5:
		if (args->hashlen != 16) { /* 128 bits */
			printf("MD5 supports only the following hash digest lengths: "
			       "16\n");
			return -EINVAL;
		}
		break;
	case SHFUNC_HAVAL:
		if (args->hashlen != 16 && /* 128 bits */
		    args->hashlen != 20 && /* 160 bits */
		    args->hashlen != 24 && /* 192 bits */
		    args->hashlen != 28 && /* 224 bits */
		    args->hashlen != 32) { /* 256 bits */
			printf("HAVAL supports only the following hash digest lengths: "
			       "16, 20, 24, 28, 32, 48, 64\n");
			return -EINVAL;
		}
		break;
	case SHFUNC_MANUAL:
	default:
		break;
	}

	/* bucket/entry overflow check */
	if (((uint64_t) args->bucket_count) * ((uint64_t) args->entries_per_bucket) > UINT32_MAX) {
		printf("Combination of bucket count and entries per bucket leads to unsupported hash table size\n");
		return -EINVAL;
	}

	/* extra arguments are devices... just add a reference of those to args */
	if (argc <= optind) {
		eprintf("Path to device(s) not specified\n");
		return -EINVAL;
	}
	args->devpath = &argv[optind];
	args->nb_devs = argc - optind;

	return 0;
}

/******************************************************************************
 * SIGNAL HANDLING                                                            *
 ******************************************************************************/

static volatile int cancel = 0;

void sigint_handler(int signum) {
	printf("Caught abort signal: Cancelling...\n");
	cancel = 1;
}

/******************************************************************************
 * MAIN                                                                       *
 ******************************************************************************/
static void mkfs(struct storage *s, struct args *args)
{
	int ret;
	void *chk0;
	void *chk0_zero;
	void *chk1;
	struct shfs_hdr_common *hdr_common;
	struct shfs_hdr_config *hdr_config;
	chk_t htable_size;
	uint64_t mdata_size;
	uint64_t chunksize;
	uint64_t member_dsize;
	uint8_t m;

	/*
	 * Fillout headers / init entries
	 */
	/* chunk0: common header */
	chk0      = calloc(1, 4096);
	chk0_zero = calloc(1, 4096);
	if (!chk0 || !chk0_zero)
		die();
	hdr_common = chk0 + BOOT_AREA_LENGTH;
	hdr_common->magic[0] = SHFS_MAGIC0;
	hdr_common->magic[1] = SHFS_MAGIC1;
	hdr_common->magic[2] = SHFS_MAGIC2;
	hdr_common->magic[3] = SHFS_MAGIC3;
	hdr_common->version[0] = SHFS_MAJOR;
	hdr_common->version[1] = SHFS_MINOR;
	uuid_generate(hdr_common->vol_uuid);
	strncpy(hdr_common->vol_name, args->volname, 16);
#if __BYTE_ORDER == __LITTLE_ENDIAN
	hdr_common->vol_byteorder = SBO_LITTLEENDIAN;
#elif __BYTE_ORDER == __BIG_ENDIAN
	hdr_common->vol_byteorder = SBO_BIGENDIAN;
#else
#error "Could not detect byte-order"
#endif
	hdr_common->vol_encoding = SENC_UNSPECIFIED;
	hdr_common->vol_ts_creation = gettimestamp_s();

	/* setup striping */
	hdr_common->member_count = s->nb_members;
	hdr_common->member_stripemode = s->stripemode;
	hdr_common->member_stripesize = s->stripesize;
	for (m = 0; m < s->nb_members; ++m)
		uuid_generate(hdr_common->member[m].uuid);

	/* calculate volume size
	 * Note: The smallest member limits the volume size
	 * Note: First chunk is not striped across members (required for volume
	 *       detection). Because of this, it's size is just the stripesize
	 *       (not chunksize). Note, minimum stripesize is 4 KiB. */
	chunksize = SHFS_CHUNKSIZE(hdr_common);
	member_dsize = s->member[0].d->size;
	for (m = 1; m < s->nb_members; ++m) {
		if (s->member[m].d->size < member_dsize)
			member_dsize = s->member[m].d->size;
	}
	if (hdr_common->member_stripemode == SHFS_SM_COMBINED) {
		hdr_common->vol_size = (chk_t) ((member_dsize - chunksize + s->stripesize) / s->stripesize);
	} else { /* SHFS_SM_INTERLEAVED */
		hdr_common->vol_size = (chk_t) (((member_dsize - chunksize) / chunksize) * s->nb_members);
	}

	/* chunk1: config header */
	chk1 = calloc(1, chunksize);
	if (!chk1)
		die();
	hdr_config = chk1;
	hdr_config->htable_ref = 2;
	hdr_config->htable_bak_ref = 0; /* disable htable backup */
	hdr_config->hfunc = args->hashfunc;
	hdr_config->hlen = args->hashlen;
	hdr_config->htable_bucket_count = args->bucket_count;
	hdr_config->htable_entries_per_bucket = args->entries_per_bucket;
	hdr_config->allocator = args->allocator;

	/*
	 * Check device size
	 */
	mdata_size = metadata_size(hdr_common, hdr_config);
	if (mdata_size > hdr_common->vol_size)
		dief("Disk label requires more space than available on members\n");

	/*
	 * Summary
	 */
	if (cancel)
		exit(-2);
	print_shfs_hdr_summary(hdr_common, hdr_config);
	if (!force) {
		char *rlin = NULL;
		size_t n = 2;
		int num_rlin_bytes;

		printf("\n");
		printf("Shall this label be written to the device?\n");
		printf("Be warned that all existing data will be lost!\n");
		printf("Continue? [yN] ");
		num_rlin_bytes = getline(&rlin, &n, stdin);

		if (num_rlin_bytes < 0)
			die();
		if (rlin[0] != 'y' && rlin[0] != 'Y') {
			printf("Aborted\n");
			exit(EXIT_SUCCESS);
		}

		if (rlin)
			free(rlin);
	}
	if (cancel)
		exit(-2);
	printf("\n");

	/*
	 * Erase common header area (quick erase) in order to disable device
	 *  detection. THis is done for the case that the user cancels the operation
	 */
	for (m = 0; m < s->nb_members; ++m) {
		printf("Erasing common header area of member %u/%u...\n",
		       m + 1, s->nb_members);

		ret = lseek(s->member[m].d->fd, 0, SEEK_SET);
		if (ret < 0)
			die();
		ret = write(s->member[m].d->fd, chk0_zero, 4096);
		if (ret < 0)
			die();
	}

	/*
	 * Erase hash table area or do a full format
	 */
	if (args->fullerase) {
		/* full format */
		printf("\rErasing volume area...\n");
		ret = sync_erase_chunk(s, 2, hdr_common->vol_size - 2);
		if (ret < 0)
			die();
	} else {
		/* quick format */
		htable_size = SHFS_HTABLE_SIZE_CHUNKS(hdr_config, chunksize);
		printf("\rErasing hash table area...\n");
		ret = sync_erase_chunk(s, hdr_config->htable_ref, htable_size);
		if (ret < 0)
			die();

		if (hdr_config->htable_bak_ref) {
			printf("\rErasing backup hash table area...\n");
			ret = sync_erase_chunk(s, hdr_config->htable_bak_ref, htable_size);
			if (ret < 0)
				die();
		}
	}

	/*
	 * Write headers
	 */
	if (cancel)
		exit(-2);
	printf("Writing config header...\n");
	ret = sync_write_chunk(s, 1, 1, chk1);
	if (ret < 0)
		die();

	if (cancel)
		exit(-2);

	for (m = 0; m < s->nb_members; ++m) {
		printf("Writing common header area to member %u/%u...\n",
		       m + 1, s->nb_members);

		ret = lseek(s->member[m].d->fd, 0, SEEK_SET);
		if (ret < 0)
			die();

		/* write header with member's uuid */
		uuid_copy(hdr_common->member_uuid, hdr_common->member[m].uuid);
		ret = write(s->member[m].d->fd, chk0, 4096);
		if (ret < 0)
			die();
	}

	free(chk1);
	free(chk0_zero);
	free(chk0);
}

int main(int argc, char **argv)
{
	struct args args;
	struct storage s;
	uint8_t m;

	signal(SIGINT,  sigint_handler);
	signal(SIGTERM, sigint_handler);
	signal(SIGQUIT, sigint_handler);

	/*
	 * ARGUMENT PARSING
	 */
	memset(&args, 0, sizeof(args));
	if (parse_args(argc, argv, &args) < 0)
		exit(EXIT_FAILURE);
	if (verbosity > 0) {
		fprintf(stderr, "Verbosity increased to level %d.\n", verbosity);
	}
	printvar(args.nb_devs, "%"PRIu8);
	printvar(args.encoding, "%d");
	printvar(args.volname, "%s");
	printvar(args.stripesize, "%"PRIu32);

	printvar(args.hashfunc, "%"PRIu8);
	printvar(args.allocator, "%"PRIu8);
	printvar(args.hashlen, "%"PRIu32);
	printvar(args.bucket_count, "%"PRIu32);
	printvar(args.entries_per_bucket, "%"PRIu32);

	/*
	 * MAIN
	 */
	if (args.nb_devs > SHFS_MAX_NB_MEMBERS) {
		printf("Sorry, supporting at most %u members for volume format.\n",
		       SHFS_MAX_NB_MEMBERS);
		exit(EXIT_FAILURE);
	}
	s.nb_members = args.nb_devs;
	s.stripesize = args.stripesize;
	s.stripemode = (args.combined_striping && (args.nb_devs > 1)) ?
		SHFS_SM_COMBINED : SHFS_SM_INDEPENDENT;
	for (m = 0; m < s.nb_members; ++m) {
		s.member[m].d = open_disk(args.devpath[m], O_RDWR);
		if (!s.member[m].d)
			exit(EXIT_FAILURE);
	}
	if (cancel)
		exit(-2);
	mkfs(&s, &args);
	for (m = 0; m < s.nb_members; ++m)
		close_disk(s.member[m].d);

	/*
	 * EXIT
	 */
	release_args(&args);
	exit(EXIT_SUCCESS);
}
