void svg_file_begin(FILE *f);
void svg_file_end(FILE *f);
void svg_polyline_begin(FILE *f, int x, int y);
void svg_polyline_point(FILE *f, int x, int y);
void svg_polyline_end(FILE *f);
void svg_text_begin(FILE *f, int x, int y);
void svg_text_character(FILE *f, int c);
void svg_text_end(FILE *f);
