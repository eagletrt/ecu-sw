/*!
 * \file post-api.h
 * \author Dorijan Di Zepp
 * \date 2026-06-01
 * \brief This file defines Power-On Self-Test (POST) functions for system initialization.
 */

#ifndef POST_API_H
#define POST_API_H

#include "post.h"

/*!
 * \brief Performs the Power-On Self-Test (POST) using the provided configuration.
 * If one initialization fails, the function will still continue to initialize every module
 * but ultimately will return \ref POST_RC_ERROR.
 *
 * \param[in] post_init_config Pointer to a structure containing the callbacks required for all modules.
 *
 * \retval POST_RC_OK if all modules have been initialized successfully.
 * \retval POST_RC_ERROR if any module was not possible to initialize or in the presence of a \c NULL.
 */
enum PostReturnCode post_api_do_init(struct PostConfig *post_config);

#endif