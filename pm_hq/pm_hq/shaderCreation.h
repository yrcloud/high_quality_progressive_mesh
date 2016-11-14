#ifndef __SHADERCREATION_H__
#define __SHADERCREATION_H__

#include "openGLDependencies\glew\glew.h"
#include "openGLDependencies\freeGLUT\freeglut.h"
#include <algorithm>
#include <string>
#include <vector>
#include <stdio.h>
//#include "openGLDependencies\glsdk_0_5_2\glload\include\glload\gl_3_2_comp.h"


GLuint CreateShader(GLenum eShaderType, const std::string &strShaderFile);

#endif