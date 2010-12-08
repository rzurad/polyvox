/*******************************************************************************
Copyright (c) 2005-2009 David Williams

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*******************************************************************************/

#include <cfloat> //For numeric_limits

namespace PolyVox
{
	////////////////////////////////////////////////////////////////////////////////
	// aStarDefaultVoxelValidator free function
	////////////////////////////////////////////////////////////////////////////////
	template <typename VoxelType>
	bool aStarDefaultVoxelValidator(const Volume<VoxelType>* volData, const Vector3DInt16& v3dPos)
	{
		//Voxels are considered valid candidates for the path if they are inside the volume...
		if(volData->getEnclosingRegion().containsPoint(v3dPos) == false)
		{
			return false;
		}

		//and if their density is below the threshold.
		Material8 voxel = volData->getVoxelAt(static_cast<Vector3DUint16>(v3dPos));
		if(voxel.getDensity() >= Material8::getThreshold())
		{
			return false;
		}

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	// AStarPathfinder Class
	////////////////////////////////////////////////////////////////////////////////
	template <typename VoxelType>
	AStarPathfinder<VoxelType>::AStarPathfinder(const AStarPathfinderParams<VoxelType>& params)
		:m_params(params)
	{
	}

	template <typename VoxelType>
	void AStarPathfinder<VoxelType>::execute()
	{
		//Clear any existing nodes
		allNodes.clear();
		openNodes.clear();
		closedNodes.clear();

		//Clear the result
		m_params.result->clear();

		AllNodesContainer::iterator startNode = allNodes.insert(Node(m_params.start.getX(), m_params.start.getY(), m_params.start.getZ())).first;
		AllNodesContainer::iterator endNode = allNodes.insert(Node(m_params.end.getX(), m_params.end.getY(), m_params.end.getZ())).first;

		/*Node::startPos = startNode->position;
		Node::endPos = endNode->position;
		Node::m_eConnectivity = m_eConnectivity;*/

		Node* tempStart = const_cast<Node*>(&(*startNode));
		tempStart->gVal = 0;
		tempStart->hVal = computeH(startNode->position, endNode->position);

		Node* tempEnd = const_cast<Node*>(&(*endNode));
		tempEnd->hVal = 0.0f;

		openNodes.insert(startNode);

		float fDistStartToEnd = (endNode->position - startNode->position).length();
		m_fProgress = 0.0f;
		if(m_params.progressCallback)
		{
			m_params.progressCallback(m_fProgress);
		}

		while((openNodes.empty() == false) && (openNodes.getFirst() != endNode))
		{
			//Move the first node from open to closed.
			current = openNodes.getFirst();
			openNodes.removeFirst();
			closedNodes.insert(current);

			//Update the user on our progress
			if(m_params.progressCallback)
			{
				const float fMinProgresIncreament = 0.001f;
				float fDistCurrentToEnd = (endNode->position - current->position).length();
				float fDistNormalised = fDistCurrentToEnd / fDistStartToEnd;
				float fProgress = 1.0f - fDistNormalised;
				if(fProgress >= m_fProgress + fMinProgresIncreament)
				{
					m_fProgress = fProgress;
					m_params.progressCallback(m_fProgress);
				}
			}

			//The distance from one cell to another connected by face, edge, or corner.
			const float fFaceCost = sqrt_1;
			const float fEdgeCost = sqrt_2;
			const float fCornerCost = sqrt_3;

			//Process the neighbours. Note the deliberate lack of 'break' 
			//statements, larger connectivities include smaller ones.
			switch(m_params.connectivity)
			{
			case TwentySixConnected:
				processNeighbour(current->position + arrayPathfinderCorners[0], current->gVal + fCornerCost);
				processNeighbour(current->position + arrayPathfinderCorners[1], current->gVal + fCornerCost);
				processNeighbour(current->position + arrayPathfinderCorners[2], current->gVal + fCornerCost);
				processNeighbour(current->position + arrayPathfinderCorners[3], current->gVal + fCornerCost);
				processNeighbour(current->position + arrayPathfinderCorners[4], current->gVal + fCornerCost);
				processNeighbour(current->position + arrayPathfinderCorners[5], current->gVal + fCornerCost);
				processNeighbour(current->position + arrayPathfinderCorners[6], current->gVal + fCornerCost);
				processNeighbour(current->position + arrayPathfinderCorners[7], current->gVal + fCornerCost);

			case EighteenConnected:
				processNeighbour(current->position + arrayPathfinderEdges[ 0], current->gVal + fEdgeCost);
				processNeighbour(current->position + arrayPathfinderEdges[ 1], current->gVal + fEdgeCost);
				processNeighbour(current->position + arrayPathfinderEdges[ 2], current->gVal + fEdgeCost);
				processNeighbour(current->position + arrayPathfinderEdges[ 3], current->gVal + fEdgeCost);
				processNeighbour(current->position + arrayPathfinderEdges[ 4], current->gVal + fEdgeCost);
				processNeighbour(current->position + arrayPathfinderEdges[ 5], current->gVal + fEdgeCost);
				processNeighbour(current->position + arrayPathfinderEdges[ 6], current->gVal + fEdgeCost);
				processNeighbour(current->position + arrayPathfinderEdges[ 7], current->gVal + fEdgeCost);
				processNeighbour(current->position + arrayPathfinderEdges[ 8], current->gVal + fEdgeCost);
				processNeighbour(current->position + arrayPathfinderEdges[ 9], current->gVal + fEdgeCost);
				processNeighbour(current->position + arrayPathfinderEdges[10], current->gVal + fEdgeCost);
				processNeighbour(current->position + arrayPathfinderEdges[11], current->gVal + fEdgeCost);

			case SixConnected:
				processNeighbour(current->position + arrayPathfinderFaces[0], current->gVal + fFaceCost);
				processNeighbour(current->position + arrayPathfinderFaces[1], current->gVal + fFaceCost);
				processNeighbour(current->position + arrayPathfinderFaces[2], current->gVal + fFaceCost);
				processNeighbour(current->position + arrayPathfinderFaces[3], current->gVal + fFaceCost);
				processNeighbour(current->position + arrayPathfinderFaces[4], current->gVal + fFaceCost);
				processNeighbour(current->position + arrayPathfinderFaces[5], current->gVal + fFaceCost);
			}

			if(allNodes.size() > m_params.maxNumberOfNodes)
			{
				//We've reached the specified maximum number
				//of nodes. Just give up on the search.
				break;
			}
		}

		if((openNodes.empty()) || (openNodes.getFirst() != endNode))
		{
			//In this case we failed to find a valid path.
			throw runtime_error("No path found");
		}
		else
		{
			Node* n = const_cast<Node*>(&(*endNode));
			while(n != 0)
			{
				m_params.result->push_front(n->position);
				n = n->parent;
			}
		}

		if(m_params.progressCallback)
		{
			m_params.progressCallback(1.0f);
		}
	}

	template <typename VoxelType>
	void AStarPathfinder<VoxelType>::processNeighbour(const Vector3DInt16& neighbourPos, float neighbourGVal)
	{
		bool bIsVoxelValidForPath = m_params.isVoxelValidForPath(m_params.volume, neighbourPos);
		if(!bIsVoxelValidForPath)
		{
			return;
		}

		float cost = neighbourGVal;

		std::pair<AllNodesContainer::iterator, bool> insertResult = allNodes.insert(Node(neighbourPos.getX(), neighbourPos.getY(), neighbourPos.getZ()));
		AllNodesContainer::iterator neighbour = insertResult.first;

		if(insertResult.second == true) //New node, compute h.
		{
			Node* tempNeighbour = const_cast<Node*>(&(*neighbour));
			tempNeighbour -> hVal = computeH(neighbour->position, m_params.end);
		}

		OpenNodesContainer::iterator openIter = openNodes.find(neighbour);
		if(openIter != openNodes.end())
		{
			if(cost < neighbour->gVal)
			{
				openNodes.remove(openIter);
				openIter = openNodes.end();
			}
		}

		//TODO - Nodes could keep track of if they are in open or closed? And a pointer to where they are?
		ClosedNodesContainer::iterator closedIter = closedNodes.find(neighbour);
		if(closedIter != closedNodes.end())
		{
			if(cost < neighbour->gVal)
			{
				//Probably shouldn't happen?
				closedNodes.remove(closedIter);
				closedIter = closedNodes.end();
			}
		}

		if((openIter == openNodes.end()) && (closedIter == closedNodes.end()))
		{
			Node* temp = const_cast<Node*>(&(*neighbour));

			temp->gVal = cost;

			openNodes.insert(neighbour);

			temp->parent = const_cast<Node*>(&(*current));
		}
	}

	template <typename VoxelType>
	float AStarPathfinder<VoxelType>::SixConnectedCost(const Vector3DInt16& a, const Vector3DInt16& b)
	{
		//This is the only heuristic I'm sure of - just use the manhatten distance for the 6-connected case.
		uint16_t faceSteps = abs(a.getX()-b.getX()) + abs(a.getY()-b.getY()) + abs(a.getZ()-b.getZ());

		return faceSteps * 1.0f;
	}

	template <typename VoxelType>
	float AStarPathfinder<VoxelType>::EighteenConnectedCost(const Vector3DInt16& a, const Vector3DInt16& b)
	{
		//I'm not sure of the correct heuristic for the 18-connected case, so I'm just letting it fall through to the 
		//6-connected case. This means 'h' will be bigger than it should be, resulting in a faster path which may not 
		//actually be the shortest one. If you have a correct heuristic for the 18-connected case then please let me know.

		return SixConnectedCost(a,b);
	}

	template <typename VoxelType>
	float AStarPathfinder<VoxelType>::TwentySixConnectedCost(const Vector3DInt16& a, const Vector3DInt16& b)
	{
		//Can't say I'm certain about this heuristic - if anyone has
		//a better idea of what it should be then please let me know.
		uint16_t array[3];
		array[0] = abs(a.getX() - b.getX());
		array[1] = abs(a.getY() - b.getY());
		array[2] = abs(a.getZ() - b.getZ());
		
		//Maybe this is better implemented directly
		//using three compares and two swaps... but not
		//until the profiler says so.
		std::sort(&array[0], &array[3]);

		uint16_t cornerSteps = array[0];
		uint16_t edgeSteps = array[1] - array[0];
		uint16_t faceSteps = array[2] - array[1];

		return cornerSteps * sqrt_3 + edgeSteps * sqrt_2 + faceSteps * sqrt_1;
	}

	template <typename VoxelType>
	float AStarPathfinder<VoxelType>::computeH(const Vector3DInt16& a, const Vector3DInt16& b)
	{
		float hVal;
			
		switch(m_params.connectivity)
		{
		case TwentySixConnected:
			hVal = TwentySixConnectedCost(a, b);
			break;
		case EighteenConnected:
			hVal = EighteenConnectedCost(a, b);
			break;
		case SixConnected:				
			hVal = SixConnectedCost(a, b);
			break;
		default:
			assert(false); //Invalid case.
		}

		//Sanity checks in debug mode. These can come out eventually, but I
		//want to make sure that the heuristics I've come up with make sense.
		assert((a-b).length() <= TwentySixConnectedCost(a,b));
		assert(TwentySixConnectedCost(a,b) <= EighteenConnectedCost(a,b));
		assert(EighteenConnectedCost(a,b) <= SixConnectedCost(a,b));

		//Apply the bias to the computed h value;
		hVal *= m_params.hBias;

		//Having computed hVal, we now apply some random bias to break ties.
		//This needs to be deterministic on the input position. This random
		//bias means it is much les likely that two paths are exactly the same
		//length, and so far fewer nodes must be expanded to find the shortest path.
		//See http://theory.stanford.edu/~amitp/GameProgramming/Heuristics.html#S12
		polyvox_hash<uint32_t> uint32Hash;
		uint32_t hashValX = uint32Hash(a.getX());
		uint32_t hashValY = uint32Hash(a.getY());
		uint32_t hashValZ = uint32Hash(a.getZ());
		uint32_t hashVal = hashValX ^ hashValY ^ hashValZ;
		//Stop hashVal going over 65535, and divide by 1000000 to make sure it is small.
		hashVal &= 0x0000FFFF;
		float fHash = hashVal / 1000000.0f;

		//Apply the hash and return
		hVal += fHash;
		return hVal;
	}
}