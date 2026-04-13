/*!
 * \file post-api.c
 * \author Dorijan Di Zepp
 * \date 2026-04-13
 * \brief This file defines Power-On Self-Test (POST) functions for system diagnostics.
 */

#include "post-api.h"
#include "eagletrt-api.h"

enum PostReturnCode post_api_execute(const struct PostConfig *post_init_config) {
    if (post_init_config == NULL || post_init_config->test_table == NULL) {
        return POST_RC_ERROR;
    }

    for (uint8_t i = 0; i < post_init_config->num_tests; i++) {
        // check callback is valid and not NULL
        if (post_init_config->test_table[i].run == NULL) {
            return POST_RC_ERROR;
        }

        if (post_init_config->test_table[i].run() != POST_RC_OK) {
            return POST_RC_ERROR;
        }
    }

    return POST_RC_OK;
}