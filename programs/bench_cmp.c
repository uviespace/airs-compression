/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2026
 * @copyright GPL-2.0
 *
 * @brief Simple compression speed benchmark
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmp.h>
#include <cmp_errors.h>

#include "clock.h"
#include "fgs_frames.h"
#include "../lib/common/arena.h"
#include "../lib/common/bithacks.h"
#include "../lib/common/compiler.h"

ARENA_NORETURN static void bench_oom(void)
{
	fprintf(stderr, "Out of Memory!\n");
	exit(3);
}


struct bench_dataset {
	const char *name;
	const void **frames;
	uint32_t frame_size; /*size of ONE frame */
	int num_frames;
	enum cmp_type dtype;
};

struct bench_cfg {
	const char *name;
	struct cmp_params params;
	int repeats;
};

struct bench_stats {
	uint64_t total_ns;
	uint64_t min_ns;
	uint64_t max_ns;
	uint64_t input_bytes;
	uint64_t output_bytes;
	uint32_t runs;
};


static double bench_mibit_per_sec(uint64_t bytes, uint64_t elapsed_ns)
{
	double const mibits = (double)(bytes * 8) / (double)(1024 * 1024);
	double const seconds = (double)elapsed_ns / 1e9;

	if (elapsed_ns == 0)
		return 0.0;

	return mibits / seconds;
}


static void bench_stats_add(struct bench_stats *stats, uint32_t input_bytes, uint32_t output_bytes,
			    uint64_t elapsed_ns)
{
	if (elapsed_ns < stats->min_ns)
		stats->min_ns = elapsed_ns;
	if (elapsed_ns > stats->max_ns)
		stats->max_ns = elapsed_ns;

	stats->total_ns += elapsed_ns;
	stats->input_bytes += input_bytes;
	stats->output_bytes += output_bytes;
	stats->runs++;
}


static void bench_stats_print(struct bench_dataset const *ds, struct bench_cfg const *cfg,
			      struct bench_stats const *stats)
{
	double best_mib_per_sec;
	double worst_mib_per_sec;
	double total_mib_per_sec;
	double cmp_ratio;

	if (stats->runs == 0) {
		printf("      no successful runs\n");
		return;
	}

	best_mib_per_sec = bench_mibit_per_sec(stats->input_bytes / stats->runs, stats->min_ns);
	worst_mib_per_sec = bench_mibit_per_sec(stats->input_bytes / stats->runs, stats->max_ns);
	total_mib_per_sec = bench_mibit_per_sec(stats->input_bytes, stats->total_ns);
	cmp_ratio = stats->output_bytes == 0 ?
			    0.0 :
			    (double)stats->input_bytes / (double)stats->output_bytes;

	printf("   runs             : %" PRIu32 " (%d repeats x %d frames)\n", stats->runs,
	       cfg->repeats, ds->num_frames);
	printf("   time (us)        : total=%.1f, min=%.1f, max=%.1f\n",
	       (double)stats->total_ns / 1000.0, (double)stats->min_ns / 1000.0,
	       (double)stats->max_ns / 1000.0);
	printf("   eff. size (bytes): input=%" PRIu64 ", output=%" PRIu64 ", ratio=%.2f\n",
	       stats->input_bytes, stats->output_bytes, cmp_ratio);
	printf("   speed (Mibits/s) : total=%.2f, best=%.2f, worst=%.2f\n", total_mib_per_sec,
	       best_mib_per_sec, worst_mib_per_sec);
}


static uint32_t get_max_work_buf_size(const struct cmp_params *params,
				      const struct bench_dataset *datasets, int dataset_count)
{
	int ds_nb;
	uint32_t max_work_buf_size = 0;

	for (ds_nb = 0; ds_nb < dataset_count; ds_nb++) {
		uint32_t work_buf_size = cmp_cal_work_buf_size(params, datasets[ds_nb].frame_size,
							       datasets[ds_nb].dtype);

		if (cmp_is_error(work_buf_size))
			return work_buf_size;
		if (max_work_buf_size < work_buf_size)
			max_work_buf_size = work_buf_size;
	}

	return max_work_buf_size;
}


static uint32_t cal_dst_cap(uint32_t src_size, enum cmp_type src_dtype,
			    int uncompressed_fallback_enabled)
{
	uint32_t packed_size = src_dtype == CMP_I16_IN_I32 ? src_size / 2 : src_size;
	uint32_t dst_capacity;

	if (uncompressed_fallback_enabled)
		dst_capacity = (uint32_t)CMP_UNCOMPRESSED_BOUND(packed_size);
	else
		dst_capacity = cmp_compress_bound(src_size, src_dtype);

	if (cmp_is_error(dst_capacity)) {
		dst_capacity = CMP_HDR_MAX_COMPRESSED_SIZE;
		(void)fprintf(
			stderr,
			"Use fallback destination buffer size of CMP_HDR_MAX_COMPRESSED_SIZE.\n"
			"Compressed data may not fit into the destination buffer!\n");
	}

	return dst_capacity;
}


static int ptr_alignment(const void *ptr)
{
	if (is_aligned(ptr, 8))
		return 8;
	if (is_aligned(ptr, 4))
		return 4;
	if (is_aligned(ptr, 2))
		return 2;

	return 1;
}


static uint32_t bench_cmp(struct arena *a, struct bench_dataset *datasets, int dataset_count,
			  struct bench_cfg *cfgs, int cfg_count, uint32_t dst_capacity, int verbose)
{
	int cfg_nb, ds_nb, repeat_nb, frame_nb;

	for (cfg_nb = 0; cfg_nb < cfg_count; cfg_nb++) {
		struct arena scratch1 = *a;
		struct bench_cfg *cur_cfg = &cfgs[cfg_nb];
		uint32_t work_buf_size;
		void *work_buf;
		struct cmp_context ctx;
		uint32_t ret;

		printf("\n=== %s ===\n", cur_cfg->name);

		work_buf_size = get_max_work_buf_size(&cur_cfg->params, datasets, dataset_count);
		if (cmp_is_error(work_buf_size)) {
			printf("cfg[%d]: cmp_cal_work_buf_size() failed: %s\n", cfg_nb,
			       cmp_get_error_message(work_buf_size));
			continue;
		}
		work_buf =
			arena_alloc(&scratch1, (ptrdiff_t)work_buf_size, 1, __alignof__(uint64_t));
		ret = cmp_initialise(&ctx, &cur_cfg->params, work_buf, work_buf_size);
		if (cmp_is_error(ret)) {
			printf("cfg[%d]: cmp_initialise() failed: %s\n", cfg_nb,
			       cmp_get_error_message(ret));
			continue;
		}

		for (ds_nb = 0; ds_nb < dataset_count; ds_nb++) {
			struct bench_dataset *cur_ds = &datasets[ds_nb];
			struct bench_stats stats = { 0 };
			uint32_t dst_cap_used;

			stats.min_ns = ~0ULL;

			if (dst_capacity)
				dst_cap_used = dst_capacity;
			else
				dst_cap_used =
					cal_dst_cap(cur_ds->frame_size, cur_ds->dtype,
						    cur_cfg->params.uncompressed_fallback_enabled);

			printf("   === %s ===\n", cur_ds->name);

			for (repeat_nb = 0; repeat_nb < cur_cfg->repeats; repeat_nb++) {
				struct arena scratch2 = scratch1;
				uint32_t err_code;

				err_code = cmp_reset(&ctx);
				if (cmp_is_error(err_code))
					return err_code;

				for (frame_nb = 0; frame_nb < cur_ds->num_frames; frame_nb++) {
					void *dst_buf;
					uint64_t start;
					uint32_t cmp_size;
					uint64_t t_run_ns;

					uint32_t const effective_input_size =
						cur_ds->dtype != CMP_I16_IN_I32 ?
							cur_ds->frame_size :
							cur_ds->frame_size / 2;


					dst_buf = arena_alloc(&scratch2, (ptrdiff_t)dst_cap_used, 1,
							      CMP_DST_ALIGNMENT);


					/* ---- TIMED REGION ---- */
					start = clock_get_time();

					switch (cur_ds->dtype) {
					case CMP_I16:
						cmp_size = cmp_compress_i16(
							&ctx, dst_buf, dst_cap_used,
							cur_ds->frames[frame_nb],
							cur_ds->frame_size);
						break;
					case CMP_I16_IN_I32:
						cmp_size = cmp_compress_i16_in_i32(
							&ctx, dst_buf, dst_cap_used,
							cur_ds->frames[frame_nb],
							cur_ds->frame_size);
						break;
					case CMP_U16:
						cmp_size = cmp_compress_u16(
							&ctx, dst_buf, dst_cap_used,
							cur_ds->frames[frame_nb],
							cur_ds->frame_size);
						break;
					case CMP_RAW12:
						cmp_size = cmp_compress_raw12(
							&ctx, dst_buf, dst_cap_used,
							cur_ds->frames[frame_nb],
							cur_ds->frame_size);
						break;
					default:
						printf("error: %s: unknown data type\n",
						       cur_ds->name);
						return -1U;
					}

					t_run_ns = clock_get_elapsed_nano(start);
					/* ---- END TIMED REGION ---- */


					if (cmp_is_error(cmp_size)) {
						printf("compression failed: %s\n",
						       cmp_get_error_message(cmp_size));
						return cmp_size;
					}
					/* Release the unused memory. */
					arena_shrink_last(&scratch2, dst_buf,
							  (ptrdiff_t)dst_cap_used,
							  (ptrdiff_t)cmp_size);

					bench_stats_add(&stats, effective_input_size, cmp_size,
							t_run_ns);

					if (verbose)
						printf("      run %02d frame %02d (ali:%d): %8.2f Mibit/s, %8" PRIu32
						       " -> %" PRIu32 " bytes, %8" PRIu64 " ns\n",
						       repeat_nb + 1, frame_nb + 1,
						       ptr_alignment(cur_ds->frames[frame_nb]),
						       bench_mibit_per_sec(effective_input_size,
									   t_run_ns),
						       effective_input_size, cmp_size, t_run_ns);
				}
			}

			bench_stats_print(cur_ds, cur_cfg, &stats);
			printf("\n");
		}
	}
	return 0;
}


int main(int argc, char **argv)
{
	static uint8_t buf[32 * 64 * 64 * 3];
	struct arena a = arena_init(buf, sizeof(buf));
	int argi;
	uint32_t err_code;
	int verbose = 0;
	struct bench_cfg cfgs[3] = { 0 };
	struct bench_dataset ds[3] = { 0 };

	arena_set_oom_handler(bench_oom);

	/*
	 * Prepare the data sets we want to bench
	 */
	ds[0].name = "32 FGS Windows as U16 (64 x 64)";
	ds[0].frames = fgs_frames;
	ds[0].frame_size = sizeof(fgs_u16_data.fgs_frame1);
	ds[0].num_frames = ARRAY_SIZE(fgs_frames);
	ds[0].dtype = CMP_U16;

	ds[1].name = "32 FGS Windows as I16 (64 x 64)";
	/* To save some memory we use for the I16 the same data as for the U16 */
	ds[1].frames = fgs_frames;
	ds[1].frame_size = sizeof(fgs_u16_data.fgs_frame1);
	ds[1].num_frames = ARRAY_SIZE(fgs_frames);
	ds[1].dtype = CMP_I16;

	ds[2].name = "32 FGS Windows as I16_IN_I32 (64 x 64)";
	ds[2].frames = fgs_frames_as_i32;
	ds[2].frame_size = sizeof(fgs_i32_data.fgs_frame_i32_1);
	ds[2].num_frames = ARRAY_SIZE(fgs_frames_as_i32);
	ds[2].dtype = CMP_I16_IN_I32;

	/*
	 * Prepare the different compression configuration we want to bench
	 */
	cfgs[0].name = "NONE/UNCOMPRESSED mode";
	cfgs[0].params.primary_preprocessing = CMP_PREPROCESS_NONE;
	cfgs[0].params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	cfgs[0].repeats = 2;

	cfgs[1].name = "DIFF/GOLOMB_ZERO mode";
	cfgs[1].params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	cfgs[1].params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	cfgs[1].params.primary_encoder_param = 1024;
	cfgs[1].repeats = 2;

	cfgs[2].name = "MODEL/GOLOMB_MULTI & DIFF/GOLOMB_ZERO mode";
	cfgs[2].params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	cfgs[2].params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	cfgs[2].params.primary_encoder_param = 1024;
	cfgs[2].params.secondary_iterations = 16;
	cfgs[2].params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	cfgs[2].params.secondary_encoder_type = CMP_ENCODER_GOLOMB_MULTI;
	cfgs[2].params.secondary_encoder_param = 512;
	cfgs[2].params.secondary_encoder_outlier = ~0U;
	cfgs[2].params.model_rate = 13;
	cfgs[2].repeats = 2;


	for (argi = 1; argi < argc; argi++) {
		if (strcmp(argv[argi], "--verbose") == 0 || strcmp(argv[argi], "-v") == 0) {
			verbose = 1;
			continue;
		}

		fprintf(stderr, "usage: %s [--verbose|-v]\n", argv[0]);
		return EXIT_FAILURE;
	}

	err_code = bench_cmp(&a, ds, ARRAY_SIZE(ds), cfgs, ARRAY_SIZE(cfgs), 0, verbose);
	if (cmp_is_error(err_code))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
