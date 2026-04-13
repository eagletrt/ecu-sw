/*!
 * \file post.h
 * \author Dorijan Di Zepp
 * \date 2026-04-13
 * \brief This file defines Power-On Self-Test (POST) structures for system diagnostics.
 */

#ifndef POST_H
#define POST_H

#include <stdint.h>

/*!
 * \brief Return codes for the post module APIs
 */
enum PostReturnCode {
    POST_RC_OK,    /*!< POST completed successfully. */
    POST_RC_ERROR, /*!< POST encountered an error. */
};

/**
 * \brief Signature for a general POST test
 * \retval POST_RC_OK if the callback completed successfully
 * \retval POST_RC_ERROR if the callback failed
 */
typedef enum PostReturnCode (*post_test_callback)(void);

/*!
 * \brief Definition of a single Power-On Self-Test task.
 */
struct PostTask {
    const char *test_name;  /*!< Name that can be used for logging. Can be set to NULL if not required. */
    post_test_callback run; /*!< Test function callback. */
};

/*!
 * \brief The configuration passed to the POST module.
 */
struct PostConfig {
    const struct PostTask *test_table; /*!< Pointer to the array of tests. */
    uint8_t num_tests;                 /*!< The exact number of tasks to run */
};

#endif