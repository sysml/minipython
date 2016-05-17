#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>

#include "tools_common.h"

struct disk *open_disk(const char *path, int mode)
{
	struct disk *d = NULL;
	struct stat fd_stat;
	int err;

	d = malloc(sizeof(*d));
	if (!d) {
		fatal();
		goto err_out;
	}

	d->path = strdup(path);
	if (!d->path) {
		fatal();
		goto err_free_d;
	}

	d->fd = open(d->path, mode);
	if (d->fd < 0) {
		eprintf("Could not open %s: %s\n", path , strerror(errno));
		goto err_free_path;
	}

	if (fstat(d->fd, &fd_stat) == -1) {
		eprintf("Could not retrieve stats from %s: %s\n", path, strerror(errno));
		goto err_free_path;
	}
	if (!S_ISBLK(fd_stat.st_mode) && !S_ISREG(fd_stat.st_mode)) {
		eprintf("%s is not a block device or a regular file\n", path);
		goto err_free_path;
	}
	if (!S_ISBLK(fd_stat.st_mode))
		dprintf(D_L0, "Note: %s is not a block device\n", path);

	/* get device size in bytes */
	if (S_ISBLK(fd_stat.st_mode)) {
		err = ioctl(d->fd, BLKGETSIZE64, &d->size);
		if (err) {
			unsigned long size32;

			dprintf(D_L0, "BLKGETSIZE64 failed. Trying BLKGETSIZE\n");
			err = ioctl(d->fd, BLKGETSIZE, &size32);
			if (err) {
				eprintf("Could not query device size from %s\n", path);
				goto err_free_path;
			}
			d->size = (uint64_t) size32;
		}
	} else {
		d->size = (uint64_t) fd_stat.st_size;
	}
	dprintf(D_L0, "%s has a size of %"PRIu64" bytes\n", path, d->size);

	/* get prefered block size in bytes */
	d->blksize = fd_stat.st_blksize;
	dprintf(D_L0, "%s has a block size of %"PRIu32" bytes\n", path, d->blksize);

	/* diable discard(trim) support */
	/* Note: We disable it here since it is not iplemented yet */
	d->discard = 0;

	return d;

 err_free_path:
	free(d->path);
 err_free_d:
	free(d);
 err_out:
	return NULL;
}

void close_disk(struct disk *d) {
	dprintf(D_L0, "Syncing %s...\n", d->path);
	fsync(d->fd); /* ignore errors */
	close(d->fd);
	free(d->path);
	free(d);
}

/* Performs I/O on the member disks of a volume */
int sync_io_chunk(struct storage *s, chk_t start, chk_t len, int owrite, void *buffer)
{
	off_t startb;
	unsigned int m;
	uint8_t *wptr = buffer;
	strp_t start_s;
	strp_t end_s;
	strp_t strp;

	assert(start != 0);

	if (s->stripemode == SHFS_SM_COMBINED) {
		start_s = (strp_t) start * (strp_t) s->nb_members;
		end_s = (strp_t) (start + len) * (strp_t) s->nb_members;
	} else { /* SHFS_SM_INTERLEAVED (chunksize == stripesize) */
		start_s = (strp_t) start + (strp_t) (s->nb_members - 1);
		end_s = (strp_t) (start_s + len);
	}

	for (strp = start_s; strp < end_s; ++strp) {
		m = strp % s->nb_members;
		startb = (strp / s->nb_members) * s->stripesize;
		dprintf(D_MAX, " %s chunk %"PRIstrp" on member %u (at %lu KiB, length: %u KiB)\n",
		        owrite ? "Writing to" : "Reading from",
		        s->stripemode == SHFS_SM_COMBINED ? strp / s->nb_members: strp - (s->nb_members - 1),
		        m,
		        startb / 1024,
		        s->stripesize / 1024);

		if (lseek(s->member[m].d->fd, startb, SEEK_SET) < 0) {
			eprintf("Could not seek on %s: %s\n", s->member[m].d->path, strerror(errno));
			return -1;
		}
		if (owrite) {
			if (write(s->member[m].d->fd, wptr, s->stripesize) < 0) {
				eprintf("Could not write to %s: %s\n", s->member[m].d->path, strerror(errno));
				return -1;
			}
		} else {
			if (read(s->member[m].d->fd, wptr, s->stripesize) < 0) {
				eprintf("Could not read from %s: %s\n", s->member[m].d->path, strerror(errno));
				return -1;
			}
		}

		wptr += s->stripesize;
	}

	return 0;
}

/* Performs a 'discard' on member disks of a volume
 * If a member does not support 'discard', the area is overwritten with zero's */
int sync_erase_chunk(struct storage *s, chk_t start, chk_t len) {
	off_t startb;
	unsigned int m;
	strp_t start_s;
	strp_t end_s;
	strp_t strp;
	void *strp0 = calloc(1, s->stripesize);
	uint64_t p = 0; /* progress in promille */

	if (!strp0)
		goto err_out;
	if (start == 0) {
		errno = EINVAL;
		goto err_free_strp0;
	}

	if (s->stripemode == SHFS_SM_COMBINED) {
		start_s = (strp_t) start * (strp_t) s->nb_members;
		end_s = (strp_t) (start + len) * (strp_t) s->nb_members;
	} else { /* SHFS_SM_INTERLEAVED (chunksize == stripesize) */
		start_s = (strp_t) start + (strp_t) (s->nb_members - 1);
		end_s = (strp_t) (start_s + len);
	}

	for (strp = start_s; strp < end_s; ++strp) {
		m = strp % s->nb_members;
		startb = (strp / s->nb_members) * s->stripesize;

		if (verbosity >= D_L0) {
			p = (strp - start_s + 1) * 1000 / (end_s - start_s);
			dprintf(D_L0, "\r Erasing chunk %"PRIstrp" on member %u (%"PRIu64".%01"PRIu64" %%)...       ",
			        s->stripemode == SHFS_SM_COMBINED ?
			        strp / s->nb_members : strp - (s->nb_members - 1),
			        m, p / 10, p % 10);
		}

		if (s->member[m].d->discard) {
			/* device supports discard */
			/* TODO: DISCARD NOT IMPLEMENTED YET */
			errno = ENOTSUP;
			goto err_free_strp0;
		} else {
			/* device does not support discard:
			 * overwrite area with zero's */
			if (lseek(s->member[m].d->fd, startb, SEEK_SET) < 0) {
				eprintf("Could not seek on %s: %s\n", s->member[m].d->path, strerror(errno));
				goto err_free_strp0;
			}
			if (write(s->member[m].d->fd, strp0, s->stripesize) < 0) {
				eprintf("Could not write to %s: %s\n", s->member[m].d->path, strerror(errno));
				goto err_free_strp0;
			}
		}
	}

	dprintf(D_L0, "\n");
	free(strp0);
	return 0;

 err_free_strp0:
	free(strp0);
 err_out:
	return -1;
}

void print_shfs_hdr_summary(struct shfs_hdr_common *hdr_common,
                            struct shfs_hdr_config *hdr_config)
{
	char     volname[17];
	uint64_t chunksize;
	uint64_t hentry_size;
	uint64_t htable_size;
	chk_t    htable_size_chks;
	uint32_t htable_total_entries;
	uint8_t  m;
	char str_uuid[37];
	char str_date[20];

	chunksize            = SHFS_CHUNKSIZE(hdr_common);
	hentry_size          = SHFS_HENTRY_SIZE;
	htable_total_entries = SHFS_HTABLE_NB_ENTRIES(hdr_config);
	htable_size_chks     = SHFS_HTABLE_SIZE_CHUNKS(hdr_config, chunksize);
	htable_size          = CHUNKS_TO_BYTES(htable_size_chks, chunksize);

	printf("SHFS version:      %2x.%02x\n",
	       hdr_common->version[0],
	       hdr_common->version[1]);
	strncpy(volname, hdr_common->vol_name, 16);
	volname[17] = '\0';
	printf("Volume name:        %s\n", volname);
	uuid_unparse(hdr_common->vol_uuid, str_uuid);
	printf("Volume UUID:        %s\n", str_uuid);
	strftimestamp_s(str_date, sizeof(str_date),
	                "%b %e, %g %H:%M", hdr_common->vol_ts_creation);
	printf("Creation date:      %s\n", str_date);
	printf("Chunksize:          %"PRIu64" KiB\n",
	       chunksize / 1024);
	printf("Volume size:        %"PRIu64" KiB\n",
	       (chunksize * hdr_common->vol_size) / 1024);

	printf("Hash function:      %s (%"PRIu32" bits)\n",
	       (hdr_config->hfunc == SHFUNC_SHA ? "SHA" :
	        (hdr_config->hfunc == SHFUNC_CRC ? "CRC" :
	         (hdr_config->hfunc == SHFUNC_MD5 ? "MD5" :
	          (hdr_config->hfunc == SHFUNC_HAVAL ? "HAVAL" :
	           (hdr_config->hfunc == SHFUNC_MANUAL ? "Manual" : "Unknown"))))),
	       hdr_config->hlen * 8);
	printf("Hash table:         %"PRIu32" entries in %"PRIu32" buckets\n" \
	       "                    %"PRIu64" chunks (%"PRIu64" KiB)\n" \
	       "                    %s\n",
	       htable_total_entries, hdr_config->htable_bucket_count,
	       htable_size_chks, htable_size / 1024,
	       hdr_config->htable_bak_ref ? "2nd copy enabled" : "No copy");
	printf("Entry size:         %"PRIu64" Bytes (raw: %zu Bytes)\n", hentry_size, sizeof(struct shfs_hentry));
	printf("Metadata total:     %"PRIu64" chunks\n", metadata_size(hdr_common, hdr_config));
	printf("Available space:    %"PRIu64" chunks\n", avail_space(hdr_common, hdr_config));

	printf("\n");
	printf("Member stripe size: %"PRIu32" KiB\n", hdr_common->member_stripesize / 1024);
	printf("Member stripe mode: %s\n", (hdr_common->member_stripemode == SHFS_SM_COMBINED ?
	                                    "Combined" : "Independent" ));
	printf("Volume members:     %"PRIu8" device(s)\n", hdr_common->member_count);
	for (m = 0; m < hdr_common->member_count; m++) {
		uuid_unparse(hdr_common->member[m].uuid, str_uuid);
		printf("  Member %2d UUID:   %s\n", m, str_uuid);
	}
}

size_t strshfshost(char *s, size_t slen, struct shfs_host *h)
{
	size_t ret = 0;
	size_t l;

	switch(h->type) {
	case SHFS_HOST_TYPE_NAME:
		l = min(slen, sizeof(h->name));
		ret = snprintf(s, l, "%s", h->name);
		break;
	case SHFS_HOST_TYPE_IPV4:
		ret = snprintf(s, slen, "%"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8,
			       h->addr[0], h->addr[1], h->addr[2], h->addr[3]);
		break;
	default:
		if (slen > 0)
			s[0] = '\0';
		errno = EINVAL;
		break;
	}

	return ret;
}

chk_t metadata_size(struct shfs_hdr_common *hdr_common,
                    struct shfs_hdr_config *hdr_config)
{
	uint64_t chunksize;
	chk_t    htable_size_chks;
	chk_t    ret = 0;

	chunksize        = SHFS_CHUNKSIZE(hdr_common);
	htable_size_chks = SHFS_HTABLE_SIZE_CHUNKS(hdr_config, chunksize);

	ret += 1; /* chunk0 (common hdr) */
	ret += 1; /* chunk1 (config hdr) */
	ret += htable_size_chks; /* hash table chunks */
	if (hdr_config->htable_bak_ref)
		ret += htable_size_chks; /* backup hash table */
	return ret;
}

chk_t avail_space(struct shfs_hdr_common *hdr_common,
                  struct shfs_hdr_config *hdr_config)
{
	return hdr_common->vol_size - metadata_size(hdr_common, hdr_config);
}
