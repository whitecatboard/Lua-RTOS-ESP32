#ifndef __HEX_H_
#define __HEX_H_

/**
 * @brief Check if a string is an hexadecimal string. A hexadecimal string is a representation of a series
 *        of bytes in text format, in which each byte is converted to it's hexadecimal representation
 *        in text.
 *
 *        For example:
 *
 *           0ABF3E is a valid hexadecimal string
 *
 * @param str The string to check.
 *
 * @return
 *     - 1 if str is an hexadecimal string
 *     - 0 if str is not an hexadecimal string
 */
int lcheck_hex_str(const char *str);

#endif
