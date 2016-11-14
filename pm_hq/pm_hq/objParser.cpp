//This code for parsing OBJ models is modified from Github repo https://github.com/stcui/Obj-Loader/blob/master/obj_parser.cpp
//I modified it to work for naive OBJ triangle meshes

#include "objParser.h"

char strequal(const char *s1, const char *s2)
{
	if(strcmp(s1, s2) == 0)
		return 1;
	return 0;
}

char contains(const char *haystack, const char *needle)
{
	if(strstr(haystack, needle) == NULL)
		return 0;
	return 1;
}

int obj_parse_vertex_index(int *vertex_index/*, int *texture_index, int *normal_index*/)
{
	char *temp_str;
	char *token;
	int vertex_count = 0;

	while( (token = strtok(NULL, WHITESPACE)) != NULL)
	{
		/*
		if(texture_index != NULL)
			texture_index[vertex_count] = 0;
		if(normal_index != NULL)
		normal_index[vertex_count] = 0;
		*/

		vertex_index[vertex_count] = atoi( token );
		
		if(contains(token, "//"))  //normal only
		{
			temp_str = strchr(token, '/');
			temp_str++;
			//normal_index[vertex_count] = atoi( ++temp_str );
			++temp_str;
		}
		else if(contains(token, "/"))
		{
			temp_str = strchr(token, '/');
			//texture_index[vertex_count] = atoi( ++temp_str );
			++temp_str;
			if(contains(temp_str, "/"))
			{
				temp_str = strchr(temp_str, '/');
				//normal_index[vertex_count] = atoi( ++temp_str );
				++temp_str;
			}
		}
		
		vertex_count++;
	}

	return vertex_count;
}

//deal with negative indices 
int obj_convert_to_list_index(int current_max, int index)
{
	if(index == 0)  //no index
		return -1;
		
	if(index < 0)  //relative to current list position
		return current_max + index;
		
	return index - 1;  //normal counting index
}

void obj_convert_to_list_index_v(int current_max, int *indices)
{
	for(int i=0; i<MAX_VERTEX_COUNT; i++)
		indices[i] = obj_convert_to_list_index(current_max, indices[i]);
}

glm::vec3 obj_parse_vector()
{
	//float3 v = make_float3(0.0f);
	glm::vec3 v = glm::vec3 (0.f);
	v.x = atof( strtok(NULL, WHITESPACE));
	v.y = atof( strtok(NULL, WHITESPACE));
	v.z = atof( strtok(NULL, WHITESPACE));
	return v;
}

//parse the faces
obj_face* obj_parse_face(int num_vertices, int num_vertex_textures, int num_vertex_normals)
{
	int vertex_count;
	//obj_face *face = (obj_face*)malloc(sizeof(obj_face));
    obj_face *face = new obj_face;
	
	vertex_count = obj_parse_vertex_index(face->vertex_index/*, face->texture_index, face->normal_index*/);
	obj_convert_to_list_index_v(num_vertices, face->vertex_index);
	//obj_convert_to_list_index_v(num_vertex_textures, face->texture_index);
	//obj_convert_to_list_index_v(num_vertex_normals, face->normal_index);
	face->vertex_count = vertex_count;

	return face;
}

int findMaterialID (std::vector <obj_material> &materials, char *name_to_find)
{
	int i = 0;
	for(i=0; i < materials.size(); i++)
	{
		if(strncmp(materials[i].name , name_to_find, strlen(name_to_find)) == 0)
			return i;
	}
	return -1;
}

int obj_parse_obj_file(char *filename, std::vector <glm::vec3> &points, /*float3 *normals, float3 *textures, */std::vector<obj_face> &faces, std::vector<int> &indices,
					   int &countP, int &countTri, /*int &countMesh,*/ /*std::vector<DeviceTriangle> &triangles,*/
					   std::vector <obj_material> &materials)
{
	FILE* obj_file_stream;
	int current_material = -1; 
	char *current_token = NULL;
	char current_line[OBJ_LINE_SIZE];
	int line_number = 0;
	bool newGroup = false;
	//DeviceTriangle tempTriangle;

	//testing 
	//vector <DeviceTriangle> testTriangleVector;
	int triangleIndex = 0;

	//DeviceTriangleMesh *temp_mesh;
	int num_points = 0, num_vertex_textures = 0, num_vertex_normals = 0, num_faces = 0, num_g = 0, num_tri = 0;
	int num_meshTri = 0, num_meshP = 0;
	// open scene
	obj_file_stream = fopen( filename, "r");
	if(obj_file_stream == 0)
	{
		fprintf(stderr, "Error reading file: %s\n", filename);
		return 0;
	}

	//parser loop
	while( fgets(current_line, OBJ_LINE_SIZE, obj_file_stream) )
	{
		current_token = strtok( current_line, " \t\n\r");
		line_number++;
		//printf ("line %d\n", line_number);
		
		//skip comments
		if( current_token == NULL || current_token[0] == '#')
			continue;

		//parse objects
		else if( strequal(current_token, "v") ) //process vertex
		{
			if (newGroup == false)
			{
				newGroup = true;
				printf ("group %d\n", num_g);	  
				/*
				if (num_g>0) 
				{
				    temp_mesh = new DeviceTriangleMesh(num_g-1,num_tri*3,num_points,num_meshTri,num_meshP,make_float3(0.f,0.f,0.f),make_float3(1.0f,1.0f,1.0f));
				    all_meshes.push_back (temp_mesh);
				}
				*/
				//num_meshTri = 0;
				//num_meshP = 0;
				num_g++;
			}
			//points[num_points] = obj_parse_vector();
			points.push_back (obj_parse_vector());
			num_points++;
			//num_meshP++;
		}
		/*
		else if( strequal(current_token, "vn") ) //process vertex normal
		{
			normals[num_points] = obj_parse_vector();
			num_vertex_normals++;
		}
		
		else if( strequal(current_token, "vt") ) //process vertex texture
		{
			textures[num_vertex_textures] = obj_parse_vector();
			num_vertex_textures++;
		}
		*/
		else if( strequal(current_token, "f") ) //process face
		{
			if (newGroup == true)
				newGroup = false;
			obj_face *face = obj_parse_face(num_points, num_vertex_textures, num_vertex_normals);
			face->material_index = current_material;
			//faces[num_faces] = *face;
			faces.push_back (*face);
			num_faces++;
			//display
			/*
			int v0 = face->vertex_index[0], v1 = face->vertex_index[1], v2 = face->vertex_index[2];
			printf ("face %d is %d/%d/%d , namely %f,%f,%f / %f,%f,%f / %f,%f,%f\n", 
				     num_faces-1, v0, v1, v2, 
					 points[v0].x, points[v0].y, points[v0].z, points[v1].x, points[v1].y, points[v1].z, points[v2].x, points[v2].y, points[v2].z);
					 */
			if (face->vertex_count == 3 )
			{
				indices.push_back (face->vertex_index[0]);
				indices.push_back (face->vertex_index[1]);
				indices.push_back (face->vertex_index[2]);
				//num_meshTri++;
				//testing
				//DeviceTriangle tempTriangle = DeviceTriangle(num_g, triangleIndex);
				//tempTriangle = DeviceTriangle(num_g, triangleIndex, current_material);
				//triangles.push_back (tempTriangle);
				triangleIndex++;
				num_tri++;
			}
			else if (face->vertex_count == 4)
			{
				//first triangle
				indices.push_back (face->vertex_index[0]);
				indices.push_back (face->vertex_index[1]);
				indices.push_back (face->vertex_index[3]);
				//DeviceTriangle tempTriangle0 = DeviceTriangle(num_g, triangleIndex);
				//tempTriangle = DeviceTriangle(num_g, triangleIndex, current_material);
				//triangles.push_back (tempTriangle);
				triangleIndex++;

				//second triangle
				indices.push_back (face->vertex_index[1]);
				indices.push_back (face->vertex_index[2]);
				indices.push_back (face->vertex_index[3]);
				//DeviceTriangle tempTriangle1 = DeviceTriangle(num_g, triangleIndex);
				//tempTriangle = DeviceTriangle(num_g, triangleIndex, current_material);
				//triangles.push_back (tempTriangle);
				triangleIndex++;

				num_tri += 2;
				//num_meshTri += 2;
			}
			else
				printf ("neither a quad nor a triangle");
		}
		
		else if( strequal(current_token, "g") ) // group
		{ 

		}		

		else if( strequal(current_token, "usemtl") ) // usemtl
		{
			current_material = findMaterialID (materials, strtok(NULL, WHITESPACE));
		}
		else
		{/*
			printf("Unknown command '%s' in scene code at line %i: \"%s\".\n",
					current_token, line_number, current_line);
					*/
		}
	}
	countP = num_points; countTri = num_tri; //countMesh = 0;

	fclose(obj_file_stream);
	
	return 1;
}

/*****************************************************materials****************************************************************************/

void obj_set_material_defaults(obj_material *mtl)
{
	mtl->amb[0] = 0.2;
	mtl->amb[1] = 0.2;
	mtl->amb[2] = 0.2;
	mtl->diff[0] = 0.8;
	mtl->diff[1] = 0.8;
	mtl->diff[2] = 0.8;
	mtl->spec[0] = 1.0;
	mtl->spec[1] = 1.0;
	mtl->spec[2] = 1.0;
	mtl->reflect = 0.0;
	mtl->trans = 1;
	mtl->glossy = 98;
	mtl->shiny = 0;
	mtl->refract_index = 1;
	mtl->texture_filename[0] = '\0';
}

int obj_parse_mtl_file(char *filename, std::vector<obj_material> &materials)
{
	int line_number = 0;
	char *current_token;
	char current_line[OBJ_LINE_SIZE];
	char material_open = 0;
	obj_material *current_mtl = NULL;
	FILE *mtl_file_stream;

	int materialCount = 0;

	// open scene
	mtl_file_stream = fopen( filename, "r");
	if(mtl_file_stream == 0)
	{
		fprintf(stderr, "Error reading file: %s\n", filename);
		return 0;
	}

	while( fgets(current_line, OBJ_LINE_SIZE, mtl_file_stream) )
	{
		current_token = strtok( current_line, " \t\n\r");
		line_number++;

		//skip comments
		if( current_token == NULL || strequal(current_token, "//") || strequal(current_token, "#"))
			continue;

		//start material
		else if( strequal(current_token, "newmtl"))
		{
			materialCount++;
			//push the previous material in the vector
			if (materialCount != 1)
			{
				materials.push_back (*current_mtl);
			}

			material_open = 1;
			current_mtl = (obj_material*) malloc(sizeof(obj_material));
			obj_set_material_defaults(current_mtl);

			// get the name
			strncpy(current_mtl->name, strtok(NULL, " \t"), MATERIAL_NAME_SIZE);
		}
		//ambient
		else if( strequal(current_token, "Ka") && material_open)
		{
			current_mtl->amb[0] = atof( strtok(NULL, " \t"));
			current_mtl->amb[1] = atof( strtok(NULL, " \t"));
			current_mtl->amb[2] = atof( strtok(NULL, " \t"));
		}

		//diff
		else if( strequal(current_token, "Kd") && material_open)
		{
			current_mtl->diff[0] = atof( strtok(NULL, " \t"));
			current_mtl->diff[1] = atof( strtok(NULL, " \t"));
			current_mtl->diff[2] = atof( strtok(NULL, " \t"));
		}
		
		//specular
		else if( strequal(current_token, "Ks") && material_open)
		{
			current_mtl->spec[0] = atof( strtok(NULL, " \t"));
			current_mtl->spec[1] = atof( strtok(NULL, " \t"));
			current_mtl->spec[2] = atof( strtok(NULL, " \t"));
		}
		//shiny
		else if( strequal(current_token, "Ns") && material_open)
		{
			current_mtl->shiny = atof( strtok(NULL, " \t"));
		}
		//transparent
		else if( strequal(current_token, "d") && material_open)
		{
			current_mtl->trans = atof( strtok(NULL, " \t"));
		}
		//reflection
		else if( strequal(current_token, "r") && material_open)
		{
			current_mtl->reflect = atof( strtok(NULL, " \t"));
		}
		//glossy
		else if( strequal(current_token, "sharpness") && material_open)
		{
			current_mtl->glossy = atof( strtok(NULL, " \t"));
		}
		//refract index
		else if( strequal(current_token, "Ni") && material_open)
		{
			current_mtl->refract_index = atof( strtok(NULL, " \t"));
		}
		// illumination type
		else if( strequal(current_token, "illum") && material_open)
		{
		}
		// texture map
		else if( strequal(current_token, "map_Ka") && material_open)
		{
			strncpy(current_mtl->texture_filename, strtok(NULL, " \t"), OBJ_FILENAME_LENGTH);
		}
		else
		{
			fprintf(stderr, "Unknown command '%s' in material file %s at line %i:\n\t%s\n",
					current_token, filename, line_number, current_line);
			//return 0;
		}
	}
	//push the last material in
	materials.push_back (*current_mtl);
	materialCount++;

	fclose(mtl_file_stream);

	return 1;
}
