
#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb\stb_image.h"


{
	int x, y, comp;
	unsigned char *data = stbi_load("lena.png", &x, &y, &comp, 4);


	glGenTextures(1, &lenaTexture);
	glBindTexture(GL_TEXTURE_2D, lenaTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

{
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, lenaTexture);
}