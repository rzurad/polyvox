/*******************************************************************************
Copyright (c) 2010 Matt Williams

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

#include "TestCubicSurfaceExtractor.h"

#include "PolyVoxCore/Density.h"
#include "PolyVoxCore/Material.h"
#include "PolyVoxCore/MaterialDensityPair.h"
#include "PolyVoxCore/RawVolume.h"
#include "PolyVoxCore/SimpleVolume.h"
#include "PolyVoxCore/CubicSurfaceExtractor.h"

#include <QtTest>

using namespace PolyVox;

template<typename _VoxelType>
class CustomIsQuadNeeded
{
public:
	typedef _VoxelType VoxelType;

	bool operator()(VoxelType back, VoxelType front, VoxelType& materialToUse)
	{
		// Not a useful test - it just does something different 
		// to the DefaultIsQuadNeeded so we can check it compiles.
		if ((back > 1) && (front <= 1))
		{
			materialToUse = static_cast<VoxelType>(back);
			return true;
		}
		else
		{
			return false;
		}
	}
};

// Runs the surface extractor for a given type. 
template <typename VolumeType>
VolumeType* createAndFillVolumeWithNoise(int32_t iVolumeSideLength, typename VolumeType::VoxelType minValue, typename VolumeType::VoxelType maxValue)
{
	//Create empty volume
	VolumeType* volData = new VolumeType(Region(Vector3DInt32(0, 0, 0), Vector3DInt32(iVolumeSideLength - 1, iVolumeSideLength - 1, iVolumeSideLength - 1)));

	// Seed generator for consistency between runs.
	srand(12345);

	//Fill the volume with data
	for (int32_t z = 0; z < iVolumeSideLength; z++)
	{
		for (int32_t y = 0; y < iVolumeSideLength; y++)
		{
			for (int32_t x = 0; x < iVolumeSideLength; x++)
			{
				if (minValue == maxValue)
				{
					// In this case we are filling the whole volume with a single value.
					volData->setVoxelAt(x, y, z, minValue);
				}
				else
				{
					// Otherwise we write random voxel values between zero and the requested maximum
					int voxelValue = (rand() % (maxValue - minValue + 1)) + minValue;
					volData->setVoxelAt(x, y, z, static_cast<typename VolumeType::VoxelType>(voxelValue));
				}
			}
		}
	}

	return volData;
}

void TestCubicSurfaceExtractor::testBehaviour()
{
	// Test with default mesh and contoller types.
	auto uint8Vol = createAndFillVolumeWithNoise< SimpleVolume<uint8_t> >(32, 0, 2);
	auto uint8Mesh = extractCubicMesh(uint8Vol, uint8Vol->getEnclosingRegion());
	QCOMPARE(uint8Mesh.getNoOfVertices(), uint32_t(57687));
	QCOMPARE(uint8Mesh.getNoOfIndices(), uint32_t(216234));

	// Test with default mesh type but user-provided controller.
	auto int8Vol = createAndFillVolumeWithNoise< SimpleVolume<int8_t> >(32, 0, 2);
	auto int8Mesh = extractCubicMesh(int8Vol, int8Vol->getEnclosingRegion(), CustomIsQuadNeeded<int8_t>());
	QCOMPARE(int8Mesh.getNoOfVertices(), uint32_t(29027));
	QCOMPARE(int8Mesh.getNoOfIndices(), uint32_t(178356));

	// Test with default controller but user-provided mesh.
	auto uint32Vol = createAndFillVolumeWithNoise< SimpleVolume<uint32_t> >(32, 0, 2);
	Mesh< CubicVertex< uint32_t >, uint16_t > uint32Mesh;
	extractCubicMeshCustom(uint32Vol, uint32Vol->getEnclosingRegion(), &uint32Mesh);
	QCOMPARE(uint32Mesh.getNoOfVertices(), uint16_t(57687));
	QCOMPARE(uint32Mesh.getNoOfIndices(), uint32_t(216234));

	// Test with both mesh and controller being provided by the user.
	auto int32Vol = createAndFillVolumeWithNoise< SimpleVolume<int32_t> >(32, 0, 2);
	Mesh< CubicVertex< int32_t >, uint16_t > int32Mesh;
	extractCubicMeshCustom(int32Vol, int32Vol->getEnclosingRegion(), &int32Mesh, CustomIsQuadNeeded<int32_t>());
	QCOMPARE(int32Mesh.getNoOfVertices(), uint16_t(29027));
	QCOMPARE(int32Mesh.getNoOfIndices(), uint32_t(178356));
}

void TestCubicSurfaceExtractor::testEmptyVolumePerformance()
{
	auto emptyVol = createAndFillVolumeWithNoise< SimpleVolume<uint32_t> >(128, 0, 0);
	Mesh< CubicVertex< uint32_t >, uint16_t > emptyMesh;
	QBENCHMARK{ extractCubicMeshCustom(emptyVol, Region(32, 32, 32, 63, 63, 63), &emptyMesh); }
	QCOMPARE(emptyMesh.getNoOfVertices(), uint16_t(0));
}

void TestCubicSurfaceExtractor::testNoiseVolumePerformance()
{
	auto noiseVol = createAndFillVolumeWithNoise< SimpleVolume<uint32_t> >(128, 0, 2);
	Mesh< CubicVertex< uint32_t >, uint16_t > noiseMesh;
	QBENCHMARK{ extractCubicMeshCustom(noiseVol, Region(32, 32, 32, 63, 63, 63), &noiseMesh); }
	QCOMPARE(noiseMesh.getNoOfVertices(), uint16_t(57729));
}

QTEST_MAIN(TestCubicSurfaceExtractor)
