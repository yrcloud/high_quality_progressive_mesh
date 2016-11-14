#include "pm.h"

inline bool operator== (const Edge & lhs, const Edge & rhs)
{
	return lhs.edgeIndex == rhs.edgeIndex && lhs.triangleIndex == rhs.triangleIndex;
}
inline bool operator!= (const Edge & lhs, const Edge & rhs)
{
	return !(lhs==rhs);
}

SampledPointAndDist::SampledPointAndDist(glm::vec3 p, float d, glm::vec3 proj): point(p), dist(d),projection (proj)  {}

VertexInfo::VertexInfo (glm::vec3 p): pos(p) {}

Edge::Edge (int t, int e): triangleIndex(t), edgeIndex(e) {}

PMConstruction::PMConstruction (const std::vector <VertexInfo> *theInputVertices, const std::vector <int> *theInputFaces)
{
	initTriangleMesh (theInputVertices, theInputFaces);
	initFirstCollapse();
	initRenderingData();
}

void PMConstruction::initTriangleMesh (const std::vector <VertexInfo> *theInputVertices, const std::vector <int> *theInputFaces)
{	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////initialization of the original input mesh starts here////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//first initialize the "input"(original) parts to the original mesh
	inputVertices = *theInputVertices;
	inputFaces = *theInputFaces;
	if (inputFaces.size() % 3 != 0)
		std::cout << "the input face size is not a multiplication of 3" << '\n';

	//then initialize the vertexTree and currentMesh to contain only the original input mesh
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////initialize vertexTree starts here//////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	assertVectorEmpty (vertexTree);   //see if vertexTree is successfully initialized to empty
	//process each vertex one by one
	for (int i=0; i<inputVertices.size(); i++)
	{
		//first cache the position of the current vertex
		glm::vec3 posCurrentVertex = inputVertices.at(i).pos;

		////////////////////////get the basic tree information for the initial original inpu mesh/////////////////////////////////// 
		VertexTreeNode newVertexTreeNode;   //the node being constructed, containing information of one vertex in the original mesh
		newVertexTreeNode.descendentLeafInInputVertices = i;  //since this is the leaf node, the leaf node is itself, the same index in inputVertices
		newVertexTreeNode.vp = -11;  //since this is undecided, -11 for undecided
		newVertexTreeNode.vtvu[0] = -10;    newVertexTreeNode.vtvu[1] = -10;   //since this is the leaf node, -10 for null
		////////////////////////get the basic tree information for the initial original inpu mesh ends/////////////////////////////////// 

		/////////////////////////get the adjacent vertices and faces for the initial original input mesh///////////////////////////////////////
		for (int j=0; j<(inputFaces.size()/3); j++)  //traverse the entire list of input original faces, j represent the "jth" face
		{
			int verticesInCurrentFace[3];  //store the 3 vertex indices of the current face being processed
			for (int k=0; k < 3; k++)
				verticesInCurrentFace [k] = inputFaces [3*j+k];
			int numAppearanceSameVertexInSameTriangle = 0;  //number of appearance of i in this same triangle, report error if it's larger than 1
			for (int k=0; k < 3; k++)
			{
				if (verticesInCurrentFace[k] == i) //if one vertex in the current face being processed matches the vertex we are processing in the outer loop
					                               //this if should only be fulfilled once, in k between 0 and 3, otherwise i appeared more than once in the same triangle
				{
					numAppearanceSameVertexInSameTriangle ++;  //this is to check if i only appeared once, otherwise report error

					//assert if current triangle is already in "neighborFaces", probably due to repetition of faces in the mesh
					if ( std::find (newVertexTreeNode.neighborFaces.begin(), newVertexTreeNode.neighborFaces.end(), j) != newVertexTreeNode.neighborFaces.end() )
						std::cout << "the same face is already in neighborFaces" << '\n';
					//push back the current triangle as one neighborFace
					else
					    newVertexTreeNode.neighborFaces.push_back (j);

					//push back the other two vertices in the current face as adjacent vertices
					for (int l=0; l<3; l++)
					{
						int adjacentVertex = verticesInCurrentFace[l];
						//push back a vertex only if it's not already contained (found) in the neighborVertices array and it's not i
						if ( std::find (newVertexTreeNode.neighborVertices.begin(), newVertexTreeNode.neighborVertices.end(), adjacentVertex) == newVertexTreeNode.neighborVertices.end() &&
							 adjacentVertex != i)
							newVertexTreeNode.neighborVertices.push_back (adjacentVertex);   //push back the adjacent vertex, if it's not already in the "neighborVertices" list or i
					}
				}

				//report error if i appeared more than once in the same triangle
				if (numAppearanceSameVertexInSameTriangle > 1)
					std::cout << "the same vertex appeared more than once in the same triangle:" << j <<"th triangle" << '\n';
			}
		}
		/////////////////////////get the adjacent vertices and faces for the initial original input mesh ends///////////////////////////////////////

		////////////////////////get the normals for the initial original input mesh/////////////////////////////////////////////////////////////////
		/////////////////////// each vertex's normal is the average of the normals of the surrounding neighbor faces//////////////////////////////////
		initNormals (newVertexTreeNode);
		////////////////////////get the normals for the initial original input mesh ends/////////////////////////////////////////////////////////////////
		
		////////////////////////get the bounding sphere for rule 1///////////////////////////////////////////////////////////////////////////////////////
		float boundingSphereRadius = 0.f;
		for (int j=0; j<newVertexTreeNode.neighborVertices.size(); j++)
		{
			glm::vec3 currentNeighborVertexPos = inputVertices[newVertexTreeNode.neighborVertices.at(j)].pos;
			float currentDistance = glm::distance (posCurrentVertex, currentNeighborVertexPos);
			if ( currentDistance > boundingSphereRadius)
				boundingSphereRadius = currentDistance;
		}
		newVertexTreeNode.boundingSphereRadius = boundingSphereRadius;
		////////////////////////get the bounding sphere for rule 1 ends///////////////////////////////////////////////////////////////////////////////////

		///////////////////////get alpha for rule 2///////////////////////////////////////////////////////////////////////////////////////////////////////
		/*
		float sinAlphaSquare = 0.f;
		for (int j=0; j<newVertexTreeNode.coveredNormals.size(); j++)
		{
			//if there is no alpha < 90 degree that bounds the cone
			if ( glm::dot (newVertexTreeNode.normal, newVertexTreeNode.coveredNormals.at(j)) < 0.f)
			{
				sinAlphaSquare = 1.f;
				break;
			}
			float currentSinAlphaSquare = glm::length (glm::cross (newVertexTreeNode.normal, newVertexTreeNode.coveredNormals.at(j)));
			currentSinAlphaSquare *= currentSinAlphaSquare;
			if (currentSinAlphaSquare > sinAlphaSquare)
				sinAlphaSquare = currentSinAlphaSquare;
		}
		newVertexTreeNode.sinAlphaSquare = sinAlphaSquare;
		*/
		newVertexTreeNode.sinAlphaSquare = getAlphaSquare (newVertexTreeNode.normal, newVertexTreeNode.coveredNormals);
		///////////////////////get alpha for rule 2 ends//////////////////////////////////////////////////////////////////////////////////////////////////

		///////////////////////get muSquare and deltaSquare for rule 3////////////////////////////////////////////////////////////////////////////////////
		newVertexTreeNode.muSquare = 0.f;
		newVertexTreeNode.deltaSquare = 0.f;
		///////////////////////get muSquare and deltaSquare for rule 3 ends////////////////////////////////////////////////////////////////////////////////////

		vertexTree.push_back (newVertexTreeNode);    //push back the current node to the vertexTree
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////initialize vertexTree halts here/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////initialize currentMesh starts from here, we will use some info about neighborFaces and neighborVertices from vertexTree above////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	assertVectorEmpty (currentMesh);   //see if vertexTree is successfully initialized to empty
	//now use i to proecess each face in input original faces
	for (int i=0; i<(inputFaces.size()/3); i++)
	{
		CurrentMeshNode newCurrentMeshNode;
		//first get all three vertex indices of the current face being processed, along with the 3 samped points 
		//int verticesInCurrentFace[3];
		for (int j=0; j<3; j++)
		{
			//verticesInCurrentFace[j] = inputFaces[3*i+j];
			newCurrentMeshNode.vertexIndices[j] = inputFaces[3*i+j];  //the vertex index itself, namely for the vertexIndeces[3]
			//now for the projectedSamplePoints vector
			SampledPointAndDist theSample = SampledPointAndDist (inputVertices[newCurrentMeshNode.vertexIndices[j]].pos, 0.f,
				                                                 inputVertices[newCurrentMeshNode.vertexIndices[j]].pos);   //the sample for this vertex
			newCurrentMeshNode.projectedSamplePoints.push_back (theSample);  //push the sample back

			//make sure the candidateCollapses are initialized coorect
			newCurrentMeshNode.candidateCollapses[j].edgeTestedElseWhere = false;
		}
		newCurrentMeshNode.totalDistSamplePoints = 0.f;  //get the totalDistSamplePoints

		//what's left is the neighborFaces[3]
		//first initialize them to null, namely -10
		for (int j=0; j<3; j++)
		{
			newCurrentMeshNode.neighborFaces[j] = -10;
		}
		//for vertex v0, let's get the neighborFace for edge v0-v1 and v0-v2
		int v0 = newCurrentMeshNode.vertexIndices[0], v1 = newCurrentMeshNode.vertexIndices[1], v2 = newCurrentMeshNode.vertexIndices[2];
		//counters for faces adjacent to each edge, to detect whether it's a manifold
		short v0v1Counter = 0, v1v2Counter = 0, v2v0Counter = 0;

		//first get the two faces adjacent to v0-v1 and v2-v0
		for (int j=0; j<vertexTree[v0].neighborFaces.size(); j++)   //traverse the neighbor faces of v0
		{
			int v0neighborFaceIndex = vertexTree[v0].neighborFaces.at(j);
			if (v0neighborFaceIndex != i)   //if it's indeed a neighbor face of v0, not the current face being processed
			{
				for (int k=0; k<3; k++)  //traverse each vertex in each neighbor face of v0
				{
					int v0neighborFaceVertexIndex = inputFaces[3*v0neighborFaceIndex+k];  //get the index for the current vertex being processed
					if (v0neighborFaceVertexIndex == v1 )   //if the neighbor face is on edge v0-v1
					{
						v0v1Counter++ ;
						newCurrentMeshNode.neighborFaces[0] = v0neighborFaceIndex;
					}
					if (v0neighborFaceVertexIndex == v2 )   //if the neighbor face is on edge v0-v2
					{
						v2v0Counter++;
						newCurrentMeshNode.neighborFaces[2] = v0neighborFaceIndex;
					}

				}
			}

			//report error if there are more than 2 faces adjacent to one edge, which means it's not a manifold
			if (v0v1Counter>1)
				std::cout << "There are more than 2 faces adjacent to these two vertices: " << v0 << " and " << v1 << "in the initial original input mesh" << '\n';
			if (v2v0Counter>1)
				std::cout << "There are more than 2 faces adjacent to these two vertices: " << v2 << " and " << v0 << "in the initial original input mesh" << '\n';
		}

		//now get the face adjacent to v1-v2
		for (int j=0; j<vertexTree[v1].neighborFaces.size(); j++)   //traverse the neighbor faces of v1
		{
			int v1neighborFaceIndex = vertexTree[v1].neighborFaces.at(j);
			if (v1neighborFaceIndex != i)
			{
				for (int k=0; k<3; k++)  //traverse each ertex in each neighbor face of v1
				{
					int v1neighborFaceVertexIndex = inputFaces[3*v1neighborFaceIndex + k];
					if (v1neighborFaceVertexIndex == v2)   //if the neighbor face is on edge v1-v2
					{
						v1v2Counter++;
						newCurrentMeshNode.neighborFaces[1] = v1neighborFaceIndex;
					}
				}
			}

			//report error if there are more than 2 faces adjacent to one edge, which means it's not a manifold
			if (v1v2Counter>1)
				std::cout << "There are more than 2 faces adjacent to these two vertices: " << v1 << " and " << v2 << "in the initial original input mesh" << '\n';
		}
		currentMesh.push_back(newCurrentMeshNode);
	}	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////initialization of the original input mesh ends here//////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

void PMConstruction::initFirstCollapse ()
{
	//////////////////////////////////////////////////////////////////////////////////////////////////
	////////Building the "progressive part" starts here///////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////

	////////First we initialize the "candidate collapsing" using all the edges in the current mesh//////////
	/*
	for (int i=0; i<currentMesh.size(); i++)  //traverse each triangle in the current mesh, which will be face fl for the edge collapse
	{
		CurrentMeshNode theFace = currentMesh.at(i);  //cache the node being processed
		//pass this face if it is already collapsed, not likely to happen since this is the initialization 
		if (theFace.vertexIndices[0] == -10 && theFace.vertexIndices[1] == -10 && theFace.vertexIndices[2] == -10)
			continue;

		//Now we traverse the edges one by one for the current triangle i in the current mesh
		for (int j=0; j<3; j++)
		{
			//first get the edge being processed, namly the potential vt and vu
			int collapsingEdge[2];
			collapsingEdge[0] = theFace.vertexIndices[j];
			collapsingEdge[1] = (j==2) ? theFace.vertexIndices[0] : theFace.vertexIndices[j+1];
			//next get the face adjacent to the current edge, this is face fr
			int edgeAdjacentFace = theFace.neighborFaces[j];

			//data structure to store info about the two candidate kinds of collapsing, for triangle i and edge j
			CandidateCollapsingWithNewSubMesh edgeCollapses[2];
			for (int k=0; k<2; k++)
			{
				edgeCollapses[k].vtOrVu = 1-k;
				edgeCollapses[k].totalDistSamplePoints = 0.f; //make sure the total distance is initialized to 0

				//get vt and vu, vt is the same as the new vertex
				edgeCollapses[k].vtvu[0] = collapsingEdge[1-k];
				edgeCollapses[k].vtvu[1] = collapsingEdge[k];
			}

			//traverse the two possible kinds of collapsing, with vt or vu as the new vertex
			//new vertex put to collapsingEdge[1] first, then collapsingEdge[0]
			//below are two kinds of collapse for one edge!
			for (int k=0; k<2; k++)             
			{
				//below is one kind of collapse for one edge!
				
				//first get fl and fr
				if (k==0)  //vtOrVu==1, collapsingEdge[1] has the same attributes as the new vertex
				{
					edgeCollapses[k].flfr[0] = i;
					edgeCollapses[k].flfr[1] = edgeAdjacentFace;
				}
				else
				{
					edgeCollapses[k].flfr[0] = edgeAdjacentFace;
					edgeCollapses[k].flfr[1] = i;
				}

				//now get vl and vr
				for (int l=0; l<2; l++)
				{
					if (edgeCollapses[k].flfr[l]<0)
						edgeCollapses[k].vlvr[l] = -10;    //if fl or fr is at the edge of the mesh
					else
						for (int m=0; m<3; m++)
						{
							int vIndex = currentMesh.at(edgeCollapses[k].flfr[l]).vertexIndices[m];
							if (vIndex != collapsingEdge[0] && vIndex != collapsingEdge[1])
							{
								edgeCollapses[k].vlvr[l] = vIndex;
								break;
							}
						}
				}

				//the intermidiate data that stores all the sampled points priviously projected onto the old submesh involved in this collapsing
				std::vector <glm::vec3> involvedSamplePoints;
				//push fl and fr 's sample points into involvedSamplePoints
				for (int n=0; n<currentMesh.at(i).projectedSamplePoints.size(); n++)
					involvedSamplePoints.push_back (currentMesh.at(i).projectedSamplePoints.at(n).point);
				if (edgeAdjacentFace != -10)  //if this edge is not an edge of the whole mesh
					for (int n=0; n<currentMesh.at(edgeAdjacentFace).projectedSamplePoints.size(); n++)
						involvedSamplePoints.push_back (currentMesh.at(edgeAdjacentFace).projectedSamplePoints.at(n).point);
				for (int l=0; l<2; l++)        //get the adjacent faces of vt and vu, by traversing all the neighbor faces of vt AND vu
				{
					for (int m=0; m<vertexTree.at(collapsingEdge[l]).neighborFaces.size(); m++)   //traverse each adjacent face of vt OR vu
					{
						int edgeEndAdjacentFace = vertexTree.at(collapsingEdge[l]).neighborFaces.at(m);

						//if the face is the fl or fr, pass
						if (edgeEndAdjacentFace == i || edgeEndAdjacentFace == edgeAdjacentFace)
							continue;
						PotentialNewFaceAfterCollapsing newFace;  //each newFace deals with one adjacent face of vt OR vu
						newFace.faceIndexInCurrentMesh = edgeEndAdjacentFace;    //the face index of the current face adjacent to vt or vu
						newFace.totalDistSamplePoints = 0.f;   //make sure the total distance of every face is initialized to 0
						//push all the sample points on the involved faces into one vector
						for (int n=0; n<currentMesh.at(newFace.faceIndexInCurrentMesh).projectedSamplePoints.size(); n++)
							involvedSamplePoints.push_back (currentMesh.at(newFace.faceIndexInCurrentMesh).projectedSamplePoints.at(n).point);
						assertVectorEmpty (newFace.projectedSamplePoints);

						//get the vertex indices of the new face 
						for (int n=0; n<3; n++)                                                  //traverse each vertex of one adjacent face of vt or vu
						{
							int theVertexIndex = currentMesh.at(newFace.faceIndexInCurrentMesh).vertexIndices[n];   //get the current vertex before the collapsing
							if (theVertexIndex == collapsingEdge[k])  //if it is the vertex of the currently collapsing (disappearing) vertex,                                   
                                newFace.newVertexIndicesInCurrentMesh[n] = collapsingEdge[1-k]; //get the other end of the edge as the new vertex
								//newFace.second.vertexIndices[n] = -11; //creat the new vertex, 
							else
								newFace.newVertexIndicesInCurrentMesh[n] = theVertexIndex;  //otherwise the vertex doesn't change
						}
						edgeCollapses[k].newSubMesh.push_back(newFace);
					}
				}
				//now we get the "edgeCollapses[k]" and the "involvedSamplePoints" we want
				//next is the battle of distance calculation
				for (int l=0; l<involvedSamplePoints.size(); l++)    //project each sample point to the new sub mesh
				{
					glm::vec3 resultProjection;
					float resultDist = INFINITY;
					int resultFaceInNewSubMesh = -11;

					//find the closest face among the new sub mesh
					for (int m=0; m<edgeCollapses[k].newSubMesh.size(); m++) //traverse each face in the new sub mesh to find the closest project for point m
					{
						//find which face within sub mesh this face projects to and push this point into that face
						glm::vec3 triangleVerticesPos[3];  //cache the positions of the 3 vertices
						for (int n=0; n<3; n++)  //get the positions from vertexTree and inputVertices for each of the 3 vertices of this face
						{
							int vertexIndexInVertexTree = edgeCollapses[k].newSubMesh.at(m).newVertexIndicesInCurrentMesh[n];
							//get the index of the vertex in inputVertices
							int leafIndexInInputVertices = vertexTree.at(vertexIndexInVertexTree).descendentLeafInInputVertices;
							triangleVerticesPos[n] = inputVertices.at(leafIndexInInputVertices).pos;
						}
						//now we get the actual coordinates of the face, let's calculate projection from samplePoint l to face m
						float currentDist;
						glm::vec3 currentProjection = getProjection (involvedSamplePoints.at(l), triangleVerticesPos, currentDist);
						if (currentDist < resultDist)
						{
							resultDist = currentDist;
							resultFaceInNewSubMesh = m;
							resultProjection = currentProjection;
						}
					}

					//push the sampled point onto this closest face in new sub-mesh
					SampledPointAndDist pointDist = SampledPointAndDist (involvedSamplePoints.at(l), resultDist, resultProjection);
					if (resultFaceInNewSubMesh < 0)
						std::cout << "wrong index that is less than 0 !!!!\n";
					else
					{
						edgeCollapses[k].newSubMesh.at(resultFaceInNewSubMesh).projectedSamplePoints.push_back (pointDist);
						edgeCollapses[k].newSubMesh.at(resultFaceInNewSubMesh).totalDistSamplePoints += resultDist;
					}
				}
				for (int l=0; l<edgeCollapses[k].newSubMesh.size(); l++)
				{
					edgeCollapses[k].totalDistSamplePoints += edgeCollapses[k].newSubMesh.at(l).totalDistSamplePoints;
				}
			}
			currentMesh.at(i).candidateCollapses[j] = (edgeCollapses[0].totalDistSamplePoints < edgeCollapses[1].totalDistSamplePoints)? edgeCollapses[0] : edgeCollapses[1];
			
		}
	}
	*/
	for (int i=0; i<currentMesh.size(); i++)
		for (int j=0; j<3; j++)
			if (currentMesh.at(i).candidateCollapses[j].edgeTestedElseWhere == false)
				updateCandidateCollapse (Edge(i, j));
}

void PMConstruction::updateCandidateCollapse (Edge theEdge)
{
	//////////////////////////////////////////////////////////////////////////////////////////////////
	////////Building the "progressive part" starts here///////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////

	//first get the edge being processed, namely the potential vt and vu
	int collapsingEdge[2];
	collapsingEdge[0] = currentMesh.at(theEdge.triangleIndex).vertexIndices[theEdge.edgeIndex];
	collapsingEdge[1] = (theEdge.edgeIndex==2) ? currentMesh.at(theEdge.triangleIndex).vertexIndices[0] : 
		                                         currentMesh.at(theEdge.triangleIndex).vertexIndices[theEdge.edgeIndex+1];
	//next get the face adjacent to the current edge, this is face fr
	int edgeAdjacentFace = currentMesh.at(theEdge.triangleIndex).neighborFaces[theEdge.edgeIndex];

	//put the corresponding edge in edgeAdjacentFace to already tested elsewhere
	if (edgeAdjacentFace != -10)
	{
		for (int k=0; k<3; k++)
		{
			int neighborOfEdgeAdjacentFace = currentMesh.at(edgeAdjacentFace).neighborFaces[k];
			if (neighborOfEdgeAdjacentFace == theEdge.triangleIndex)
			{
				currentMesh.at(edgeAdjacentFace).candidateCollapses[k].edgeTestedElseWhere = true;
			    break;
			}
		}
	}

	//data structure to store info about the two candidate kinds of collapsing, for triangle i and edge j
	CandidateCollapsingWithNewSubMesh edgeCollapses[2];
	for (int k=0; k<2; k++)
	{
		//initialize some info
		edgeCollapses[k].vtOrVu = 1-k;
		edgeCollapses[k].totalDistSamplePoints = 0.f; //make sure the total distance is initialized to 0
		edgeCollapses[k].edgeTestedElseWhere = false;

		//get vt and vu, vt is the same as the new vertex
		edgeCollapses[k].vtvu[0] = collapsingEdge[1-k];
		edgeCollapses[k].vtvu[1] = collapsingEdge[k];


	}

	//traverse the two possible kinds of collapsing, with vt or vu as the new vertex
	//new vertex put to collapsingEdge[1] first, then collapsingEdge[0]
	//below are two kinds of collapse for one edge!
	for (int k=0; k<2; k++)             
	{
		//below is one kind of collapse for one edge!
				
		//first get fl and fr
		if (k==0)  //vtOrVu==1, collapsingEdge[1] has the same attributes as the new vertex
		{
			edgeCollapses[k].flfr[0] = theEdge.triangleIndex;
			edgeCollapses[k].flfr[1] = edgeAdjacentFace;
		}
		else
		{
			edgeCollapses[k].flfr[0] = edgeAdjacentFace;
			edgeCollapses[k].flfr[1] = theEdge.triangleIndex;
		}

		//now get vl and vr
		for (int l=0; l<2; l++)
		{
			if (edgeCollapses[k].flfr[l]<0)
			{
				edgeCollapses[k].vlvr[l] = -10;    //if fl or fr is at the edge of the mesh
				continue;
			}
			else
				for (int m=0; m<3; m++)
				{
					int vIndex = currentMesh.at(edgeCollapses[k].flfr[l]).vertexIndices[m];
					if (vIndex != collapsingEdge[0] && vIndex != collapsingEdge[1])
					{
						edgeCollapses[k].vlvr[l] = vIndex;
						break;
					}
				}
		}

		//the intermidiate data that stores all the sampled points priviously projected onto the old submesh involved in this collapsing
		std::vector <glm::vec3> involvedSamplePoints;
		//push fl and fr 's sample points into involvedSamplePoints
		for (int n=0; n<currentMesh.at(theEdge.triangleIndex).projectedSamplePoints.size(); n++)
			involvedSamplePoints.push_back (currentMesh.at(theEdge.triangleIndex).projectedSamplePoints.at(n).point);
		if (edgeAdjacentFace != -10)  //if this edge is not an edge of the whole mesh
			for (int n=0; n<currentMesh.at(edgeAdjacentFace).projectedSamplePoints.size(); n++)
				involvedSamplePoints.push_back (currentMesh.at(edgeAdjacentFace).projectedSamplePoints.at(n).point);
		for (int l=0; l<2; l++)        //get the adjacent faces of vt and vu, by traversing all the neighbor faces of vt AND vu
		{
			for (int m=0; m<vertexTree.at(collapsingEdge[l]).neighborFaces.size(); m++)   //traverse each adjacent face of vt OR vu
			{
				int edgeEndAdjacentFace = vertexTree.at(collapsingEdge[l]).neighborFaces.at(m);

				//if the face is the fl or fr, pass
				if (edgeEndAdjacentFace == theEdge.triangleIndex || edgeEndAdjacentFace == edgeAdjacentFace)
					continue;
				PotentialNewFaceAfterCollapsing newFace;  //each newFace deals with one adjacent face of vt OR vu
				newFace.faceIndexInCurrentMesh = edgeEndAdjacentFace;    //the face index of the current face adjacent to vt or vu
				newFace.totalDistSamplePoints = 0.f;   //make sure the total distance of every face is initialized to 0
				newFace.shapeChanged = false;          //initialize the shapeChanged as false, so later when we encounter vu we put it to true
				//push all the sample points on the involved faces into one vector
				for (int n=0; n<currentMesh.at(newFace.faceIndexInCurrentMesh).projectedSamplePoints.size(); n++)
					involvedSamplePoints.push_back (currentMesh.at(newFace.faceIndexInCurrentMesh).projectedSamplePoints.at(n).point);
				assertVectorEmpty (newFace.projectedSamplePoints);

				//get the vertex indices of the new face 
				for (int n=0; n<3; n++)                                                  //traverse each vertex of one adjacent face of vt or vu
				{
					int theVertexIndex = currentMesh.at(newFace.faceIndexInCurrentMesh).vertexIndices[n];   //get the current vertex before the collapsing
					if (theVertexIndex == collapsingEdge[k])  //if it is the vertex of the currently collapsing (disappearing) vertex, 
					{
                        newFace.newVertexIndicesInCurrentMesh[n] = collapsingEdge[1-k]; //get the other end of the edge as the new vertex
						//newFace.second.vertexIndices[n] = -11; //creat the new vertex, 
						newFace.shapeChanged = true;
					}
					else
					{
						newFace.newVertexIndicesInCurrentMesh[n] = theVertexIndex;  //otherwise the vertex doesn't change
					}
				}
				edgeCollapses[k].newSubMesh.push_back(newFace);
			}
		}
		//now we get the "edgeCollapses[k]" and the "involvedSamplePoints" we want
		//next is the battle of distance calculation
		for (int l=0; l<involvedSamplePoints.size(); l++)    //project each sample point to the new sub mesh
		{
			glm::vec3 resultProjection;
			float resultDist = INFINITY;
			int resultFaceInNewSubMesh = -11;

			//find the closest face among the new sub mesh
			for (int m=0; m<edgeCollapses[k].newSubMesh.size(); m++) //traverse each face in the new sub mesh to find the closest project for point m
			{
				//find which face within sub mesh this face projects to and push this point into that face
				glm::vec3 triangleVerticesPos[3];  //cache the positions of the 3 vertices
				for (int n=0; n<3; n++)  //get the positions from vertexTree and inputVertices for each of the 3 vertices of this face
				{
					int vertexIndexInVertexTree = edgeCollapses[k].newSubMesh.at(m).newVertexIndicesInCurrentMesh[n];
					//get the index of the vertex in inputVertices
					int leafIndexInInputVertices = vertexTree.at(vertexIndexInVertexTree).descendentLeafInInputVertices;
					triangleVerticesPos[n] = inputVertices.at(leafIndexInInputVertices).pos;
				}
				//now we get the actual coordinates of the face, let's calculate projection from samplePoint l to face m
				float currentDist;
				glm::vec3 currentProjection = getProjection (involvedSamplePoints.at(l), triangleVerticesPos, currentDist);
				if (currentDist < resultDist)
				{
					resultDist = currentDist;
					resultFaceInNewSubMesh = m;
					resultProjection = currentProjection;
				}
			}

			//push the sampled point onto this closest face in new sub-mesh
			SampledPointAndDist pointDist = SampledPointAndDist (involvedSamplePoints.at(l), resultDist, resultProjection);
			if (resultFaceInNewSubMesh < 0)
				std::cout << "wrong index that is less than 0 !!!!\n";
			else
			{
				edgeCollapses[k].newSubMesh.at(resultFaceInNewSubMesh).projectedSamplePoints.push_back (pointDist);
				edgeCollapses[k].newSubMesh.at(resultFaceInNewSubMesh).totalDistSamplePoints += resultDist;
			}
		}
		for (int l=0; l<edgeCollapses[k].newSubMesh.size(); l++)
		{
			edgeCollapses[k].totalDistSamplePoints += edgeCollapses[k].newSubMesh.at(l).totalDistSamplePoints;
		}
	}
	currentMesh.at(theEdge.triangleIndex).candidateCollapses[theEdge.edgeIndex] = (edgeCollapses[0].totalDistSamplePoints < edgeCollapses[1].totalDistSamplePoints)? edgeCollapses[0] : edgeCollapses[1];
}

void PMConstruction::updtGeoInfoColOneEdge()
{
	//first find which edge to collapse
	int collapsingTriangle = -11;
	int collapsingEdge = -11;
	float smallestSubMeshDistSamplePts = INFINITY;
	for (int i = 0; i<currentMesh.size(); i++)
	{
		if (currentMesh.at(i).vertexIndices[0] < 0)
			continue;
		for (int j=0; j<3; j++)
		{
			if (currentMesh.at(i).candidateCollapses[j].edgeTestedElseWhere == false  &&
				currentMesh.at(i).candidateCollapses[j].totalDistSamplePoints < smallestSubMeshDistSamplePts)
			{
				collapsingTriangle = i;
				collapsingEdge = j;
				smallestSubMeshDistSamplePts = currentMesh.at(i).candidateCollapses[j].totalDistSamplePoints;
			}
		}
	}
	//now update the vertexTree
	VertexTreeNode newTreeNode = constructVertexTreeNode (collapsingTriangle, collapsingEdge);
	vertexTree.push_back (newTreeNode);
	vertexTree.at(newTreeNode.vtvu[0]).vp = int(vertexTree.size()-1);
	vertexTree.at(newTreeNode.vtvu[1]).vp = int(vertexTree.size()-1);

	//next is the currentMesh
	const CandidateCollapsingWithNewSubMesh &collapse = currentMesh.at(collapsingTriangle).candidateCollapses[collapsingEdge];
	//update the currentMesh
	updateMeshForNewVertexNode (collapse);
	updtCandidColInfoColOneEdge (collapse);
	updtRenderData();
}

void PMConstruction::updtCandidColInfoColOneEdge(const CandidateCollapsingWithNewSubMesh &collapse)
{
	std::vector <Edge> edgesToUpdate;
	std::vector <int> changedArea; //face indices of the changed area (with shapeChanged as True) 
	//first get all the edges of the changed faces
	for (int i=0; i<collapse.newSubMesh.size(); i++)
	{
		changedArea.push_back (collapse.newSubMesh.at(i).faceIndexInCurrentMesh);
		for (int j=0; j<3; j++)
			edgesToUpdate.push_back (Edge(collapse.newSubMesh.at(i).faceIndexInCurrentMesh, j));
	}

	//Then get the edges that are ajdacent to the changed area
	std::vector <int> changedAreaEdgeVertices;   //the vertices that surrounding the changed area (with shapeChanged as True) 
	changedAreaEdgeVertices = (*(vertexTree.end()-1)).neighborVertices; //This list equal to the neighbors of vu
	//update vt to new vertex
	for (int i=0; i<changedAreaEdgeVertices.size(); i++)
	{
		//if ( changedAreaEdgeVertices.at(i) == collapse.vtvu[0])
		//	changedAreaEdgeVertices.at(i) = int(vertexTree.size()-1);
		int vIndex = changedAreaEdgeVertices.at(i);
		for (int j=0; j<vertexTree.at(vIndex).neighborFaces.size(); j++)
		{
			int fIndex = vertexTree.at(vIndex).neighborFaces.at(j);
			for (int k=0; k<3; k++)
			{
				if (std::find (changedArea.begin(), changedArea.end(), fIndex) == changedArea.end() &&
					std::find (edgesToUpdate.begin(), edgesToUpdate.end(), Edge(fIndex, k)) == edgesToUpdate.end())
				{
					int theEdge[2];
					theEdge[0] = currentMesh.at(fIndex).vertexIndices[k];
					theEdge[1] = (k==2)? currentMesh.at(fIndex).vertexIndices[0] : currentMesh.at(fIndex).vertexIndices[k+1];
					if (findElementInArray(theEdge, vIndex, 2) >=0)
						edgesToUpdate.push_back(Edge(fIndex, k));
				}
			}
		}
	}
	for (int i=0; i<edgesToUpdate.size(); i++)
		currentMesh.at(edgesToUpdate.at(i).triangleIndex).candidateCollapses[edgesToUpdate.at(i).edgeIndex].edgeTestedElseWhere = false;
	for (int i=0; i<edgesToUpdate.size(); i++)
		if (currentMesh.at(edgesToUpdate.at(i).triangleIndex).candidateCollapses[edgesToUpdate.at(i).edgeIndex].edgeTestedElseWhere == false)
			updateCandidateCollapse (edgesToUpdate.at(i));
}

VertexTreeNode PMConstruction::constructVertexTreeNode (int collapsingTriangle,int collapsingEdge)
{
	//get the two vertices of the collapsing edge
	int vertexToDisappear[2]; //[0] is the same as the new vertex, namely vt; [1] is the one totally disappeared, namly vu
	vertexToDisappear[0] = currentMesh.at(collapsingTriangle).candidateCollapses[collapsingEdge].vtvu[0];
	vertexToDisappear[1] = currentMesh.at(collapsingTriangle).candidateCollapses[collapsingEdge].vtvu[1];

	//now construct the new vertexTreeNode
	VertexTreeNode newVertex;
	newVertex.descendentLeafInInputVertices = vertexTree.at(vertexToDisappear[0]).descendentLeafInInputVertices;
	newVertex.vp = -11;
	newVertex.vtvu[0] = vertexToDisappear[0];
	newVertex.vtvu[1] = vertexToDisappear[1];

	//now get the neighbor geometry
	assertVectorEmpty (newVertex.neighborFaces);
	assertVectorEmpty (newVertex.neighborVertices);
	//first the neighbor faces, namely all the new faces in CandidateCollapse
	for (int i=0; i<currentMesh.at(collapsingTriangle).candidateCollapses[collapsingEdge].newSubMesh.size(); i++)
	{
		newVertex.neighborFaces.push_back (currentMesh.at(collapsingTriangle).candidateCollapses[collapsingEdge].newSubMesh.at(i).faceIndexInCurrentMesh);
	}
	//next get the neighbor vertices, vl, vr and all the other original adjacent vertices of vt and vu
	//this may be -10 if one of fl, fr is null
	if (currentMesh.at(collapsingTriangle).candidateCollapses[collapsingEdge].vlvr[0] > 0)
		newVertex.neighborVertices.push_back(currentMesh.at(collapsingTriangle).candidateCollapses[collapsingEdge].vlvr[0]);
	if (currentMesh.at(collapsingTriangle).candidateCollapses[collapsingEdge].vlvr[1] > 0)
		newVertex.neighborVertices.push_back(currentMesh.at(collapsingTriangle).candidateCollapses[collapsingEdge].vlvr[1]);
	for (int i=0; i<2; i++)
		for (int j=0; j<vertexTree.at(vertexToDisappear[i]).neighborVertices.size(); j++)
		{
			int neighborVertex = vertexTree.at(vertexToDisappear[i]).neighborVertices.at(j);
			if (neighborVertex != vertexToDisappear[0] && neighborVertex != vertexToDisappear[1] &&
				neighborVertex != currentMesh.at(collapsingTriangle).candidateCollapses[collapsingEdge].vlvr[0] &&
				neighborVertex != currentMesh.at(collapsingTriangle).candidateCollapses[collapsingEdge].vlvr[1] )
				newVertex.neighborVertices.push_back (neighborVertex);
		}
	//now process the normals
	assertVectorEmpty (newVertex.coveredNormals);
	newVertex.normal = calAvgNormal (currentMesh.at(collapsingTriangle).candidateCollapses[collapsingEdge]);
	for (int i=0; i<2; i++)
		for (int j=0; j<vertexTree.at(vertexToDisappear[i]).coveredNormals.size(); j++)
		{
			bool normalAlreadyPushed = false;
			for (int k=0; k<newVertex.coveredNormals.size(); k++)
			{
				if (newVertex.coveredNormals.at(k) == vertexTree.at(vertexToDisappear[i]).coveredNormals.at(j))
				{
					normalAlreadyPushed = true;
					break;
				}
			}
			if (normalAlreadyPushed == false)
				newVertex.coveredNormals.push_back (vertexTree.at(vertexToDisappear[i]).coveredNormals.at(j));
		}
	//get alpha for rule 2
	newVertex.sinAlphaSquare = getAlphaSquare (newVertex.normal, newVertex.coveredNormals);

	//update the bounding sphere
	float newRadius =  updateBoundingSphere (inputVertices.at( newVertex.descendentLeafInInputVertices ).pos, vertexTree.at (vertexToDisappear[0]).boundingSphereRadius, 
		                                     inputVertices.at( vertexTree.at(vertexToDisappear[1]).descendentLeafInInputVertices ).pos, 
											 vertexTree.at(vertexToDisappear[1]).boundingSphereRadius);
	newVertex.boundingSphereRadius = newRadius;

	//update the screen space error information
	float resultMuSquare = 0.f, resultDeltaSquare = 0.f;
	getSSErrorInfo (currentMesh.at(collapsingTriangle).candidateCollapses[collapsingEdge], resultMuSquare, resultDeltaSquare, newVertex.normal);
	newVertex.muSquare = resultMuSquare; newVertex.deltaSquare = resultDeltaSquare;
	return newVertex;
}

void PMConstruction::updateMeshForNewVertexNode (const CandidateCollapsingWithNewSubMesh &collapse)
{
	//update the neighbor faces for the new faces
	//this chunk of code identifies fn0,1,2,3 in the paper
	////////////////////////////////////////////////////////////////////////////////////
	int fn[4];  //order is fvlvt, fvlvu, fvrvt, fvrv
	int edgeIndex[4];  //the edge index of these 4 faces that are adjacent to fl and fr
	for (int i=0; i<4; i++)
	{
		fn[i] = -10;
		edgeIndex[i] = -10;
	}
	for (int i=0; i<2; i++)
	{
		if (collapse.flfr[i] == -10)
			continue;
		for (int j=0; j<3; j++)
		{
			int neighborFaceIndex = currentMesh.at(collapse.flfr[i]).neighborFaces[j];
			if (neighborFaceIndex == collapse.flfr[1-i] || neighborFaceIndex == -10)  //if this neighbor is fl or fr or null
				continue;

			//if it is one of the four fn
			
			//find the edge index (0,1 or 2) on face "neighborFaceIndex" that is adjacent to Fl or Fr
			int edgeAdjacentToFlFr = findElementInArray (currentMesh.at(neighborFaceIndex).neighborFaces, 
				                                         collapse.flfr[i], 3);
			if (edgeAdjacentToFlFr < 0 || edgeAdjacentToFlFr > 2)
				std::cout << "Error!\n";
			int thisEdge[2];
			thisEdge[0] = currentMesh.at(neighborFaceIndex).vertexIndices [edgeAdjacentToFlFr];
			thisEdge[1] = (edgeAdjacentToFlFr == 2)? currentMesh.at(neighborFaceIndex).vertexIndices [0] : 
				                                     currentMesh.at(neighborFaceIndex).vertexIndices [edgeAdjacentToFlFr+1];
			if (findElementInArray(thisEdge, collapse.vlvr[0], 2) >= 0 && findElementInArray(thisEdge, collapse.vlvr[1], 2) < 0)
			{
				if (findElementInArray(thisEdge, collapse.vtvu[0], 2) >= 0 && findElementInArray(thisEdge, collapse.vtvu[1], 2) < 0)
				{
					fn[0] = neighborFaceIndex;
					edgeIndex[0] = edgeAdjacentToFlFr;
				}
				else if (findElementInArray(thisEdge, collapse.vtvu[0], 2) < 0 && findElementInArray(thisEdge, collapse.vtvu[1], 2) >= 0)
				{
					fn[1] = neighborFaceIndex;
					edgeIndex[1] = edgeAdjacentToFlFr;
				}
				else
					std::cout << "Error in fn[0] or fn[1]!\n";
			}
			else if (findElementInArray(thisEdge, collapse.vlvr[1], 2) >= 0 && findElementInArray(thisEdge, collapse.vlvr[0], 2) < 0)
			{
				if (findElementInArray(thisEdge, collapse.vtvu[0], 2) >= 0 && findElementInArray(thisEdge, collapse.vtvu[1], 2) < 0)
				{
					fn[2] = neighborFaceIndex;
					edgeIndex[2] = edgeAdjacentToFlFr;
				}
				else if (findElementInArray(thisEdge, collapse.vtvu[0], 2) < 0 && findElementInArray(thisEdge, collapse.vtvu[1], 2) >= 0)
				{
					fn[3] = neighborFaceIndex;
					edgeIndex[3] = edgeAdjacentToFlFr;
				}
				else
					std::cout << "Error in in fn[2] or fn[3]!\n";
			}
			else
				std::cout << "Error in fn[0]fn[1] and fn[2]fn[3]!\n";
		}
	}
	//update the neighbor face
	if (fn[0] >= 0)
		currentMesh.at(fn[0]).neighborFaces[edgeIndex[0]] = fn[1];
	if (fn[1] >= 0)
		currentMesh.at(fn[1]).neighborFaces[edgeIndex[1]] = fn[0];
	if (fn[2] >= 0)
		currentMesh.at(fn[2]).neighborFaces[edgeIndex[2]] = fn[3];
	if (fn[3] >= 0)
		currentMesh.at(fn[3]).neighborFaces[edgeIndex[3]] = fn[2];
	///////////////////////////////////////////////////////////////////////////////////

	//update vertices in the new faces and info of projected points, along with neighbor face/vertices for each vertex
	for (int i=0; i<collapse.newSubMesh.size(); i++)
	{
		const PotentialNewFaceAfterCollapsing &newFace = collapse.newSubMesh.at(i);
		CurrentMeshNode &meshNode =  currentMesh.at(newFace.faceIndexInCurrentMesh);
		for (int j=0; j<3; j++)
		{	
			int &vIndex = meshNode.vertexIndices[j];
			if (vIndex < 0)
				std::cout << "Error that one of the vertex in the new sub mesh is null\n";
			if (vIndex == collapse.vtvu[0] || vIndex == collapse.vtvu[1])  //if this vertex changes into the new vertex
			{
				vIndex = int(vertexTree.size()-1);
			}
			if (vIndex == collapse.vlvr[0] || vIndex == collapse.vlvr[1])
				continue;
			else //update neighbor vertices of this involved vertex except when it is vr or vl
			{
				short timeOfVtVuAsNeighbor = 0;
				for (int k=0; k<vertexTree.at(vIndex).neighborVertices.size(); k++)
				{
					int &neighborOfVtOrVu = vertexTree.at(vIndex).neighborVertices.at(k);
					if (neighborOfVtOrVu == collapse.vtvu[0] ||
						neighborOfVtOrVu == collapse.vtvu[1])
					{
						neighborOfVtOrVu = int(vertexTree.size()-1);
						timeOfVtVuAsNeighbor++;
					}
				}
				if (timeOfVtVuAsNeighbor > 1)
					std::cout << "Error that vt or vu appeared more than once in one triangle\n";
			}
			meshNode.projectedSamplePoints = newFace.projectedSamplePoints;
			meshNode.totalDistSamplePoints = newFace.totalDistSamplePoints;
		}
	}

	//now update neighbor faces/vertices for vl and vr
	for (int i=0; i<2; i++)
	{
		int vlvr = collapse.vlvr[i];
		if (vlvr == -10)
			continue;
		for (int j=0; j<vertexTree.at(vlvr).neighborFaces.size(); j++)
		{
			if (vertexTree.at(vlvr).neighborFaces.at(j) == collapse.flfr[i])
			{
				vertexTree.at(vlvr).neighborFaces.erase(vertexTree.at(vlvr).neighborFaces.begin() + j);
				break;
			}
		}
		for (int j=0; j<vertexTree.at(vlvr).neighborVertices.size(); j++)
		{
			if (vertexTree.at(vlvr).neighborVertices.at(j) == collapse.vtvu[0] ||
				vertexTree.at(vlvr).neighborVertices.at(j) == collapse.vtvu[1])
			{
				vertexTree.at(vlvr).neighborVertices.erase(vertexTree.at(vlvr).neighborVertices.begin() + j);
				j-=1;
			}
		}
		vertexTree.at(vlvr).neighborVertices.push_back (int(vertexTree.size()-1));	
	}

	//update the normal of the surrounding vertices
	for (int i=0; i< (*(vertexTree.end()-1)).neighborVertices.size(); i++)
	{
		int neighborV = (*(vertexTree.end()-1)).neighborVertices.at(i);
		calNormal (vertexTree.at(neighborV));
	}
	//put fl and fr down
	//this is where all the info of those two faces should be cleared
	for (int i=0; i<2; i++)
	{
		if (collapse.flfr[i] == -10)
			continue;
		for (int j=0; j<3; j++)
			currentMesh.at(collapse.flfr[i]).vertexIndices[j] = -10;
	}
}

glm::vec3 PMConstruction::calAvgNormal (const CandidateCollapsingWithNewSubMesh & collapse)
{
	glm::vec3 candidateNormal = glm::vec3 (0.f);
	for (int i=0; i<collapse.newSubMesh.size(); i++)
	{
		glm::vec3 currentFaceVertices [3];
		currentFaceVertices[0] = inputVertices.at(vertexTree.at(collapse.newSubMesh.at(i).newVertexIndicesInCurrentMesh[0]).descendentLeafInInputVertices).pos;
		currentFaceVertices[1] = inputVertices.at(vertexTree.at(collapse.newSubMesh.at(i).newVertexIndicesInCurrentMesh[1]).descendentLeafInInputVertices).pos;
		currentFaceVertices[2] = inputVertices.at(vertexTree.at(collapse.newSubMesh.at(i).newVertexIndicesInCurrentMesh[2]).descendentLeafInInputVertices).pos;
		glm::vec3 currentFaceNormal = getFaceNormal (currentFaceVertices);   //this is the resulting normal of the current face
		candidateNormal += currentFaceNormal;
	}
	candidateNormal /= float(collapse.newSubMesh.size());
	return glm::normalize (candidateNormal);
}

void PMConstruction::initNormals(VertexTreeNode &vertex)
{
	/////////////////////// each vertex's normal is the average of the normals of the surrounding neighbor faces//////////////////////////////////
	glm::vec3 candidateNormal = glm::vec3 (0.f);
	for (int i=0; i<vertex.neighborFaces.size(); i++)
	{
		int faceIndex = vertex.neighborFaces.at(i);
		glm::vec3 currentFaceVertices [3];
		currentFaceVertices[0] = inputVertices.at(inputFaces.at(3*faceIndex)).pos;
		currentFaceVertices[1] = inputVertices.at(inputFaces.at(3*faceIndex+1)).pos;
		currentFaceVertices[2] = inputVertices.at(inputFaces.at(3*faceIndex+2)).pos;
		glm::vec3 currentFaceNormal = getFaceNormal (currentFaceVertices);   //this is the resulting normal of the current face
		vertex.coveredNormals.push_back(currentFaceNormal);   //push it back for rule 2 later
		candidateNormal += currentFaceNormal;
	}
	candidateNormal /= float(vertex.neighborFaces.size());
	vertex.normal = glm::normalize (candidateNormal);
}

void PMConstruction::calNormal (VertexTreeNode &vertex)
{
	/////////////////////// each vertex's normal is the average of the normals of the surrounding neighbor faces//////////////////////////////////
	glm::vec3 candidateNormal = glm::vec3 (0.f);
	for (int i=0; i<vertex.neighborFaces.size(); i++)
	{
		int faceIndex = vertex.neighborFaces.at(i);
		glm::vec3 currentFaceVertices [3];
		currentFaceVertices[0] = inputVertices.at(vertexTree.at(currentMesh.at(faceIndex).vertexIndices[0]).descendentLeafInInputVertices).pos;
		currentFaceVertices[1] = inputVertices.at(vertexTree.at(currentMesh.at(faceIndex).vertexIndices[1]).descendentLeafInInputVertices).pos;
		currentFaceVertices[2] = inputVertices.at(vertexTree.at(currentMesh.at(faceIndex).vertexIndices[2]).descendentLeafInInputVertices).pos;
		glm::vec3 currentFaceNormal = getFaceNormal (currentFaceVertices);   //this is the resulting normal of the current face
		//vertex.coveredNormals.push_back(currentFaceNormal);   //push it back for rule 2 later
		candidateNormal += currentFaceNormal;
	}
	candidateNormal /= float(vertex.neighborFaces.size());
	vertex.normal = glm::normalize (candidateNormal);
}

void PMConstruction::getSSErrorInfo (const CandidateCollapsingWithNewSubMesh &collapse, float &muVSquare, float &deltaVSquare, glm::vec3 normalV)
{
	float maxError = 0.f;
	float maxEDotN = 0.f;
	float maxECrossN = 0.f;
	for (int i=0; i<collapse.newSubMesh.size(); i++)
	{
		const PotentialNewFaceAfterCollapsing &newFace = collapse.newSubMesh.at(i);
		for (int j=0; j<newFace.projectedSamplePoints.size(); j++)
		{
			//get the max error, for bounding the error using muV
			const SampledPointAndDist &sample = newFace.projectedSamplePoints.at(j);
			if (sample.dist > maxError)
				maxError = sample.dist;

			//get the ratio between deltaV and muV
			glm::vec3 errorVector = sample.point - sample.projection;
			float EDotN = glm::dot (errorVector, normalV);
			if (EDotN < 0.f)
				std::cout << "error vector lies on the other side of normal \n";
			EDotN = std::abs (EDotN);
			if (EDotN > maxEDotN)
				maxEDotN = EDotN;

			float eCrossN = glm::length (glm::cross (errorVector, normalV));
			if (eCrossN > maxECrossN)
				maxECrossN = eCrossN;			
		}
	}
	float ratio = maxEDotN/maxECrossN;
	float muV = maxError;
	muVSquare = muV * muV;
	float deltaV = muV * ratio;
	deltaVSquare = deltaV * deltaV;
}

void PMConstruction::collapseOneEdge()
{
	updtGeoInfoColOneEdge();
}

void PMConstruction::initRenderingData()
{
	for (int i=0; i<vertexTree.size(); i++)
	{
		const VertexTreeNode &currentNode = vertexTree.at(i);
		VertexFormat newVertex (inputVertices.at(currentNode.descendentLeafInInputVertices).pos, currentNode.normal);
		renderVertices.push_back (newVertex);
	}
	for (int i=0; i<currentMesh.size(); i++)
	{
		for (int j=0; j<3; j++)
			renderIndices.push_back (currentMesh.at(i).vertexIndices[j]);
	}
}

void PMConstruction::updtRenderData()
{
	//find the newly added node at the end of the current vertexTree
	const VertexTreeNode &newlyAddedNode= vertexTree.at(vertexTree.size()-1);
	VertexFormat newVertex (inputVertices.at(newlyAddedNode.descendentLeafInInputVertices).pos, newlyAddedNode.normal);
	renderVertices.push_back (newVertex);
	//update the normals
	for (int i=0; i<vertexTree.size(); i++)
	{
		renderVertices.at(i).normal = vertexTree.at(i).normal;
	}
	//update the indices
	renderIndices.clear();
	for (int i=0; i<currentMesh.size(); i++)
		if (currentMesh.at(i).vertexIndices[0] >= 0 )
			for (int j=0; j<3; j++)
				renderIndices.push_back (currentMesh.at(i).vertexIndices[j]);
}