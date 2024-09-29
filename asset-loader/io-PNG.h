#ifndef IO_PNG_H_
#define IO_PNG_H_

#include <core/pixel.h>
#include <stdio.h>

#define PNG_MAGIC_SIZE 8

/* Both functions return 0 on success,
 * and a non-zero error code on failure
 */
i32 read_PNG(struct pixel_flat_data *pixel_data, FILE *fp);
i32 write_PNG(struct pixel_flat_data *pixel_data, FILE *fp);

i32 load_libPNG();
void close_libPNG();

i32 is_PNG(FILE *fp);

#endif /* IO_PNG_H_ */
