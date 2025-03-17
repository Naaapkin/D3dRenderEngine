#pragma once
#include <pch.h>
#include <Engine/math/math.h>

struct SubMesh
{
	uint32_t m_indexNum;
	uint32_t m_startIndex;
	int32_t m_baseVertex;
	// boundingBox
};

/**
 * 模型数据 \n
 * 注意：Mesh不管理顶点和索引数据的生命周期
 */
class Mesh
{
	std::vector<Vector3> mVertices;
	std::vector<Vector3> mNormals;
	std::vector<uint32_t> mIndices;
	std::vector<SubMesh> mSubMeshes;

public:
	const std::vector<Vector3>& Vertices();
	const std::vector<uint32_t>& Indices();
	const std::vector<SubMesh>& SubMeshes();
	uint32_t VCount() const;
	uint32_t ICount() const;
	void SetVertices(std::vector<Vector3>&& vertices);
	void SetIndices(std::vector<uint32_t>&& indices);
	void CopyVertices(const std::vector<Vector3>& vertices);
	void CopyIndices(const std::vector<uint32_t>& indices);

	Mesh();
	
	DEFAULT_COPY_CONSTRUCTOR(Mesh)
	DEFAULT_COPY_OPERATOR(Mesh)
	DEFAULT_MOVE_CONSTRUCTOR(Mesh)
	DEFAULT_MOVE_OPERATOR(Mesh)
};

namespace MeshPrototype
{
	Mesh CreateCubeMesh();

	Mesh cube = std::move(CreateCubeMesh());
}