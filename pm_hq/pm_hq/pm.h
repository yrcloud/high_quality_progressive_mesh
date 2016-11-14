#ifndef __PM_H_INCLUDED__
#define __PM_H_INCLUDED__

#include "glm-0.9.6.3\glm\glm\glm.hpp"
#include <vector>
#include <iostream>
#include <queue>
#include "util.h"

#include "openGLDependencies\glew\glew.h"
#include "openGLDependencies\freeGLUT\freeglut.h"

//vertex information for rendering purpose
struct VertexFormat
{
	glm::vec3 position;//our first vertex attribute
	glm::vec3 normal;
 
	VertexFormat(const glm::vec3 &pos, const glm::vec3 &norm)
	{
		position = pos;
		normal = norm;
	}
};

/////////////////////////////////////the vertex tree///////////////////////////////////////////////////////////////////////////////////////////
//first, let's define each element of the vertex tree structure
struct VertexTreeNode 
{
	int descendentLeafInInputVertices;  //the index of the leaf node IN INPUT_VERTICES, the vt* in the paper
	int vp;  //the index of the parent in the tree structure, the vp in the paer
	int vtvu[2];  //the index of vt, vu. vtvu[0] is the vertex with the same attributes as this new (parent) vertex

	//the normal of this vertex itself
	glm::vec3 normal;

	//information about the surrounding area of the vertex, these are used during the construction of the tree
	std::vector <int> neighborVertices;  //indices of the neighboring vertices AMONG THE CURRENT TREE in the current step of construction
	std::vector <int> neighborFaces;  //indices TO THE ORIGINAL FACE ARRAY

	//the rule 1 for bounding sphere
	float boundingSphereRadius;
	//the rule 2 for normals
	std::vector <glm::vec3> coveredNormals;
	//float normalConeAlpha;
	float sinAlphaSquare;
	//the rule 3 for screen space error
	float muSquare;
	float deltaSquare;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////the face (triangle mesh) structure for the current step of the construction/////////////////////////////////////////
//the element that record one sampled point on the original geometry, along with the according distance to the currently projected triangle
struct SampledPointAndDist
{
	glm::vec3 point;
	float dist;
	glm::vec3 projection;
	SampledPointAndDist (glm::vec3, float, glm::vec3);
};
//typedef std::pair <int, std::vector<SampledPointAndDist>> cacheNewFaceData;

//one new face for candidate collapse
struct PotentialNewFaceAfterCollapsing
{
	int faceIndexInCurrentMesh;          //which face this face is in the original mesh
	int newVertexIndicesInCurrentMesh[3];   //new v0, v1, v2 in this new face after the collapsing
	std::vector <SampledPointAndDist> projectedSamplePoints;   //new sample points that will be projected to this face after collapsing
	float totalDistSamplePoints;         //total distance from the above sample points
	bool shapeChanged;  //documenting whether one of the vertices in this triangle has moved to a new position, namely whether it has vu
};

//element documenting each edge collapse operation and corresponding new sub-mesh
struct CandidateCollapsingWithNewSubMesh
{
	//info to help locate which edge is collapsed and how
	//int triangleIndex;    //which triangle this edge collapse is associated to
	//short edgeIndex;      //0, 1 or 2, as which edge within this triangle is being collapsed
	bool edgeTestedElseWhere;
	bool vtOrVu;          //which end of the edge is the new vertex, 0 for first and 1 for second.
	int vtvu[2];          //[0] is vt and [1] is vu
	int vlvr[2];
	int flfr[2];

	//info for the sub-mesh involved
	std::vector <PotentialNewFaceAfterCollapsing> newSubMesh;   //list of new faces that will be formed after this edge collapse
	float totalDistSamplePoints;  //sum of distance from all points to all the faces included in this submesh	
};

//each node represent one triangle in the current mesh. Every node contains an array of SampledPointAndDist, representing the sampled points projected to this current triangle
struct CurrentMeshNode
{
	int vertexIndices[3]; //the vertex indices of the three vertices in this triangle, in the current vertex tree. It should be updated in every step of building the tree
	int neighborFaces[3]; //the faces that are adjacent to each edge of this triangle, in the order of edge v0-v1, edge v1-v2, edge v2-v0
	std::vector <SampledPointAndDist> projectedSamplePoints;
	float totalDistSamplePoints;  //sum of distance from all the points projected to this one single face
	//bool edgeTested[3];    //to document if the edge has been tested during the consideration of the collapsing of edges
	CandidateCollapsingWithNewSubMesh candidateCollapses[3];
};

//structure that documents edges, mainly for identifying which edge is being collapse and which edge's candidate collapsing needs to be updated
struct Edge
{
	int triangleIndex;  //in currentMesh
	int edgeIndex;  //0 to 2
	Edge (int t, int e);
};

inline bool operator== (const Edge & lhs, const Edge & rhs);
inline bool operator!= (const Edge & lhs, const Edge & rhs);
//typedef std::pair <int, CurrentMeshNode> CacheNewFaceData;  //data structure to record one new face after collapsing



//comparision for the priority queue containing all the CandidateCollapsingWithNewSubMesh
/*
class CompareCollapsing {
public:
    bool operator()(CandidateCollapsingWithNewSubMesh& t1, CandidateCollapsingWithNewSubMesh& t2)   //if t1 < t2 in the priority, this should return true
    {
	   if (t1.totalDistSamplePoints > t2.totalDistSamplePoints) return true;   //if t1 < t2 in the priority, t2 has larger total distance than t1
       return false;
    }
};
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////the ordered original faces//////////////////////////////////////////////////////////////////////////////////////////////
//the node for the ordered original faces, corresponding to the order of the vertexTree
struct OrderedOriginalFacesNode
{
	int faceIndex; //the index pointing to the unordered, original input face array
	int fns[4];  //the adjacent faces when the collapsing happened, in the order of fn0, fn1, fn2, fn3. Indices pointing to the unordered, original input face array
	int vlrMax[2];  //the vlmax and vrmax
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////the information of an individual, original vertex. Now it only has coordinates, but we could put in normals and texture coordinates later
struct VertexInfo
{
	glm::vec3 pos;  //the x, y, z position of the vertex
	//constructor
	VertexInfo (glm::vec3 p);
};

//structure to cache the result of edge collapse
//documenting the new submesh (list of faces, the corresponding list of sample points and distance for each face)

class PMConstruction
{
private:
	//private data
	std::vector <VertexInfo> inputVertices;   //the original unordered input vertices, which have the ultimate data of the coordinates
	std::vector <int> inputFaces;  //the original indices pointing to the inputVertices
	std::vector <VertexTreeNode> vertexTree; //the current vertex tree, updated in every step of the pm construction
	std::vector <CurrentMeshNode> currentMesh;  //the current triangle mesh
	std::vector <OrderedOriginalFacesNode> orderedOriginalFaces;  //the resulting array of the orginal faces with order matching the current vertextTree
	//std::priority_queue <CandidateCollapsingWithNewSubMesh> candidateCollapsing;
public:

	//the actual construction process, which uses several functions below
	PMConstruction (const std::vector <VertexInfo> *, const std::vector <int> *);  
	void collapseOneEdge(); 

	//for rendering purpose
	std::vector <VertexFormat> renderVertices;
	std::vector <GLuint> renderIndices;

private:
	//initialize inputVertices, inputFaces and vertexTree and currentMesh without candidate collapsing info 
	//These are two function being called by the constructor of PMConstruction
	void initTriangleMesh (const std::vector <VertexInfo> *, const std::vector <int> *); //initialize the geometry
	void initFirstCollapse ();  //initialize the candidate collapsing and related sub-mesh info
	void initRenderingData();   //initialize the rendering vertices and indices
	void updateCandidateCollapse (Edge theEdge);

	//sub-functions called by collapseOneEdge()
	//first, collapsing the geometry and update the info of bounding sphere, normal and screen space error
	void updtGeoInfoColOneEdge();
	//next, update the info related to further collapsing
	void updtCandidColInfoColOneEdge(const CandidateCollapsingWithNewSubMesh &);
	//next, update the rendering info as the current mesh
	void updtRenderData ();

	//create one vertexTreeNode for the proposed collapsing
	VertexTreeNode constructVertexTreeNode (int ,int );
	void updateMeshForNewVertexNode (const CandidateCollapsingWithNewSubMesh &);

	//some utility functions
	void initNormals(VertexTreeNode &);
	void PMConstruction::calNormal (VertexTreeNode &vertex);
	glm::vec3 calAvgNormal (const CandidateCollapsingWithNewSubMesh &);
	void getSSErrorInfo (const CandidateCollapsingWithNewSubMesh &, float &, float &, glm::vec3);
	//void calVertexNormal (VertexTreeNode &);
};

#endif