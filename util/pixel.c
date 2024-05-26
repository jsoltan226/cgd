#include "pixel.h"
#include "int.h"

void destroy_pixel_data(struct pixel_data *pixel_data)
{
    if (pixel_data == NULL) return;

    for (u32 i = 0; i < pixel_data->h; i++)
        free(pixel_data->data[i]);

    free(pixel_data);
}
