#ifndef LUA_RTOS_SYS_PATH_H_
#define LUA_RTOS_SYS_PATH_H_

/**
 * @brief Make all directories in a path, like mkdir -p.
 *
 * @return
 *    Returns the value 0 if successful; otherwise the value -1 is returned,
 *    and the global variable errno	is set to indicate the error:
 */
int mkpath(const char *path);

/**
 * @brief Make a file if not exists.
 *
 * @return
 *    Returns the value 0 if successful; otherwise the value -1 is returned,
 *    and the global variable errno	is set to indicate the error:
 */
int mkfile(const char *path);

#endif /* COMPONENTS_LUA_RTOS_SYS_PATH_H_ */
