/* Interface to the FreeType2 library. */

extern void ft_load(const char *font, int size, int dpi);
extern int ft_char(int c, int *raster_width, int *character_width,
                   int *height, int *kern, int *top);
extern void ft_draw(void);
