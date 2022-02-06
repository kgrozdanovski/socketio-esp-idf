/**
 * @file utility.c
 * 
 * @author Kristijan Grozdanovski (kgrozdanovski7@gmail.com)
 * 
 * @brief Common utility functions and macros.
 * 
 * @version 1
 * 
 * @date 2022-01-19
 * 
 * @copyright Copyright (c) 2022, Kristijan Grozdanovski
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. 
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

#include "esp_types.h"
#include "esp_log.h"

static const char *SIO_UTIL_TAG = "sio.util";

/**
 * @brief Optimized and safe string concatenation function.
 * 
 * @note This implementation is suggested by Joel Spolsky in this excellent
 * article - "Back to Basics", on his blog.
 * 
 * @link https://www.joelonsoftware.com/2001/12/11/back-to-basics/
 * 
 * @param destination 
 * @param source 
 * @return char* 
 */
char* util_str_cat(char *destination, char *source);

/**
 * @brief Extrapolate a substring from a C string.
 * 
 * @param substr 
 * @param string 
 * @param start 
 * @param end 
 * @return void
 */
void util_substr(
    char *substr,
    char *source,
    size_t *source_len,
    int start,
    int end
);

/**
 * @brief Returns pointer to a cleaned JSON body form any arbitrary string
 * containing one JSON body.
 * 
 * @note Thanks to Reddit user u/ogiota for sharing this function.
 * 
 * @link https://www.reddit.com/r/esp32/comments/p2tnyn/parsing_json_from_https_response/
 * 
 * @param pcBuffer 
 * @return char* // Returns NULL if no JSON body found
 */
char *util_extract_json(char *pcBuffer);

#ifdef __cplusplus
}
#endif