/******************************************************************************
This file is part of a voxel plugin for OGRE
Copyright (C) 2006  David Williams

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
******************************************************************************/

#include "GradientEstimators.h"
#include "IndexedSurfacePatch.h"
#include "MarchingCubesTables.h"
#include "PolyVoxSceneManager.h"
#include "RegionGeometry.h"
#include "SurfaceVertex.h"
#include "Vector.h"
#include "Volume.h"
#include "VolumeIterator.h"

using namespace boost;

namespace PolyVox
{

	//////////////////////////////////////////////////////////////////////////
	// PolyVoxSceneManager
	//////////////////////////////////////////////////////////////////////////
	PolyVoxSceneManager::PolyVoxSceneManager()
		:volumeData(0)
		,useNormalSmoothing(false)
		,normalSmoothingFilterSize(1)
		,m_normalGenerationMethod(SOBEL)
		,m_bHaveGeneratedMeshes(false)
	{	
		//sceneNodes.clear();
	}

	PolyVoxSceneManager::~PolyVoxSceneManager()
	{
	}

	std::list<RegionGeometry> PolyVoxSceneManager::getChangedRegionGeometry(void)
	{
		std::list<RegionGeometry> listChangedRegionGeometry;

		//Regenerate meshes.
		for(uint16_t regionZ = 0; regionZ < POLYVOX_VOLUME_SIDE_LENGTH_IN_REGIONS; ++regionZ)
		//for(uint16_t regionZ = 6; regionZ < 7; ++regionZ)
		{		
			for(uint16_t regionY = 0; regionY < POLYVOX_VOLUME_SIDE_LENGTH_IN_REGIONS; ++regionY)
			//for(uint16_t regionY = 6; regionY < 7; ++regionY)
			{
				for(uint16_t regionX = 0; regionX < POLYVOX_VOLUME_SIDE_LENGTH_IN_REGIONS; ++regionX)
				//for(uint16_t regionX = 3; regionX < 4; ++regionX)
				{
					if(surfaceUpToDate[regionX][regionY][regionZ] == false)
					{
						//Generate the surface
						RegionGeometry regionGeometry;
						regionGeometry.m_patchSingleMaterial = new IndexedSurfacePatch(false);
						regionGeometry.m_patchMultiMaterial = new IndexedSurfacePatch(true);
						regionGeometry.m_v3dRegionPosition = Vector3DInt32(regionX, regionY, regionZ);

						generateMeshDataForRegion(regionX,regionY,regionZ, regionGeometry.m_patchSingleMaterial, regionGeometry.m_patchMultiMaterial);

						regionGeometry.m_bContainsSingleMaterialPatch = regionGeometry.m_patchSingleMaterial->getVertices().size() > 0;
						regionGeometry.m_bContainsMultiMaterialPatch = regionGeometry.m_patchMultiMaterial->getVertices().size() > 0;
						regionGeometry.m_bIsEmpty = ((regionGeometry.m_patchSingleMaterial->getVertices().size() == 0) && (regionGeometry.m_patchMultiMaterial->getIndices().size() == 0));

						listChangedRegionGeometry.push_back(regionGeometry);
					}
				}
			}
		}

		return listChangedRegionGeometry;
	}

	void PolyVoxSceneManager::setAllUpToDateFlagsTo(bool newUpToDateValue)
	{
		for(uint16_t blockZ = 0; blockZ < POLYVOX_VOLUME_SIDE_LENGTH_IN_REGIONS; ++blockZ)
		{
			for(uint16_t blockY = 0; blockY < POLYVOX_VOLUME_SIDE_LENGTH_IN_REGIONS; ++blockY)
			{
				for(uint16_t blockX = 0; blockX < POLYVOX_VOLUME_SIDE_LENGTH_IN_REGIONS; ++blockX)
				{
					surfaceUpToDate[blockX][blockY][blockZ] = newUpToDateValue;
				}
			}
		}
	}

	void PolyVoxSceneManager::createSphereAt(Vector3DFloat centre, float radius, uint8_t value, bool painting)
	{
		int firstX = static_cast<int>(std::floor(centre.x() - radius));
		int firstY = static_cast<int>(std::floor(centre.y() - radius));
		int firstZ = static_cast<int>(std::floor(centre.z() - radius));

		int lastX = static_cast<int>(std::ceil(centre.x() + radius));
		int lastY = static_cast<int>(std::ceil(centre.y() + radius));
		int lastZ = static_cast<int>(std::ceil(centre.z() + radius));

		float radiusSquared = radius * radius;

		//Check bounds
		firstX = std::max(firstX,0);
		firstY = std::max(firstY,0);
		firstZ = std::max(firstZ,0);

		lastX = std::min(lastX,int(POLYVOX_VOLUME_SIDE_LENGTH-1));
		lastY = std::min(lastY,int(POLYVOX_VOLUME_SIDE_LENGTH-1));
		lastZ = std::min(lastZ,int(POLYVOX_VOLUME_SIDE_LENGTH-1));

		VolumeIterator<boost::uint8_t> volIter(*volumeData);
		volIter.setValidRegion(firstX,firstY,firstZ,lastX,lastY,lastZ);
		volIter.setPosition(firstX,firstY,firstZ);
		while(volIter.isValidForRegion())
		{
			//if((volIter.getPosX()*volIter.getPosX()+volIter.getPosY()*volIter.getPosY()+volIter.getPosZ()*volIter.getPosZ()) < radiusSquared)
			if((centre - Vector3DFloat(volIter.getPosX(),volIter.getPosY(),volIter.getPosZ())).lengthSquared() <= radiusSquared)
			{
				if(painting)
				{
					if(volIter.getVoxel() != 0)
					{
						volIter.setVoxel(value);
						//volIter.setVoxelAt(volIter.getPosX(),volIter.getPosY(),volIter.getPosZ(),value);
					}
				}
				else
				{
					volIter.setVoxel(value);
					//volIter.setVoxelAt(volIter.getPosX(),volIter.getPosY(),volIter.getPosZ(),value);
				}
				//markVoxelChanged(volIter.getPosX(),volIter.getPosY(),volIter.getPosZ()); //FIXME - create a version of this function to mark larger regions at a time.
			}
			volIter.moveForwardInRegion();
		}
		markRegionChanged(firstX,firstY,firstZ,lastX,lastY,lastZ);
	}

	void PolyVoxSceneManager::generateLevelVolume(void)
	{
		//volumeData = VolumePtr(new Volume);
		volumeData = new Volume<boost::uint8_t>();
		VolumeIterator<boost::uint8_t> volIter(*volumeData);
		for(uint16_t z = 0; z < POLYVOX_VOLUME_SIDE_LENGTH; ++z)
		{
			for(uint16_t y = 0; y < POLYVOX_VOLUME_SIDE_LENGTH; ++y)
			{
				for(uint16_t x = 0; x < POLYVOX_VOLUME_SIDE_LENGTH; ++x)
				{
					volIter.setPosition(x,y,z);
					if((x/16+y/16+z/16)%2 == 0)
						volIter.setVoxel(4);
					else
						volIter.setVoxel(8);
				}
			}
		}		

		for(uint16_t z = 0; z < POLYVOX_VOLUME_SIDE_LENGTH; ++z)
		{
			for(uint16_t y = 0; y < POLYVOX_VOLUME_SIDE_LENGTH; ++y)
			{
				for(uint16_t x = 0; x < POLYVOX_VOLUME_SIDE_LENGTH; ++x)
				{
					if(
						(z<62)||
						(z>193)||
						(y<78)||
						(y>177)||
						(x<30)||
						(x>225)
						)
					{
						volIter.setPosition(x,y,z);
						volIter.setVoxel(2);
					}
				}
			}
		}		

		//Rooms
		Vector3DFloat centre(128,128,128);
		Vector3DFloat v3dSize(192,96,128);

		uint16_t uHalfX = static_cast<uint16_t>(v3dSize.x() / 2);
		uint16_t uHalfY = static_cast<uint16_t>(v3dSize.y() / 2);
		uint16_t uHalfZ = static_cast<uint16_t>(v3dSize.z() / 2);

		for(uint16_t z = static_cast<uint16_t>(centre.z()) - uHalfZ; z < static_cast<uint16_t>(centre.z()) + uHalfZ; z++)
		{
			for(uint16_t y = static_cast<uint16_t>(centre.y()) - uHalfY; y < static_cast<uint16_t>(centre.y()) + uHalfY; y++)
			{
				for(uint16_t x = static_cast<uint16_t>(centre.x()) - uHalfX; x < static_cast<uint16_t>(centre.x()) + uHalfX; x++)
				{
					volIter.setPosition(x,y,z);
					volIter.setVoxel(0);
				}
			}
		}

		for(uint16_t z = 0; z < POLYVOX_VOLUME_SIDE_LENGTH; ++z)
		{
			for(uint16_t y = 0; y < POLYVOX_VOLUME_SIDE_LENGTH; ++y)
			{
				for(uint16_t x = 0; x < POLYVOX_VOLUME_SIDE_LENGTH; ++x)
				{
					if(
						(x%64 < 8) &&
						(y < 128) &&
						(z>=62)&&
						(z<=193)&&
						(y>=78)&&
						(y<=177)&&
						(x>=30)&&
						(x<=225)
						)
					{
						volIter.setPosition(x,y,z);
						volIter.setVoxel(1);
					}
				}
			}
		}
	}

	void PolyVoxSceneManager::generateMeshDataForRegion(const uint16_t regionX, const uint16_t regionY, const uint16_t regionZ, IndexedSurfacePatch* singleMaterialPatch, IndexedSurfacePatch* multiMaterialPatch) const
	{	
		//First and last voxels in the region
		const uint16_t firstX = regionX * POLYVOX_REGION_SIDE_LENGTH;
		const uint16_t firstY = regionY * POLYVOX_REGION_SIDE_LENGTH;
		const uint16_t firstZ = regionZ * POLYVOX_REGION_SIDE_LENGTH;
		const uint16_t lastX = (std::min)(firstX + POLYVOX_REGION_SIDE_LENGTH-1,static_cast<uint32_t>(POLYVOX_VOLUME_SIDE_LENGTH-2));
		const uint16_t lastY = (std::min)(firstY + POLYVOX_REGION_SIDE_LENGTH-1,static_cast<uint32_t>(POLYVOX_VOLUME_SIDE_LENGTH-2));
		const uint16_t lastZ = (std::min)(firstZ + POLYVOX_REGION_SIDE_LENGTH-1,static_cast<uint32_t>(POLYVOX_VOLUME_SIDE_LENGTH-2));

		//Offset from lower block corner
		const Vector3DFloat offset(firstX,firstY,firstZ);

		Vector3DFloat vertlist[12];
		uint8_t vertMaterials[12];
		VolumeIterator<boost::uint8_t> volIter(*volumeData);
		volIter.setValidRegion(firstX,firstY,firstZ,lastX,lastY,lastZ);

		//////////////////////////////////////////////////////////////////////////
		//Get mesh data
		//////////////////////////////////////////////////////////////////////////

		//Iterate over each cell in the region
		for(volIter.setPosition(firstX,firstY,firstZ);volIter.isValidForRegion();volIter.moveForwardInRegion())
		{		
			//Current position
			const uint16_t x = volIter.getPosX();
			const uint16_t y = volIter.getPosY();
			const uint16_t z = volIter.getPosZ();

			//Voxels values
			const uint8_t v000 = volIter.getVoxel();
			const uint8_t v100 = volIter.peekVoxel1px0py0pz();
			const uint8_t v010 = volIter.peekVoxel0px1py0pz();
			const uint8_t v110 = volIter.peekVoxel1px1py0pz();
			const uint8_t v001 = volIter.peekVoxel0px0py1pz();
			const uint8_t v101 = volIter.peekVoxel1px0py1pz();
			const uint8_t v011 = volIter.peekVoxel0px1py1pz();
			const uint8_t v111 = volIter.peekVoxel1px1py1pz();

			//Determine the index into the edge table which tells us which vertices are inside of the surface
			uint8_t iCubeIndex = 0;

			if (v000 == 0) iCubeIndex |= 1;
			if (v100 == 0) iCubeIndex |= 2;
			if (v110 == 0) iCubeIndex |= 4;
			if (v010 == 0) iCubeIndex |= 8;
			if (v001 == 0) iCubeIndex |= 16;
			if (v101 == 0) iCubeIndex |= 32;
			if (v111 == 0) iCubeIndex |= 64;
			if (v011 == 0) iCubeIndex |= 128;

			/* Cube is entirely in/out of the surface */
			if (edgeTable[iCubeIndex] == 0)
			{
				continue;
			}

			/* Find the vertices where the surface intersects the cube */
			if (edgeTable[iCubeIndex] & 1)
			{
				vertlist[0].setX(x + 0.5f);
				vertlist[0].setY(y);
				vertlist[0].setZ(z);
				vertMaterials[0] = v000 | v100; //Because one of these is 0, the or operation takes the max.
			}
			if (edgeTable[iCubeIndex] & 2)
			{
				vertlist[1].setX(x + 1.0f);
				vertlist[1].setY(y + 0.5f);
				vertlist[1].setZ(z);
				vertMaterials[1] = v100 | v110;
			}
			if (edgeTable[iCubeIndex] & 4)
			{
				vertlist[2].setX(x + 0.5f);
				vertlist[2].setY(y + 1.0f);
				vertlist[2].setZ(z);
				vertMaterials[2] = v010 | v110;
			}
			if (edgeTable[iCubeIndex] & 8)
			{
				vertlist[3].setX(x);
				vertlist[3].setY(y + 0.5f);
				vertlist[3].setZ(z);
				vertMaterials[3] = v000 | v010;
			}
			if (edgeTable[iCubeIndex] & 16)
			{
				vertlist[4].setX(x + 0.5f);
				vertlist[4].setY(y);
				vertlist[4].setZ(z + 1.0f);
				vertMaterials[4] = v001 | v101;
			}
			if (edgeTable[iCubeIndex] & 32)
			{
				vertlist[5].setX(x + 1.0f);
				vertlist[5].setY(y + 0.5f);
				vertlist[5].setZ(z + 1.0f);
				vertMaterials[5] = v101 | v111;
			}
			if (edgeTable[iCubeIndex] & 64)
			{
				vertlist[6].setX(x + 0.5f);
				vertlist[6].setY(y + 1.0f);
				vertlist[6].setZ(z + 1.0f);
				vertMaterials[6] = v011 | v111;
			}
			if (edgeTable[iCubeIndex] & 128)
			{
				vertlist[7].setX(x);
				vertlist[7].setY(y + 0.5f);
				vertlist[7].setZ(z + 1.0f);
				vertMaterials[7] = v001 | v011;
			}
			if (edgeTable[iCubeIndex] & 256)
			{
				vertlist[8].setX(x);
				vertlist[8].setY(y);
				vertlist[8].setZ(z + 0.5f);
				vertMaterials[8] = v000 | v001;
			}
			if (edgeTable[iCubeIndex] & 512)
			{
				vertlist[9].setX(x + 1.0f);
				vertlist[9].setY(y);
				vertlist[9].setZ(z + 0.5f);
				vertMaterials[9] = v100 | v101;
			}
			if (edgeTable[iCubeIndex] & 1024)
			{
				vertlist[10].setX(x + 1.0f);
				vertlist[10].setY(y + 1.0f);
				vertlist[10].setZ(z + 0.5f);
				vertMaterials[10] = v110 | v111;
			}
			if (edgeTable[iCubeIndex] & 2048)
			{
				vertlist[11].setX(x);
				vertlist[11].setY(y + 1.0f);
				vertlist[11].setZ(z + 0.5f);
				vertMaterials[11] = v010 | v011;
			}

			for (int i=0;triTable[iCubeIndex][i]!=-1;i+=3)
			{
				//The three vertices forming a triangle
				const Vector3DFloat vertex0 = vertlist[triTable[iCubeIndex][i  ]] - offset;
				const Vector3DFloat vertex1 = vertlist[triTable[iCubeIndex][i+1]] - offset;
				const Vector3DFloat vertex2 = vertlist[triTable[iCubeIndex][i+2]] - offset;

				//Cast to floats and divide by two.
				//const Vector3DFloat vertex0AsFloat = (static_cast<Vector3DFloat>(vertex0) / 2.0f) - offset;
				//const Vector3DFloat vertex1AsFloat = (static_cast<Vector3DFloat>(vertex1) / 2.0f) - offset;
				//const Vector3DFloat vertex2AsFloat = (static_cast<Vector3DFloat>(vertex2) / 2.0f) - offset;

				const uint8_t material0 = vertMaterials[triTable[iCubeIndex][i  ]];
				const uint8_t material1 = vertMaterials[triTable[iCubeIndex][i+1]];
				const uint8_t material2 = vertMaterials[triTable[iCubeIndex][i+2]];


				//If all the materials are the same, we just need one triangle for that material with all the alphas set high.
				if((material0 == material1) && (material1 == material2))
				{
					SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1,1.0);
					SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1,1.0);
					SurfaceVertex surfaceVertex2Alpha1(vertex2,material2 + 0.1,1.0);
					singleMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
				}
				else if(material0 == material1)
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1,1.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1,1.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1,0.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material2 + 0.1,0.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material2 + 0.1,0.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material2 + 0.1,1.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}
				else if(material0 == material2)
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1,1.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1,0.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1,1.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material1 + 0.1,0.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1,1.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material1 + 0.1,0.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}
				else if(material1 == material2)
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material1 + 0.1,0.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1,1.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material1 + 0.1,1.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1,1.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1,0.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1,0.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}
				else
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1,1.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1,0.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1,0.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material1 + 0.1,0.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1,1.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material1 + 0.1,0.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material2 + 0.1,0.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material2 + 0.1,0.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material2 + 0.1,1.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}
				//If there not all the same, we need one triangle for each unique material.
				//We'll also need some vertices with low alphas for blending.
				/*else 
				{
				SurfaceVertex surfaceVertex0Alpha0(vertex0,0.0);
				SurfaceVertex surfaceVertex1Alpha0(vertex1,0.0);
				SurfaceVertex surfaceVertex2Alpha0(vertex2,0.0);

				if(material0 == material1)
				{
				surfacePatchMapResult[material0]->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha0);
				surfacePatchMapResult[material2]->addTriangle(surfaceVertex0Alpha0, surfaceVertex1Alpha0, surfaceVertex2Alpha1);
				}
				else if(material1 == material2)
				{
				surfacePatchMapResult[material1]->addTriangle(surfaceVertex0Alpha0, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
				surfacePatchMapResult[material0]->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha0, surfaceVertex2Alpha0);
				}
				else if(material2 == material0)
				{
				surfacePatchMapResult[material0]->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha0, surfaceVertex2Alpha1);
				surfacePatchMapResult[material1]->addTriangle(surfaceVertex0Alpha0, surfaceVertex1Alpha1, surfaceVertex2Alpha0);
				}
				else
				{
				surfacePatchMapResult[material0]->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha0, surfaceVertex2Alpha0);
				surfacePatchMapResult[material1]->addTriangle(surfaceVertex0Alpha0, surfaceVertex1Alpha1, surfaceVertex2Alpha0);
				surfacePatchMapResult[material2]->addTriangle(surfaceVertex0Alpha0, surfaceVertex1Alpha0, surfaceVertex2Alpha1);
				}
				}*/
			}//For each triangle
		}//For each cell

		//FIXME - can it happen that we have no vertices or triangles? Should exit early?


		//for(std::map<uint8_t, IndexedSurfacePatch*>::iterator iterPatch = surfacePatchMapResult.begin(); iterPatch != surfacePatchMapResult.end(); ++iterPatch)
		{

			std::vector<SurfaceVertex>::iterator iterSurfaceVertex = singleMaterialPatch->getVertices().begin();
			while(iterSurfaceVertex != singleMaterialPatch->getVertices().end())
			{
				Vector3DFloat tempNormal = computeNormal(static_cast<Vector3DFloat>(iterSurfaceVertex->getPosition() + offset), CENTRAL_DIFFERENCE);
				const_cast<SurfaceVertex&>(*iterSurfaceVertex).setNormal(tempNormal);
				++iterSurfaceVertex;
			}

			iterSurfaceVertex = multiMaterialPatch->getVertices().begin();
			while(iterSurfaceVertex != multiMaterialPatch->getVertices().end())
			{
				Vector3DFloat tempNormal = computeNormal(static_cast<Vector3DFloat>(iterSurfaceVertex->getPosition() + offset), CENTRAL_DIFFERENCE);
				const_cast<SurfaceVertex&>(*iterSurfaceVertex).setNormal(tempNormal);
				++iterSurfaceVertex;
			}

			uint16_t noOfRemovedVertices = 0;
			//do
			{
				//noOfRemovedVertices = iterPatch->second.decimate();
			}
			//while(noOfRemovedVertices > 10); //We don't worry about the last few vertices - it's not worth the overhead of calling the function.
		}

		//return singleMaterialPatch;
	}

	Vector3DFloat PolyVoxSceneManager::computeNormal(const Vector3DFloat& position, NormalGenerationMethod normalGenerationMethod) const
	{
		VolumeIterator<boost::uint8_t> volIter(*volumeData); //FIXME - save this somewhere - could be expensive to create?

		const float posX = position.x();
		const float posY = position.y();
		const float posZ = position.z();

		const uint16_t floorX = static_cast<uint16_t>(posX);
		const uint16_t floorY = static_cast<uint16_t>(posY);
		const uint16_t floorZ = static_cast<uint16_t>(posZ);

		Vector3DFloat result;


		if(normalGenerationMethod == SOBEL)
		{
			volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ));
			const Vector3DFloat gradFloor = computeSobelGradient(volIter);
			if((posX - floorX) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX+1.0),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ));
			}
			if((posY - floorY) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY+1.0),static_cast<uint16_t>(posZ));
			}
			if((posZ - floorZ) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ+1.0));					
			}
			const Vector3DFloat gradCeil = computeSobelGradient(volIter);
			result = ((gradFloor + gradCeil) * -1.0);
			if(result.lengthSquared() < 0.0001)
			{
				//Operation failed - fall back on simple gradient estimation
				normalGenerationMethod = SIMPLE;
			}
		}
		if(normalGenerationMethod == CENTRAL_DIFFERENCE)
		{
			volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ));
			const Vector3DFloat gradFloor = computeCentralDifferenceGradient(volIter);
			if((posX - floorX) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX+1.0),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ));
			}
			if((posY - floorY) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY+1.0),static_cast<uint16_t>(posZ));
			}
			if((posZ - floorZ) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ+1.0));					
			}
			const Vector3DFloat gradCeil = computeCentralDifferenceGradient(volIter);
			result = ((gradFloor + gradCeil) * -1.0);
			if(result.lengthSquared() < 0.0001)
			{
				//Operation failed - fall back on simple gradient estimation
				normalGenerationMethod = SIMPLE;
			}
		}
		if(normalGenerationMethod == SIMPLE)
		{
			volIter.setPosition(static_cast<uint16_t>(posX),static_cast<uint16_t>(posY),static_cast<uint16_t>(posZ));
			const uint8_t uFloor = volIter.getVoxel() > 0 ? 1 : 0;
			if((posX - floorX) > 0.25) //The result should be 0.0 or 0.5
			{					
				uint8_t uCeil = volIter.peekVoxel1px0py0pz() > 0 ? 1 : 0;
				result = Vector3DFloat(uFloor - uCeil,0.0,0.0);
			}
			else if((posY - floorY) > 0.25) //The result should be 0.0 or 0.5
			{
				uint8_t uCeil = volIter.peekVoxel0px1py0pz() > 0 ? 1 : 0;
				result = Vector3DFloat(0.0,uFloor - uCeil,0.0);
			}
			else if((posZ - floorZ) > 0.25) //The result should be 0.0 or 0.5
			{
				uint8_t uCeil = volIter.peekVoxel0px0py1pz() > 0 ? 1 : 0;
				result = Vector3DFloat(0.0, 0.0,uFloor - uCeil);					
			}
		}
		return result;
	}

	void PolyVoxSceneManager::markVoxelChanged(uint16_t x, uint16_t y, uint16_t z)
	{
		//If we are not on a boundary, just mark one region.
		if((x % POLYVOX_REGION_SIDE_LENGTH != 0) &&
			(x % POLYVOX_REGION_SIDE_LENGTH != POLYVOX_REGION_SIDE_LENGTH-1) &&
			(y % POLYVOX_REGION_SIDE_LENGTH != 0) &&
			(y % POLYVOX_REGION_SIDE_LENGTH != POLYVOX_REGION_SIDE_LENGTH-1) &&
			(z % POLYVOX_REGION_SIDE_LENGTH != 0) &&
			(z % POLYVOX_REGION_SIDE_LENGTH != POLYVOX_REGION_SIDE_LENGTH-1))
		{
			surfaceUpToDate[x >> POLYVOX_REGION_SIDE_LENGTH_POWER][y >> POLYVOX_REGION_SIDE_LENGTH_POWER][z >> POLYVOX_REGION_SIDE_LENGTH_POWER] = false;
		}
		else //Mark surrounding block as well
		{
			const uint16_t regionX = x >> POLYVOX_REGION_SIDE_LENGTH_POWER;
			const uint16_t regionY = y >> POLYVOX_REGION_SIDE_LENGTH_POWER;
			const uint16_t regionZ = z >> POLYVOX_REGION_SIDE_LENGTH_POWER;

			const uint16_t minRegionX = (std::max)(uint16_t(0),uint16_t(regionX-1));
			const uint16_t minRegionY = (std::max)(uint16_t(0),uint16_t(regionY-1));
			const uint16_t minRegionZ = (std::max)(uint16_t(0),uint16_t(regionZ-1));

			const uint16_t maxRegionX = (std::min)(uint16_t(POLYVOX_VOLUME_SIDE_LENGTH_IN_REGIONS-1),uint16_t(regionX+1));
			const uint16_t maxRegionY = (std::min)(uint16_t(POLYVOX_VOLUME_SIDE_LENGTH_IN_REGIONS-1),uint16_t(regionY+1));
			const uint16_t maxRegionZ = (std::min)(uint16_t(POLYVOX_VOLUME_SIDE_LENGTH_IN_REGIONS-1),uint16_t(regionZ+1));

			for(uint16_t zCt = minRegionZ; zCt <= maxRegionZ; zCt++)
			{
				for(uint16_t yCt = minRegionY; yCt <= maxRegionY; yCt++)
				{
					for(uint16_t xCt = minRegionX; xCt <= maxRegionX; xCt++)
					{
						surfaceUpToDate[xCt][yCt][zCt] = false;
					}
				}
			}
		}
	}

	void PolyVoxSceneManager::markRegionChanged(uint16_t firstX, uint16_t firstY, uint16_t firstZ, uint16_t lastX, uint16_t lastY, uint16_t lastZ)
	{
		const uint16_t firstRegionX = firstX >> POLYVOX_REGION_SIDE_LENGTH_POWER;
		const uint16_t firstRegionY = firstY >> POLYVOX_REGION_SIDE_LENGTH_POWER;
		const uint16_t firstRegionZ = firstZ >> POLYVOX_REGION_SIDE_LENGTH_POWER;

		const uint16_t lastRegionX = lastX >> POLYVOX_REGION_SIDE_LENGTH_POWER;
		const uint16_t lastRegionY = lastY >> POLYVOX_REGION_SIDE_LENGTH_POWER;
		const uint16_t lastRegionZ = lastZ >> POLYVOX_REGION_SIDE_LENGTH_POWER;

		for(uint16_t zCt = firstRegionZ; zCt <= lastRegionZ; zCt++)
		{
			for(uint16_t yCt = firstRegionY; yCt <= lastRegionY; yCt++)
			{
				for(uint16_t xCt = firstRegionX; xCt <= lastRegionX; xCt++)
				{
					surfaceUpToDate[xCt][yCt][zCt] = false;
				}
			}
		}
	}

	uint16_t PolyVoxSceneManager::getSideLength(void)
	{
		return POLYVOX_VOLUME_SIDE_LENGTH;
	}

	uint8_t PolyVoxSceneManager::getMaterialIndexAt(uint16_t uX, uint16_t uY, uint16_t uZ)
	{
		if(volumeData->containsPoint(Vector3DInt32(uX,uY,uZ),0))
		{
			VolumeIterator<boost::uint8_t> volIter(*volumeData);
			volIter.setPosition(uX,uY,uZ);
			return volIter.getVoxel();
		}
		else
		{
			return 0;
		}
	}

	void PolyVoxSceneManager::setNormalGenerationMethod(NormalGenerationMethod method)
	{
		m_normalGenerationMethod = method;
	}

	bool PolyVoxSceneManager::containsPoint(Vector3DFloat pos, float boundary)
	{
		return volumeData->containsPoint(pos, boundary);
	}

	bool PolyVoxSceneManager::containsPoint(Vector3DInt32 pos, uint16_t boundary)
	{
		return volumeData->containsPoint(pos, boundary);
	}

	/*

	void PolyVoxSceneManager::setAxisVisible(bool visible)
	{
		if(m_axisNode)
			m_axisNode->setVisible(visible);
	}*/
}
