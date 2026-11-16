/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data preprocessing header file
 *
 * Example:
 * const struct preprocessing_method *preprocess =
 *	preprocessing_get_method(CMP_PREPROCESS_IWT);
 *
 * if (preprocess == NULL)
 *	return -1; /1* Handle error: Preprocessing method not found *1/
 *
 * uint32_t n_values = preprocess->init(src, work_buf, work_buf_size);
 * if (cmp_is_error_int(n_values))  /1* Handle error: Preprocessing initialisation failed *1/
 *	return n_values;
 *
 * for (uint32_t i = 0; i < n_values; i++) {
 *	int16_t const preprocessed_value = preprocess->process(i, src, work_buf);
 *
 *	/1* Do something with preprocessed_value here (like compress it) *1/
 *	(void)preprocessed_value;
 * }
 */

#ifndef CMP_PREPROCESS_H
#define CMP_PREPROCESS_H

#include <stdint.h>

#include "../cmp.h"
#include "../common/sample_reader.h"


/**
 * @brief Rounds up a number to the next multiple of 2
 *
 * @param n	integer to be rounded
 *
 * @returns next even number or the input if already even
 */

#define ROUND_UP_TO_NEXT_2(n) (((n) + 1U) & ~1U)


/**
 * @brief Preprocessing method structure
 */
struct preprocessing_method {
	enum cmp_preprocessing type;
	uint32_t (*get_work_buf_size)(uint32_t input_size);
	uint32_t (*init)(const struct sample_desc *src_desc, void *work_buf,
			 uint32_t work_buf_size);
	int16_t (*process)(uint32_t i, const struct sample_desc *src_desc, void *work_buf);
};


/**
 * @brief Gets the preprocessing method structure for a given type
 *
 * @param type	preprocessing method type
 *
 * @returns a pointer to the cmp_preprocessing structure, or NULL if not found
 */
const struct preprocessing_method *preprocessing_get_method(enum cmp_preprocessing type);


#endif /* CMP_PREPROCESS_H */
