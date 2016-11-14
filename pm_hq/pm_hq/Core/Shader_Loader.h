//Modified from OpenGL tutorial at http://in2gpu.com/
//Thanks to their awesome tutorial!!!

#ifndef __SHADER_LOADER_H__
#define __SHADER_LOADER_H__
 
#include "..\openGLDependencies\glew\glew.h"
#include "..\openGLDependencies\freeGLUT\freeglut.h"
#include<iostream>
#include<fstream>
#include<vector>



namespace Core
{
	//const int g_projectionBlockIndex = 2;

	struct ProgramData
	{
		GLuint theProgram;

		GLuint modelToCameraMatrixUnif;

		//GLuint lightIntensityUnif;
		//GLuint ambientIntensityUnif;

		//GLuint normalModelToCameraMatrixUnif;
		//GLuint cameraSpaceLightPosUnif;
		//GLuint lightAttenuationUnif;
		//GLuint shininessFactorUnif;
		//GLuint baseDiffuseColorUnif;

		GLuint perspectiveMatrixUnif;
	};

	class Shader_Loader
	{

		private:
 
			std::string ReadShader(char *filename);
			GLuint CreateShader(GLenum shaderType, std::string source, char* shaderName);
 
		public:
 
			Shader_Loader(void);
			~Shader_Loader(void);
			GLuint CreateProgram(char* VertexShaderFilename, char* FragmentShaderFilename);
			ProgramData loadProgram (char* vertexShaderFilename, char* fragmentShaderFilename, int projectionBlockIndex);
 
	};
}

#endif