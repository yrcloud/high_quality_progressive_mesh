#version 430 core
//#version 330

layout(location = 0) in vec3 position;
//layout(location = 0) in vec3 position;
//layout(location = 1) in vec4 inDiffuseColor;
//layout(location = 2) in vec3 normal;

//out vec4 diffuseColor;
//out vec3 vertexNormal;
//out vec3 cameraSpacePosition;

uniform mat4 modelToCameraMatrix;
//uniform mat4 perspectiveMatrix;
//uniform mat3 normalModelToCameraMatrix;

layout(std140) uniform projection
{
	mat4 cameraToClipMatrix;
};

void main()
{
	vec4 tempCamPosition = modelToCameraMatrix * vec4(position, 1.0);
	//gl_Position = cameraToClipMatrix * tempCamPosition;
	//gl_Position = perspectiveMatrix * tempCamPosition;

	//vertexNormal = normalModelToCameraMatrix * normal;
	//diffuseColor = inDiffuseColor;
	//diffuseColor = vec3 (1.0);
	//cameraSpacePosition = vec3(tempCamPosition);


	//gl_Position = vec4 (position, 1.0);
	gl_Position = cameraToClipMatrix * tempCamPosition;
}
