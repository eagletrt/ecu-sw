/*!
 * \file post-api.h
 * \author Dorijan Di Zepp
 * \date 2026-04-13
 * \brief This file defines Power-On Self-Test (POST) functions for system diagnostics.
 */

#ifndef POST_API_H
#define POST_API_H

#include "post.h"

/*!
 * \brief Performs the Power-On Self-Test (POST) using the provided configuration table.
 * This function iterates through the provided test table and executes each callback 
 * sequentially. It uses a "fail-fast" approach: if any test returns \ref POST_RC_ERROR, 
 * the function exits immediately and returns the error code.
 *
 * \warning The \c .num_tests field must exactly match the number of valid elements in 
 * the \c .test_table array. Providing a count larger than the actual array size 
 * results in undefined behavior (reading out-of-bounds memory).
 *
 * \param[in] post_init_config Pointer to a structure containing the test table.
 *
 * \retval POST_RC_OK if all tests in the table completed successfully.
 * \retval POST_RC_ERROR if a test failed or if the provided configuration is invalid.
 */
enum PostReturnCode post_api_execute(const struct PostConfig *post_init_config);

#endif