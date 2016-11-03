#ifndef _LI2C_H
#define	_LI2C_H

#include "modules.h"

#include <stdint.h>
#include <sys/drivers/i2c.h>
#include <sys/drivers/cpu.h>

#ifdef I2C1
#define I2C_I2C1 {LSTRKEY(I2C1_NAME), LINTVAL(I2C1)},
#else
#define I2C_I2C1
#endif

#ifdef I2C2
#define I2C_I2C2 {LSTRKEY(I2C2_NAME), LINTVAL(I2C2)},
#else
#define I2C_I2C2
#endif

#ifdef I2C3
#define I2C_I2C3 {LSTRKEY(I2C3_NAME), LINTVAL(I2C3)},
#else
#define I2C_I2C3
#endif

#ifdef I2C4
#define I2C_I2C4 {LSTRKEY(I2C4_NAME), LINTVAL(I2C4)},
#else
#define I2C_I2C4
#endif

#ifdef I2C5
#define I2C_I2C5 {LSTRKEY(I2C5_NAME), LINTVAL(I2C5)},
#else
#define I2C_I2C5
#endif

#ifdef I2CBB1
#define I2C_I2CBB1 {LSTRKEY(I2CBB1_NAME), LINTVAL(I2CBB1)},
#else
#define I2C_I2CBB1
#endif

#ifdef I2CBB2
#define I2C_I2CBB2 {LSTRKEY(I2CBB2_NAME), LINTVAL(I2CBB2)},
#else
#define I2C_I2CBB2
#endif

#ifdef I2CBB3
#define I2C_I2CBB3 {LSTRKEY(I2CBB3_NAME), LINTVAL(I2CBB3)},
#else
#define I2C_I2CBB3
#endif

#ifdef I2CBB4
#define I2C_I2CBB4 {LSTRKEY(I2CBB4_NAME), LINTVAL(I2CBB4)},
#else
#define I2C_I2CBB4
#endif

#ifdef I2CBB5
#define I2C_I2CBB5 {LSTRKEY(I2CBB5_NAME), LINTVAL(I2CBB5)},
#else
#define I2C_I2CBB5
#endif

int platform_i2c_exists(int id);

#endif	

