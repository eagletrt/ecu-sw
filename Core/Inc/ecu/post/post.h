/*!
 * \file post.h
 * \author Dorijan Di Zepp
 * \date 2026-06-01
 * \brief This file defines Power-On Self-Test (POST) structures for system initialization.
 */

#ifndef POST_H
#define POST_H

#include "buzzer.h"
#include "pedals.h"
#include "tractive-system.h"

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
 * \brief The configuration passed to the POST module.
 */
struct PostConfig {
    post_test_callback check_sys_clock;
    post_test_callback check_power_rails;
    post_test_callback check_can_bus;
    post_test_callback check_pedals;
};

#endif