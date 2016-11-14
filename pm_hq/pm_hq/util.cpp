#include "util.h"
#include <iostream>

#include "pm.h"

template <class T>
void assertVectorEmpty (std::vector <T>  &inputVector)
{
	if (inputVector.size() != 0)
		std::cout << "the current vector is not initialized to empty";
}
template void assertVectorEmpty (std::vector <VertexTreeNode>  &inputVector);
template void assertVectorEmpty (std::vector <CurrentMeshNode>  &inputVector);
template void assertVectorEmpty (std::vector <SampledPointAndDist>  &inputVector);
template void assertVectorEmpty (std::vector <int>  &inputVector);
template void assertVectorEmpty (std::vector <glm::vec3>  &inputVector);

//get the normal for a given triangle
glm::vec3 getFaceNormal (glm::vec3 *vertices)
{
	glm::vec3 e0 = vertices[1] - vertices[0];
	e0 = glm::normalize (e0);
	glm::vec3 e1 = vertices[2] - vertices[1];
	e1 = glm::normalize (e1);
	glm::vec3 resultNormal = glm::cross (e0, e1);
	resultNormal = glm::normalize (resultNormal);
	return resultNormal;
}

float getAlphaSquare (glm::vec3 vertexNormal, const std::vector <glm::vec3> &coveredNormals)
{
	float sinAlphaSquare = 0.f;
	for (int j=0; j<coveredNormals.size(); j++)
	{
		//if there is no alpha < 90 degree that bounds the cone
		if ( glm::dot (vertexNormal, coveredNormals.at(j)) < 0.f)
		{
			sinAlphaSquare = 1.f;
			break;
		}
		float currentSinAlphaSquare = glm::length (glm::cross (vertexNormal, coveredNormals.at(j)));
		currentSinAlphaSquare *= currentSinAlphaSquare;
		if (currentSinAlphaSquare > sinAlphaSquare)
			sinAlphaSquare = currentSinAlphaSquare;
	}
	return sinAlphaSquare;
}

float updateBoundingSphere (glm::vec3 center, float originalR, glm::vec3 centerOtherSphere, float otherR)
{
	float distTwoCenters = glm::distance (center, centerOtherSphere);
	if (distTwoCenters + otherR > originalR)
		return (distTwoCenters + otherR);
	else
		return originalR;
}

glm::vec3 getProjection (glm::vec3 p, glm::vec3 *triangle, float &resultDistance)
{
	//get the B, E0 and E1 for the Baricentric form of T(s,t) = B + s E0 + t E1
	glm::vec3 B = triangle[0];

	glm::vec3 E0 = triangle[1] - triangle[0];
	glm::vec3 E1 = triangle[2] - triangle[0];

	//get the parameter for the distance expression
	short regionID = -1;
	float a = glm::dot (E0, E0);
	float b = glm::dot (E0, E1);
	float c = glm::dot (E1, E1);
	float d = glm::dot (E0, B-p);
	float e = glm::dot (E1, B-p);
	float f = glm::dot (B-p, B-p);

	//first decide which region the minimum falls into
	float det = a * c - b * b, s = b * e - c * d, t = b * d - a * e;
	if (s + t <= det)   //test the line s+t = 0
	{
		if ( s < 0.f )
			if (t<0.f)
				regionID = 4;
			else
				regionID = 3;
		else if (t<0.f)
			regionID = 5;
		else
			regionID = 0;
	}
	else
	{
		if (s<0.f)
			regionID = 2;
		else if (t<0.f)
			regionID = 6;
		else
			regionID = 1;
	}

	//debug
	//std::cout << "result region in triangle is " << regionID << "\n";

	//test if region ID is successfully assigned
	if (regionID < 0)
		std::cout << "region ID is not successfully assigned\n";
	//cache floats used for more than once
	float numer = 0.f;   
	float denom = 0.f;   
	float tmp0 = 0.f;
	float tmp1 = 0.f;

	switch (regionID)
	{
	//case 0
	case 0:
	{
		float invDet = 1.f/det;
		s *= invDet;
		t *= invDet;
		break;
	}

	//case 1
	case 1:
		numer = c+e-b-d;
		if (numer <= 0.f)
			s=0.f;
		else
		{
			denom = a-2*b+c;
			s = (numer >= denom)? 1.f : (numer/denom);
		}
		t = 1-s;
		break;

    //case 3 and 5 
	case 3:
		s = 0.f;
		t = (e >= 0.f) ? 0.f : ( (-e>=c) ? 1.f : -e/c );
		break;	    
	case 5:
		//F(s,0) = a s^2 + 2ds + f
		//F'(s,0)/2 = as + d
		//F'(s,0) == 0  =>  s = -d/a
		t = 0.f;
		s = (d >= 0.f) ? 0.f : ( (-d>=a) ? 1.f : -d/a );
		break;

	//case 2, 4 and 6
	case 2:
		tmp0 = b+d;
		tmp1 = c+e;
		if (tmp1 > tmp0)  //minimum on edge s+t=1
		{
			numer = tmp1 - tmp0;
			denom = a-2*b+c;
			s = (numer >= denom) ? 1.f : (numer/denom);
			t = 1-s;
		}
		else //minimum on edge s=0
		{
			s = 0.f;
			//  (-e/c > 1 ) ? 1.f : ( (-e/c < 0) ? 0.f : -e/c);
			t = (tmp1 <= 0.f) ? 1.f : ( (e>=0.f) ? 0.f : -e/c);
		}
		break;

	case 6:
		//Grad(Q) = 2(as+bt+d, bs+ct+e)
		//(-1, 0) dot Grad(Q(1,0)) = (-1,0) dot (2(a+d), 2(b+e)) = -2(a+d)
		//(-1, 1) dot Grad(Q(1,0)) = (-1,1) dot (2(a+d), 2(b+e)) = 2[(b+e)-(a+d)]
		//min on edge s+t=1 if (-1, 1) dot Grad(Q(1,0)) < 0
		//min on edge t = 0 otherwise
		tmp0 = b+e;
		tmp1 = a+d;
		if (tmp1 > tmp0)  //minimum on edge s+t = 1
		{
		    //Q'(1-t,t)/2 = (a-2b+c)t + (b-a+e-d)
			//Q' = 0 when T = (a+d-b-e)/(a-2b+c)
			// a-2b+c > 0, and now a+d-b-e ? 0 as well
			numer = tmp1-tmp0;
			denom = a-2*b+c;
			t = (numer >= denom) ? 1.f : (numer/denom);
			s = 1-t;
		}
		else //minimum on edge t=0
		{
			t = 0.f;
			s = (tmp1 <= 0.f) ? 1.f : ( (d>=0.f) ? 0.f : -d/a);
		}
		break;

	case 4:
		//Grad(Q) = 2(as+bt+d, bs+ct+e)
		//(1, 0) dot Grad(Q(0,0)) = (1,0) dot (2d, 2e) = 2d
		//(0, 1) dot Grad(Q(0,0)) = (0,1) dot (2d, 2e) = 2e
		//min on edge s=0 if (0, 1) dot Grad(Q(0,0)) < 0
		//min on edge t=0 otherwise
		if (e < 0.f)   //on edge s=0, following region 3
		{
			s = 0.f;
			t = (-e>=c)? 1.f:-e/c;
		}
		else           //on edge t=0, following region 5
		{
			t = 0.f;
			s = (d >= 0.f) ? 0.f : ( (-d>=a) ? 1.f : -d/a );
		}
		break;
	default:
		std::cout << "No region matches the current triangle for projection calculation\n";
	}	

	glm::vec3 resultProjection = B + E0 * s + E1 * t;
	resultDistance = glm::distance (p, resultProjection);
	return resultProjection;
}

int findElementInArray (int *theArray, int target, int length)
{
	for (int i=0; i<length; i++)
		if (theArray[i] == target)
			return i;
	return -11;
}

/*****************************************graphics utilities*************************************/

float DegToRad(float fAngDeg)
{
	const float fDegToRad = 3.14159f * 2.0f / 360.0f;
	return fAngDeg * fDegToRad;
}

glm::vec3 ResolveCamPosition(const glm::vec3 &sphereCoord, const glm::vec3 &camTarget)
{
	float phi = DegToRad(sphereCoord.x);
	float theta = DegToRad(sphereCoord.y + 90.0f);

	float fSinTheta = sinf(theta);
	float fCosTheta = cosf(theta);
	float fCosPhi = cosf(phi);
	float fSinPhi = sinf(phi);

	glm::vec3 dirToCamera(fSinTheta * fCosPhi, fCosTheta, fSinTheta * fSinPhi);
	return (dirToCamera * sphereCoord.z) + camTarget;
}