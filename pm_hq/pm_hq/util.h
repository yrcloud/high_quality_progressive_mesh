#ifndef __UTIL_H_INCLUDED__
#define __UTIL_H_INCLUDED__

#define INFINITY 3.402823466e+38F
#define M_PI     3.14159265358979323846f
#define INV_PI     0.31830988618379067154f
#define INV_TWOPI  0.15915494309189533577f

#include <vector>
#include <cmath>
#include "glm-0.9.6.3\glm\glm\glm.hpp"

template <class T>
void assertVectorEmpty (std::vector <T>  &inputVector);

//calculate the normal given the 3 vertices of a triangle
//glm::vec3 getFaceNormalFromMesh (
glm::vec3 getFaceNormal (glm::vec3 *vertices);

float getAlphaSquare (glm::vec3 vertexNormal, const std::vector <glm::vec3> &coveredNormals);

float updateBoundingSphere (glm::vec3 center, float originalR, glm::vec3 centerOtherSphere, float otherR);

//get projection from point p to triangle t, with distance
glm::vec3 getProjection (glm::vec3, glm::vec3 *, float &);

//check if an element is within an array, return the position. Otherwise return -11 as null
int findElementInArray (int *, int target, int length);

/***********************************************************************************************/
//turn degree into radius
float DegToRad(float fAngDeg);

glm::vec3 ResolveCamPosition(const glm::vec3 &sphereCoord, const glm::vec3 &camTarget);

#endif