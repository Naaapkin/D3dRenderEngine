#ifdef WIN32
#include <Engine/render/Mesh.h>

Mesh::Mesh() = default;

Mesh::~Mesh() = default;

void Mesh::SetIndices(std::vector<uint32_t>&& indices)
{
	this->mIndices = std::move(indices);
}

void Mesh::SetVertices(std::vector<Vector3>&& vertices)
{
	mVertices = std::move(vertices);
}

const std::vector<Vector3>& Mesh::Vertices()
{
	return mVertices;
}

const std::vector<uint32_t>& Mesh::Indices()
{
	return mIndices;
}

const std::vector<SubMesh>& Mesh::SubMeshes()
{
	return mSubMeshes;
}

uint32_t Mesh::VCount() const
{
	return mVertices.size();
}

uint32_t Mesh::ICount() const
{
	return mIndices.size();
}

void Mesh::CopyVertices(const std::vector<Vector3>& vertices)
{
	mVertices = vertices;
}

void Mesh::CopyIndices(const std::vector<uint32_t>& indices)
{
	mIndices = indices;
}

// Mesh MeshPrototype::CreateCubeMesh()
// {
// 	std::vector<Vertex> cubeVertices{
// 		// -x
// 		{ { -0.5f, -0.5f,  0.5f }, {-1,  0,  0}, { 0,  0, -1}, {0, 0} },
// 		{ { -0.5f, -0.5f, -0.5f }, {-1,  0,  0}, { 0,  0, -1}, {1, 0} },
// 		{ { -0.5f,  0.5f, -0.5f }, {-1,  0,  0}, { 0,  0, -1}, {1, 1} },
// 		{ { -0.5f,  0.5f,  0.5f }, {-1,  0,  0}, { 0,  0, -1}, {0, 1} },
//
// 		// +x
// 		{ {  0.5f, -0.5f, -0.5f }, { 1,  0,  0}, { 0,  0,  1}, {0, 0} },
// 		{ {  0.5f, -0.5f,  0.5f }, { 1,  0,  0}, { 0,  0,  1}, {1, 0} },
// 		{ {  0.5f,  0.5f,  0.5f }, { 1,  0,  0}, { 0,  0,  1}, {1, 1} },
// 		{ {  0.5f,  0.5f, -0.5f }, { 1,  0,  0}, { 0,  0,  1}, {0, 1} },
//
// 		// -z
// 		{ { -0.5f, -0.5f, -0.5f }, { 0,  0, -1}, { 1,  0,  0}, {0, 0} },
// 		{ {  0.5f, -0.5f, -0.5f }, { 0,  0, -1}, { 1,  0,  0}, {1, 0} },
// 		{ {  0.5f,  0.5f, -0.5f }, { 0,  0, -1}, { 1,  0,  0}, {1, 1} },
// 		{ { -0.5f,  0.5f, -0.5f }, { 0,  0, -1}, { 1,  0,  0}, {0, 1} },
//
// 		// +z
// 		{ {  0.5f, -0.5f,  0.5f }, { 0,  0,  1}, {-1,  0,  0}, {0, 0} },
// 		{ { -0.5f, -0.5f,  0.5f }, { 0,  0,  1}, {-1,  0,  0}, {1, 0} },
// 		{ { -0.5f,  0.5f,  0.5f }, { 0,  0,  1}, {-1,  0,  0}, {1, 1} },
// 		{ {  0.5f,  0.5f,  0.5f }, { 0,  0,  1}, {-1,  0,  0}, {0, 1} },
//
// 		// -y
// 		{ { -0.5f, -0.5f,  0.5f }, { 0, -1,  0}, { 1,  0,  0}, {0, 0} },
// 		{ {  0.5f, -0.5f,  0.5f }, { 0, -1,  0}, { 1,  0,  0}, {1, 0} },
// 		{ {  0.5f, -0.5f, -0.5f }, { 0, -1,  0}, { 1,  0,  0}, {1, 1} },
// 		{ { -0.5f, -0.5f, -0.5f }, { 0, -1,  0}, { 1,  0,  0}, {0, 1} },
//
// 		// +y
// 		{ { -0.5f,  0.5f, -0.5f }, { 0, -1,  0}, { 1,  0,  0}, {0, 0} },
// 		{ {  0.5f,  0.5f, -0.5f }, { 0, -1,  0}, { 1,  0,  0}, {1, 0} },
// 		{ {  0.5f,  0.5f,  0.5f }, { 0, -1,  0}, { 1,  0,  0}, {1, 1} },
// 		{ { -0.5f,  0.5f,  0.5f }, { 0, -1,  0}, { 1,  0,  0}, {0, 1} },
// 	};
// 	std::vector<uint32_t> cubeIndices = {
// 		// -x
// 		0, 1, 2,
// 		0, 2, 3,
// 		// +x
// 		4, 5, 6,
// 		4, 6, 7,
// 		// -z
// 		8, 9, 10,
// 		8, 10, 11,
// 		// +z
// 		12, 13, 14,
// 		12, 14, 15,
// 		//-y
// 		16, 17, 18,
// 		16, 18, 19,
// 		//+y
// 		20, 21, 22,
// 		20, 22, 23
// 	};
// 	std::vector<SubMesh> subMeshes = { {36, 0, 0} };
// 	return { std::move(cubeVertices), std::move(cubeIndices), std::move(subMeshes) };
// }
#endif
