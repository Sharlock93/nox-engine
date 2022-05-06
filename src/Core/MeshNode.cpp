#include <Core/MeshNode.h>
#include <glm/gtx/matrix_interpolation.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <fstream>

using namespace NoxEngine;

MeshNode::MeshNode(){ }

MeshNode::MeshNode(std::istream& stream)
{
	size_t nameSize;
	stream.read((char*)&nameSize, sizeof(nameSize));
	name.resize(nameSize);
	stream.read((char*)&name[0], nameSize);

	stream.read((char*)&parent, sizeof(parent));
	if (parent != nullptr)
		parent->deserialize(stream);

	stream.read((char*)&transformation, sizeof(transformation));

	size_t meshIndexSize;
	stream.read((char*)&meshIndexSize, sizeof(meshIndexSize));
	meshIndex.resize(meshIndexSize);
	stream.read((char*)&meshIndex[0], meshIndexSize * sizeof(u32));

	size_t childSize;
	stream.read((char*)&childSize, sizeof(childSize));
	child.resize(childSize);
	for (u32 i = 0; i < childSize; i++)
	{
		child[i].deserialize(stream);
	}

	size_t nodeAnimSize;
	stream.read((char*)&nodeAnimSize, sizeof(nodeAnimSize));
	nodeAnimations.resize(nodeAnimSize);
	if(nodeAnimSize > 0)
		stream.read((char*)&nodeAnimations[0], nodeAnimSize * sizeof(aiNodeAnim*));

	size_t animationSize;
	stream.read((char*)&animationSize, sizeof(animationSize));
	nodeAnimTransformation.resize(animationSize);
	stream.read((char*)&nodeAnimTransformation[0], animationSize * sizeof(animationSize));
	for (size_t i = 0; i < animationSize; i++)
	{
		size_t keyframeSize;
		stream.read((char*)&keyframeSize, sizeof(keyframeSize));
		nodeAnimTransformation[i].resize(keyframeSize);
		stream.read((char*)&nodeAnimTransformation[i][0], keyframeSize * sizeof(mat4));
	}

	nodeAnimTranslationMatrices.resize(animationSize);
	stream.read((char*)&nodeAnimTranslationMatrices[0], animationSize * sizeof(animationSize));
	for (size_t i = 0; i < animationSize; i++)
	{
		size_t keyframeSize;
		stream.read((char*)&keyframeSize, sizeof(keyframeSize));
		nodeAnimTranslationMatrices[i].resize(keyframeSize);
		stream.read((char*)&nodeAnimTranslationMatrices[i][0], keyframeSize * sizeof(mat4));
	}

	eulerAngleXYZ.resize(animationSize);
	stream.read((char*)&eulerAngleXYZ[0], animationSize * sizeof(animationSize));
	for (size_t i = 0; i < animationSize; i++)
	{
		size_t keyframeSize;
		stream.read((char*)&keyframeSize, sizeof(keyframeSize));
		eulerAngleXYZ[i].resize(keyframeSize);
		stream.read((char*)&eulerAngleXYZ[i][0], keyframeSize * sizeof(glm::vec3));
	}

	nodeAnimRotationMatrices.resize(animationSize);
	stream.read((char*)&nodeAnimRotationMatrices[0], animationSize * sizeof(animationSize));
	for (size_t i = 0; i < animationSize; i++)
	{
		size_t keyframeSize;
		stream.read((char*)&keyframeSize, sizeof(keyframeSize));
		nodeAnimRotationMatrices[i].resize(keyframeSize);
		stream.read((char*)&nodeAnimRotationMatrices[i][0], keyframeSize * sizeof(mat4));
	}

	nodeAnimScalingMatrices.resize(animationSize);
	stream.read((char*)&nodeAnimScalingMatrices[0], animationSize * sizeof(animationSize));
	for (size_t i = 0; i < animationSize; i++)
	{
		size_t keyframeSize;
		stream.read((char*)&keyframeSize, sizeof(keyframeSize));
		nodeAnimScalingMatrices[i].resize(keyframeSize);
		stream.read((char*)&nodeAnimScalingMatrices[i][0], keyframeSize * sizeof(mat4));
	}

	maximumFrame.resize(animationSize);
	stream.read((char*)&maximumFrame[0], animationSize * sizeof(u32));
}

MeshNode::~MeshNode(){ }

// The idle transformation
glm::mat4 MeshNode::getTransformation()
{
	return transformation;
}

// Frame based transformation
glm::mat4 MeshNode::getTransformation(u32 frameIndex, u32 animationIndex)
{
	if (!hasAnimations())
		return getTransformation();

	// Frame based way to update transformation
	return nodeAnimTransformation[animationIndex][frameIndex];
}

// Linear interpolated transformation
glm::mat4 MeshNode::getTransformation(u32 frameIndex, u32 animationIndex, f32 accumulator, time_type timeStep, i32 whichTickFloor, i32 whichTickCeil)
{
	if (!hasAnimations())
		return getTransformation();

	glm::mat4 defaultTransformation(1);

	// Interpolate between two keyframe
	f32 ratio = (f32)(accumulator / timeStep);
	if (accumulator == 0 || timeStep == 0)
		ratio = 0;

	glm::mat4 matrixFloor = nodeAnimTransformation[animationIndex][whichTickFloor];
	glm::mat4 matrixCeil = nodeAnimTransformation[animationIndex][whichTickCeil];

	// Linear interpolation
	return ((float)ratio * matrixCeil) + (matrixFloor * (1.0f - (float)ratio));
	//return interpolatedTranslation * interpolatedRotation * interpolatedScale;
}

// Idle global transformation
glm::mat4 MeshNode::getGlobalTransformation()
{
	glm::mat4 transformatiom_temp(1);
	bool isRoot = this->parent == nullptr;
	if (isRoot)
	{
		return getTransformation();
	}

	transformatiom_temp = getTransformation() * transformatiom_temp;
	MeshNode* currentNode = parent;
	while (!isRoot)
	{
		transformatiom_temp = currentNode->getTransformation() * transformatiom_temp;

		if (currentNode->parent == nullptr)
		{
			isRoot = true;
			break;
		}

		currentNode = currentNode->parent;
	}

	return transformatiom_temp;
}

// Frame based global transformation
glm::mat4 MeshNode::getGlobalTransformation(u32 frameIndex, u32 animationIndex)
{
	glm::mat4 transformatiom_temp(1);
	bool isRoot = this->parent == nullptr;
	if (isRoot)
	{
		return getTransformation(frameIndex, animationIndex);
	}

	transformatiom_temp = getTransformation(frameIndex, animationIndex) * transformatiom_temp;
	MeshNode* currentNode = parent;
	while (!isRoot)
	{
		transformatiom_temp = currentNode->getTransformation(frameIndex, animationIndex) * transformatiom_temp;

		if (currentNode->parent == nullptr)
		{
			isRoot = true;
			break;
		}

		currentNode = currentNode->parent;
	}

	return transformatiom_temp;
}

// Linear interpolated global transformation
glm::mat4 MeshNode::getGlobalTransformation(u32 frameIndex, u32 animationIndex, f32 accumulator, time_type timeStep,
	i32 whichTickFloor, i32 whichTickCeil)
{
	glm::mat4 transformatiom_temp(1);
	bool isRoot = this->parent == nullptr;
	if (isRoot)
	{
		return getTransformation(frameIndex, animationIndex, accumulator, timeStep, whichTickFloor, whichTickCeil);
	}

	transformatiom_temp = getTransformation(frameIndex, animationIndex, accumulator, timeStep,
		whichTickFloor, whichTickCeil) * transformatiom_temp;
	MeshNode* currentNode = parent;
	while (!isRoot)
	{
		transformatiom_temp = 
			currentNode->getTransformation(frameIndex, animationIndex, accumulator, timeStep, 
				whichTickFloor, whichTickCeil)
			* transformatiom_temp;

		if (currentNode->parent == nullptr)
		{
			isRoot = true;
			break;
		}

		currentNode = currentNode->parent;
	}

	return transformatiom_temp;
}

void MeshNode::setupEulerAngle()
{
	for (u32 i = 0; i < nodeAnimTransformation.size(); i++)
	{
		eulerAngleXYZ[i].resize(nodeAnimTransformation[i].size());
		for (u32 j = 0; j < nodeAnimTransformation[i].size(); j++)
		{
			glm::mat4 rotationMatrix = nodeAnimRotationMatrices[i][j];
			//float rotation[3] = { atan2(rotationMatrix[2][1], rotationMatrix[2][2]),
			//					atan2(-rotationMatrix[2][0], std::sqrt(rotationMatrix[2][1] * rotationMatrix[2][1] + rotationMatrix[2][2] * rotationMatrix[2][2])),
			//					atan2(rotationMatrix[1][0], rotationMatrix[0][0]) };
			glm::vec3 rotation(0);
			glm::extractEulerAngleXYZ(rotationMatrix, rotation.x, rotation.y, rotation.z);

			eulerAngleXYZ[i][j].x = rotation.x;
			eulerAngleXYZ[i][j].y = rotation.y;
			eulerAngleXYZ[i][j].z = rotation.z;
		}
	}
}

void MeshNode::convertEulerAngleToMatrix()
{
	for (u32 i = 0; i < nodeAnimTransformation.size(); i++)
	{
		for (u32 j = 0; j < nodeAnimTransformation[i].size(); j++)
		{
			nodeAnimRotationMatrices[i][j] = glm::eulerAngleXYZ(eulerAngleXYZ[i][j].x, 
				eulerAngleXYZ[i][j].y, eulerAngleXYZ[i][j].z);
		}
	}
}

void MeshNode::updateTransformation()
{
	convertEulerAngleToMatrix();

	for (u32 i = 0; i < nodeAnimTransformation.size(); i++)
	{
		for (u32 j = 0; j < nodeAnimTransformation[i].size(); j++)
		{
			nodeAnimTransformation[i][j] = nodeAnimTranslationMatrices[i][j]
				* nodeAnimRotationMatrices[i][j] * nodeAnimScalingMatrices[i][j];
		}
	}
}

void MeshNode::updateMaximumFrame(u32 animationIndex, u32 i)
{
	if (hasAnimations())
	{
		maximumFrame[animationIndex] = i;
	}
	else
	{
		maximumFrame.resize(animationIndex + 1, 0);
		nodeAnimTransformation.resize(i);
		nodeAnimTranslationMatrices.resize(i);
		nodeAnimRotationMatrices.resize(i);
		nodeAnimScalingMatrices.resize(i);
		eulerAngleXYZ.resize(i);

		maximumFrame[animationIndex] = i;
	}
}

void MeshNode::updateAnimationSize(u32 animationIndex, u32 num)
{
	if (hasAnimations())
	{
		u32 currentSize = (u32)nodeAnimTransformation[animationIndex].size();
		if (num > 0)
		{
			if (num < currentSize)
			{
				updateMaximumFrame(animationIndex, num);
				setupEulerAngle();
				return;
			}
			else
			{
				updateMaximumFrame(animationIndex, num);

				glm::mat4 lastFrameTransformation = nodeAnimTransformation[animationIndex][currentSize - 1];
				glm::mat4 lastFrameTranslation = nodeAnimTranslationMatrices[animationIndex][currentSize - 1];
				glm::mat4 lastFrameRotation = nodeAnimRotationMatrices[animationIndex][currentSize - 1];
				glm::mat4 lastFrameScaling = nodeAnimScalingMatrices[animationIndex][currentSize - 1];

				nodeAnimTransformation[animationIndex].resize(num);
				nodeAnimTranslationMatrices[animationIndex].resize(num);
				nodeAnimRotationMatrices[animationIndex].resize(num);
				nodeAnimScalingMatrices[animationIndex].resize(num);
				eulerAngleXYZ[animationIndex].resize(num);

				for (u32 i = currentSize; i < num; i++)
				{
					nodeAnimTransformation[animationIndex][i] = lastFrameTransformation;
					nodeAnimTranslationMatrices[animationIndex][i] = lastFrameTranslation;
					nodeAnimRotationMatrices[animationIndex][i] = lastFrameRotation;
					nodeAnimScalingMatrices[animationIndex][i] = lastFrameScaling;
				}

				setupEulerAngle();
			}
		}
	}
	else
	{
		updateMaximumFrame(animationIndex, num);

		nodeAnimTransformation[animationIndex].resize(num);
		nodeAnimTranslationMatrices[animationIndex].resize(num);
		nodeAnimRotationMatrices[animationIndex].resize(num);
		nodeAnimScalingMatrices[animationIndex].resize(num);
		eulerAngleXYZ[animationIndex].resize(num);

		for (u32 i = 0; i < num; i++)
		{
			nodeAnimTransformation[animationIndex][i] = getTransformation();
			nodeAnimTranslationMatrices[animationIndex][i] = getTransformation();
			nodeAnimRotationMatrices[animationIndex][i] = getTransformation();
			nodeAnimScalingMatrices[animationIndex][i] = getTransformation();
		}

		setupEulerAngle();
	}
}

void MeshNode::insertFrameAfter(u32 animationIndex, u32 selectedFrame)
{
	if (hasAnimations())
	{
		u32 animationSize = (u32)nodeAnimTransformation[animationIndex].size();
		updateMaximumFrame(animationIndex, animationSize + 1);

		glm::mat4 lastFrameTransformation = nodeAnimTransformation[animationIndex][selectedFrame];
		glm::mat4 lastFrameTranslation = nodeAnimTranslationMatrices[animationIndex][selectedFrame];
		glm::mat4 lastFrameRotation = nodeAnimRotationMatrices[animationIndex][selectedFrame];
		glm::mat4 lastFrameScaling = nodeAnimScalingMatrices[animationIndex][selectedFrame];

		nodeAnimTransformation[animationIndex].insert(
			(nodeAnimTransformation[animationIndex].begin() + (selectedFrame + 2))
			, lastFrameTransformation);
		nodeAnimTranslationMatrices[animationIndex].insert(
			(nodeAnimTranslationMatrices[animationIndex].begin() + (selectedFrame + 2))
			, lastFrameTranslation);
		nodeAnimRotationMatrices[animationIndex].insert(
			(nodeAnimRotationMatrices[animationIndex].begin() + (selectedFrame + 2))
			, lastFrameRotation);
		nodeAnimScalingMatrices[animationIndex].insert(
			(nodeAnimScalingMatrices[animationIndex].begin() + (selectedFrame + 2))
			, lastFrameScaling);

		setupEulerAngle();
	}
}

u32 MeshNode::getNumOfAnimations()
{
	return (u32)nodeAnimTransformation.size();
}

bool MeshNode::hasAnimations()
{
	return nodeAnimTransformation.size() > 0;
}

bool MeshNode::hasMesh()
{
	return meshIndex.size() > 0;
}

//MeshNode2* MeshNode2::parse(std::ostream& stream)
//{
//
//}

void MeshNode::serialize(std::ostream& stream)
{
	std::string nameStr(name);
	size_t nameSize = nameStr.size();
	stream.write((char*)&nameSize, sizeof(nameSize));
	stream.write((char*)nameStr.c_str(), nameSize);
	
	// Whether the node has parent
	bool hasParent = (parent != nullptr);
	stream.write((char*)&hasParent, sizeof(bool));

	if (hasParent)
		stream.write((char*)&parent, sizeof(parent));

	stream.write((char*)&transformation, sizeof(transformation));

	size_t meshIndexSize = meshIndex.size();
	stream.write((char*)&meshIndexSize, sizeof(meshIndexSize));
	if(meshIndexSize > 0)
		stream.write((char*)&meshIndex[0], meshIndexSize * sizeof(u32));

	size_t childSize = child.size();
	stream.write((char*)&childSize, sizeof(childSize));
	for (u32 i = 0; i < childSize; i++)
	{
		child[i].serialize(stream);
	}

	size_t nodeAnimSize = nodeAnimations.size();
	stream.write((char*)&nodeAnimSize, sizeof(nodeAnimSize));
	if(nodeAnimSize > 0)
		stream.write((char*)&nodeAnimations[0], nodeAnimSize * sizeof(aiNodeAnim*));

	size_t animationSize = nodeAnimTransformation.size();
	stream.write((char*)&animationSize, sizeof(animationSize));
	for (size_t i = 0; i < animationSize; i++)
	{
		size_t keyframeSize = nodeAnimTransformation[i].size();
		stream.write((char*)&keyframeSize, sizeof(keyframeSize));
		if (keyframeSize > 0)
			stream.write((char*)&nodeAnimTransformation[i][0], keyframeSize * sizeof(mat4));
	}

	for (size_t i = 0; i < animationSize; i++)
	{
		size_t keyframeSize = nodeAnimTranslationMatrices[i].size();
		stream.write((char*)&keyframeSize, sizeof(keyframeSize));
		if (keyframeSize > 0)
			stream.write((char*)&nodeAnimTranslationMatrices[i][0], keyframeSize * sizeof(mat4));
	}

	for (size_t i = 0; i < animationSize; i++)
	{
		size_t keyframeSize = eulerAngleXYZ[i].size();
		stream.write((char*)&keyframeSize, sizeof(keyframeSize));
		if (keyframeSize > 0)
			stream.write((char*)&eulerAngleXYZ[i][0], keyframeSize * sizeof(glm::vec3));
	}

	for (size_t i = 0; i < animationSize; i++)
	{
		size_t keyframeSize = nodeAnimRotationMatrices[i].size();
		stream.write((char*)&keyframeSize, sizeof(keyframeSize));
		if (keyframeSize > 0)
			stream.write((char*)&nodeAnimRotationMatrices[i][0], keyframeSize * sizeof(mat4));
	}

	for (size_t i = 0; i < animationSize; i++)
	{
		size_t keyframeSize = nodeAnimScalingMatrices[i].size();
		stream.write((char*)&keyframeSize, sizeof(keyframeSize));
		if (keyframeSize > 0)
			stream.write((char*)&nodeAnimScalingMatrices[i][0], keyframeSize * sizeof(mat4));
	}

	if(animationSize > 0)
		stream.write((char*)&maximumFrame[0], animationSize * sizeof(u32));
}

void MeshNode::deserialize(std::istream& stream)
{
	std::string nameStr;
	size_t nameSize;
	stream.read((char*)&nameSize, sizeof(nameSize));
	nameStr.resize(nameSize);
	stream.read((char*)&nameStr[0], nameSize);
	name = nameStr;

	bool hasParent;
	stream.read((char*)&hasParent, sizeof(bool));

	if(hasParent)
		stream.read((char*)&parent, sizeof(parent));

	stream.read((char*)&transformation, sizeof(transformation));

	size_t meshIndexSize;
	stream.read((char*)&meshIndexSize, sizeof(meshIndexSize));
	meshIndex.resize(meshIndexSize);
	if(meshIndexSize > 0)
		stream.read((char*)&meshIndex[0], meshIndexSize * sizeof(u32));

	size_t childSize;
	stream.read((char*)&childSize, sizeof(childSize));
	child.resize(childSize);
	for (u32 i = 0; i < childSize; i++)
	{
		child[i].deserialize(stream);
	}

	size_t nodeAnimSize;
	stream.read((char*)&nodeAnimSize, sizeof(nodeAnimSize));
	nodeAnimations.resize(nodeAnimSize);
	if(nodeAnimSize > 0)
		stream.read((char*)&nodeAnimations[0], nodeAnimSize * sizeof(aiNodeAnim*));

	size_t animationSize;
	stream.read((char*)&animationSize, sizeof(animationSize));
	nodeAnimTransformation.resize(animationSize);
	for (size_t i = 0; i < animationSize; i++)
	{
		size_t keyframeSize;
		stream.read((char*)&keyframeSize, sizeof(keyframeSize));
		nodeAnimTransformation[i].resize(keyframeSize, mat4(1));
		if(keyframeSize > 0)
			stream.read((char*)&nodeAnimTransformation[i][0], keyframeSize * sizeof(mat4));
	}

	nodeAnimTranslationMatrices.resize(animationSize);
	for (size_t i = 0; i < animationSize; i++)
	{
		size_t keyframeSize;
		stream.read((char*)&keyframeSize, sizeof(keyframeSize));
		nodeAnimTranslationMatrices[i].resize(keyframeSize, mat4(1));
		if (keyframeSize > 0)
			stream.read((char*)&nodeAnimTranslationMatrices[i][0], keyframeSize * sizeof(mat4));
	}

	eulerAngleXYZ.resize(animationSize);
	for (size_t i = 0; i < animationSize; i++)
	{
		size_t keyframeSize;
		stream.read((char*)&keyframeSize, sizeof(keyframeSize));
		eulerAngleXYZ[i].resize(keyframeSize, vec3(0));
		if (keyframeSize > 0)
			stream.read((char*)&eulerAngleXYZ[i][0], keyframeSize * sizeof(glm::vec3));
	}

	nodeAnimRotationMatrices.resize(animationSize);
	for (size_t i = 0; i < animationSize; i++)
	{
		size_t keyframeSize;
		stream.read((char*)&keyframeSize, sizeof(keyframeSize));
		nodeAnimRotationMatrices[i].resize(keyframeSize, mat4(1));
		if (keyframeSize > 0)
			stream.read((char*)&nodeAnimRotationMatrices[i][0], keyframeSize * sizeof(mat4));
	}

	nodeAnimScalingMatrices.resize(animationSize);
	for (size_t i = 0; i < animationSize; i++)
	{
		size_t keyframeSize;
		stream.read((char*)&keyframeSize, sizeof(keyframeSize));
		nodeAnimScalingMatrices[i].resize(keyframeSize, mat4(1));
		if (keyframeSize > 0)
			stream.read((char*)&nodeAnimScalingMatrices[i][0], keyframeSize * sizeof(mat4));
	}

	maximumFrame.resize(animationSize);
	if (nodeAnimSize > 0)
		stream.read((char*)&maximumFrame[0], animationSize * sizeof(u32));
}
