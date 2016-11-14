#include "pm.h"
//#include "shaderCreation.h"
#include "Core\Shader_Loader.h"
#include "glm-0.9.6.3\glm\glm\gtc\type_ptr.hpp"
#include "glm-0.9.6.3\glm\glm\gtc\matrix_transform.hpp"
#include "objParser.h"
using namespace Core;

struct VertexFormat;

GLuint g_program;

//global constant

float g_fzNear = 1.0f;
float g_fzFar = 1000.0f;
ProgramData g_WhitePhong;
const int g_projectionBlockIndex = 2;
GLuint g_projectionUniformBuffer = 0;
PMConstruction *g_pm;
/*
float g_vertexPositions []=  {0.75f, 0.75f, 0.0f, 1.0f,
							0.75f, -0.75f, 0.0f, 1.0f,
							-0.75f, -0.75f, 0.0f, 1.0f
													};
*/
float *g_vertexPositions;
GLuint *g_indices = NULL;
float *g_vertexNormals = NULL;
GLuint g_vao;
GLuint g_vertexBufferObject;
GLuint g_vertexNormalBufferObject;
GLuint g_indexBufferObject;
int g_numVertices = 8;
GLuint g_numTriangles = 8;
std::vector<VertexFormat> *g_vertexPosVector;
std::vector<GLuint> *g_vertexPosVectorIndices;

glm::vec3 g_sphereCamRelPos =  glm::vec3(60.0f, -46.0f, 3.0f);
glm::vec3 g_camTarget = glm::vec3 (0.f, 1.f, 0.f);

int g_mousePos_x = 0;
int g_mousePos_y = 0;
//global constant ends

struct ProjectionBlock
{
	glm::mat4 cameraToClipMatrix;
};


glm::vec3 generateTestNormal (float *vertices, GLuint *indices, int triangleID)
{
	glm::vec3 theVertices[3];
	theVertices[0] = glm::vec3 (vertices[3*indices[triangleID * 3]], vertices[3*indices[triangleID * 3]+1], vertices[3*indices[triangleID * 3]+2] );
	theVertices[1] = glm::vec3 (vertices[3*indices[triangleID * 3 + 1]], vertices[3*indices[triangleID * 3 + 1]+1], vertices[3*indices[triangleID * 3 + 1]+2] );
	theVertices[2] = glm::vec3 (vertices[3*indices[triangleID * 3 + 2]], vertices[3*indices[triangleID * 3 + 2]+1], vertices[3*indices[triangleID * 3 + 2]+2] );
	return getFaceNormal (theVertices);
}

void renderScene(void)
{
	
	glClearColor(0.1, 0.1, 0.8, 1.0);//clear red
	glClearDepth (1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glClear(GL_COLOR_BUFFER_BIT);

	
	glm::mat4 modelToWorldMatrix = glm::mat4 (1.0f);
	glm::vec3 cameraPos = ResolveCamPosition(g_sphereCamRelPos, g_camTarget);
	glm::mat4 worldToCameraMatrix = glm::lookAt (cameraPos, g_camTarget, glm::vec3 (0.f, 1.f, 0.f));
	glm::mat4 modelToCameraMatrix = worldToCameraMatrix * modelToWorldMatrix;
	//glm::vec4 lightPosWorld = glm::vec4 (10.f, 10.f, 10.f, 1.f);
	//glm::vec4 lightPosCamera = worldToCameraMatrix * lightPosWorld;

	//glm::vec4 testPos = glm::vec4 (1.f, 1.f, -1.f, 1.f);
	//testPos = modelToCameraMatrix * testPos;
	
 
	//use the created program
	glUseProgram(g_WhitePhong.theProgram);
	glBindVertexArray (g_vao);
	/*
	glBindBuffer(GL_ARRAY_BUFFER, g_vertexBufferObject);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	*/
	glUniformMatrix4fv (g_WhitePhong.modelToCameraMatrixUnif, 1, GL_FALSE, glm::value_ptr(modelToCameraMatrix));

	//glDrawArrays(GL_TRIANGLES, 0, 3);
	glDrawElements (GL_TRIANGLES, g_pm->renderIndices.size(), GL_UNSIGNED_INT, 0);

	//glDisableVertexAttribArray(0);
	glUseProgram(0);
	glBindVertexArray(0);

	glutSwapBuffers();
	glutPostRedisplay();
	/*
	glUniform4f (g_WhitePhong.lightIntensityUnif, 0.8f, 0.8f, 0.8f, 1.0f);
	glUniform4f (g_WhitePhong.ambientIntensityUnif, 0.2f, 0.2f, 0.2f, 1.0f);
	glUniform3fv (g_WhitePhong.cameraSpaceLightPosUnif, 1, glm::value_ptr(lightPosCamera));
	glUniform1f (g_WhitePhong.lightAttenuationUnif, 1.2f);
	glUniform1f (g_WhitePhong.shininessFactorUnif, 4.0f);
	
	const glm::vec4 lightColor(1.0f);
	glUniform4fv (g_WhitePhong.baseDiffuseColorUnif, 1, glm::value_ptr (lightColor));
	

	glm::mat3 normMatrix (modelToCameraMatrix);
	normMatrix = glm::transpose (glm::inverse (normMatrix));
	*/

	//glUniformMatrix4fv (g_WhitePhong.modelToCameraMatrixUnif, 1, GL_FALSE, glm::value_ptr(modelToCameraMatrix));
	//glUniformMatrix3fv (g_WhitePhong.normalModelToCameraMatrixUnif, 1, GL_FALSE, glm::value_ptr(normMatrix));

	

	//draw 3 vertices as triangles
	//glDrawArrays(GL_TRIANGLES, 0, 3);
	//glDrawElements (GL_TRIANGLES, g_numTriangles, GL_UNSIGNED_INT, 0);

	//glUseProgram(0);
 
	//glutSwapBuffers();
	//glutPostRedisplay();
	int debug0 = 0;
}

void reshape (int w, int h)
{
	//glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	//glutil::MatrixStack persMatrix;
	//persMatrix.Perspective(45.0f, (w / (float)h), g_fzNear, g_fzFar);
	glm::mat4 perspectiveMatrix = glm::perspective (45.0f, (w / (float)h), g_fzNear, g_fzFar);

	ProjectionBlock projData;
	projData.cameraToClipMatrix = perspectiveMatrix;

	/*
	GLfloat perspectiveMatrix[16];
	const float fFrustumScale = 1.0f;
	float fzNear = 1.0f; float fzFar = 35.0f;
	perspectiveMatrix[0] = fFrustumScale;
	perspectiveMatrix[5] = fFrustumScale;
	perspectiveMatrix[10] = (fzFar + fzNear) / (fzNear - fzFar);
	perspectiveMatrix[14] = (2 * fzFar * fzNear) / (fzNear - fzFar);
	perspectiveMatrix[11] = -1.0f;
	*/

	glBindBuffer(GL_UNIFORM_BUFFER, g_projectionUniformBuffer);
	//ProjectionBlock projData;
	//projData.cameraToClipMatrix = perspectiveMatrix;
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ProjectionBlock), &projData);
	//glBufferSubData(GL_UNIFORM_BUFFER, 0, 16*sizeof(GLfloat), &perspectiveMatrix);
	//glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(perspectiveMatrix));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glutPostRedisplay();
	
}

void Init()
{
	glEnable(GL_DEPTH_TEST);
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
 
	//load and compile shaders
	Core::Shader_Loader shaderLoader;
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	g_WhitePhong = shaderLoader.loadProgram("Shaders\\Vertex_Shader.glsl", "Shaders\\Fragment_Shader.glsl", g_projectionBlockIndex);

	glGenBuffers (1, &g_vertexBufferObject);
	glBindBuffer (GL_ARRAY_BUFFER, g_vertexBufferObject);
	//glBufferData (GL_ARRAY_BUFFER, 48, g_vertexPositions, GL_STATIC_DRAW);
	//glBufferData (GL_ARRAY_BUFFER, sizeof(VertexFormat) * g_vertexPosVector->size(), g_vertexPosVector->data(), GL_STATIC_DRAW);
	glBufferData (GL_ARRAY_BUFFER, sizeof(VertexFormat) * g_pm->renderVertices.size(), g_pm->renderVertices.data(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &g_vao);
	glBindVertexArray(g_vao);

	/*
	float perspectiveMatrixUnif = glGetUniformLocation(g_WhitePhong.theProgram, "perspectiveMatrix");
	float perspectiveMatrix[16];
	const float fFrustumScale = 1.0f;

	float fzNear = 1.0f; float fzFar = 3.0f;

	memset(perspectiveMatrix, 0, sizeof(float) * 16);

	perspectiveMatrix[0] = fFrustumScale;
	perspectiveMatrix[5] = fFrustumScale;
	perspectiveMatrix[10] = (fzFar + fzNear) / (fzNear - fzFar);
	perspectiveMatrix[14] = (2 * fzFar * fzNear) / (fzNear - fzFar);
	perspectiveMatrix[11] = -1.0f;

	glUseProgram(g_WhitePhong.theProgram);
	glUniformMatrix4fv(perspectiveMatrixUnif, 1, GL_FALSE, perspectiveMatrix);
	glUseProgram(0);
	*/
	glGenBuffers(1, &g_projectionUniformBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, g_projectionUniformBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
	//Bind the static buffers.
	glBindBufferRange(GL_UNIFORM_BUFFER, g_projectionBlockIndex, g_projectionUniformBuffer, 0, sizeof(glm::mat4));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	/*
	glGenBuffers(1, &g_vertexBufferObject);
	glBindBuffer (GL_ARRAY_BUFFER, g_vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertexPositions), g_vertexPositions, GL_DYNAMIC_DRAW);
	glBindBuffer (GL_ARRAY_BUFFER, 0);

	
	//glGenBuffers(1, &g_vertexNormalBufferObject);
	//glBindBuffer (GL_ARRAY_BUFFER, g_vertexNormalBufferObject);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertexNormals), g_vertexPositions, GL_DYNAMIC_DRAW);
	//glBindBuffer (GL_ARRAY_BUFFER, 0);
	*/

	glGenBuffers(1, &g_indexBufferObject);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_indexBufferObject);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*g_vertexPosVectorIndices->size(), g_vertexPosVectorIndices->data(), GL_DYNAMIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * g_pm->renderIndices.size(), g_pm->renderIndices.data(), GL_DYNAMIC_DRAW);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	//glGenVertexArrays (1, &g_vao);
	//glBindVertexArray (g_vao);

	//size_t normalDataOffset = sizeof(float) * 3 * g_numVertices;
	
	glEnableVertexAttribArray(0);
	//glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, g_vertexBufferObject);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2*sizeof(glm::vec3), 0);
	//glBindBuffer(GL_ARRAY_BUFFER, g_vertexNormalBufferObject);
	//glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer (GL_ARRAY_BUFFER, 0);

	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_indexBufferObject);
	glBindVertexArray(0);
}

void keyboard (unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:
		glutLeaveMainLoop();
		return;
	
	case 32:
		g_pm->collapseOneEdge();
		glBindBuffer (GL_ARRAY_BUFFER, g_vertexBufferObject);
		//glBufferData (GL_ARRAY_BUFFER, 48, g_vertexPositions, GL_STATIC_DRAW);
		//glBufferData (GL_ARRAY_BUFFER, sizeof(VertexFormat) * g_vertexPosVector->size(), g_vertexPosVector->data(), GL_STATIC_DRAW);
		glBufferData (GL_ARRAY_BUFFER, sizeof(VertexFormat) * g_pm->renderVertices.size(), g_pm->renderVertices.data(), GL_STATIC_DRAW);

		glBindVertexArray(g_vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_indexBufferObject);
		//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*g_vertexPosVectorIndices->size(), g_vertexPosVectorIndices->data(), GL_DYNAMIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * g_pm->renderIndices.size(), g_pm->renderIndices.data(), GL_DYNAMIC_DRAW);

		return;
	}
	glutPostRedisplay();
}

void mouseClick (int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)
	{
		g_mousePos_x = x;
		g_mousePos_y = y;
		//g_sphereCamRelPos.x += 1;
	}
	glutPostRedisplay();
}

void mouseMotion (int x, int y)
{
	
	int d_x = x - g_mousePos_x;
	int d_y = y - g_mousePos_y;
	g_sphereCamRelPos.x += d_x;

	if (g_sphereCamRelPos.x >= 360.0)
		g_sphereCamRelPos.x = 0.0f;
	else if (g_sphereCamRelPos.x <= 0.f)
		g_sphereCamRelPos.x = 360.f;

	g_sphereCamRelPos.y -= d_y;

	if (g_sphereCamRelPos.y >= 88.0)
		g_sphereCamRelPos.y = 88.0f;
	else if (g_sphereCamRelPos.y <= -88.f)
		g_sphereCamRelPos.y = -88.f;

	g_mousePos_x = x;
	g_mousePos_y = y;
	
	glutPostRedisplay();
}

int main (int argc, char **argv)
{
	std::vector <glm::vec3> parsedPoints;
	std::vector <int> parsedIndices;
	std::vector <obj_face> parsedObjFaces;
	std::vector <obj_material> parsedMaterials;
	int num_p = 0; //recording the total number of points in the scene
	int num_v = 0; //recording the total number of vertex indices
	int num_tri = 0;  //recording the total number of triangles in the scene

	int success = obj_parse_obj_file("models/bunny.obj", parsedPoints, /*h_n, h_t, */parsedObjFaces, parsedIndices, num_p, num_tri, parsedMaterials);
	std::vector <VertexInfo> testVertices3;
	std::vector <int> &testFaces3 = parsedIndices;
	for (int i=0; i<parsedPoints.size(); i++)
		testVertices3.push_back (VertexInfo (parsedPoints.at(i)*10.f));
	PMConstruction *PMTest0 = new PMConstruction (&testVertices3, &testFaces3);
	g_pm = PMTest0;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);

    glutInitWindowSize(1024, 1024);

    glutCreateWindow("Progressive Bunny!! Press or hold Space to simplify it");
    glewInit();
 
    Init();
 
    // register callbacks
    glutDisplayFunc(renderScene);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc (mouseClick);
	glutMotionFunc(mouseMotion);
    glutMainLoop();
	glDeleteProgram(g_WhitePhong.theProgram);
    return 0;
}

