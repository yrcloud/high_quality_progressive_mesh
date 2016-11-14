#ifndef __OBJ_PARSER__
#define __OBJ_PARSER__

//#include "triangleMesh.h"
#include <vector>
#include "glm-0.9.6.3\glm\glm\glm.hpp"
#include "openGLDependencies\glew\glew.h"
#include "openGLDependencies\freeGLUT\freeglut.h"

#define MAX_VERTEX_COUNT 4
#define OBJ_LINE_SIZE 500
#define WHITESPACE " \t\n\r"

#define OBJ_FILENAME_LENGTH 500
#define MATERIAL_NAME_SIZE 255

typedef struct obj_face
{
	int vertex_index[MAX_VERTEX_COUNT];
	//int normal_index[MAX_VERTEX_COUNT];
	//int texture_index[MAX_VERTEX_COUNT];
	int vertex_count;
	int material_index;
};

char strequal(const char *s1, const char *s2);

char contains(const char *haystack, const char *needle);

int obj_parse_vertex_index(int *vertex_index/*, int *texture_index, int *normal_index*/);

//deal with negative indices 
int obj_convert_to_list_index(int current_max, int index);

void obj_convert_to_list_index_v(int current_max, int *indices);

glm::vec3 obj_parse_vector();
//parse the faces
obj_face* obj_parse_face(int num_vertices, int num_vertex_textures, int num_vertex_normals);

typedef struct obj_material
{
	char name[MATERIAL_NAME_SIZE];
	char texture_filename[OBJ_FILENAME_LENGTH];
	double amb[3];
	double diff[3];
	double spec[3];
	double reflect;
	double refract;
	double trans;
	double shiny;
	double glossy;
	double refract_index;
};

int findMaterialID (std::vector <obj_material> &materials, char *name_to_find);

int obj_parse_obj_file(char *filename, std::vector <glm::vec3> &points, /*float3 *normals, float3 *textures, */std::vector<obj_face> &faces, std::vector<int> &indices,
					   int &countP, int &countTri, /*int &countMesh,*/ /*std::vector<DeviceTriangle> &triangles,*/
					   std::vector <obj_material> &materials);

/*****************************************************materials****************************************************************************/

void obj_set_material_defaults(obj_material *mtl);
int obj_parse_mtl_file(char *filename, std::vector<obj_material> &materials);

#endif