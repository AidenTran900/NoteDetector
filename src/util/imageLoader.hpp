#ifndef IMAGE_LOADER_HPP
#define IMAGE_LOADER_HPP

#include <GLFW/glfw3.h>

GLuint LoadTextureFromFile(const char* filename, int* out_width, int* out_height);

#endif
