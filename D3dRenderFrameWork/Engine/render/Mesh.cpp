
#ifdef WIN32
#include <Engine/render/Mesh.h>

Mesh::Mesh()
{
	ZeroMemory(this, sizeof(Mesh));
}


/**
 * 此构造函数会使原vector对象失效
 */
Mesh::Mesh(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices, std::vector<SubMesh>&& subMeshes) :
	mVertices(std::move(vertices)), mIndices(std::move(indices)), mSubMeshes(std::move(subMeshes)) { }

/**
 * 此构造函数会拷贝一份新的网格数据
 */
Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
	const std::vector<SubMesh>& subMeshes) : mVertices(vertices), mIndices(indices), mSubMeshes(subMeshes) { }


/**
 * 移动赋值，会使原顶点vector失效
 */
void Mesh::SetVertices(std::vector<Vertex>&& vertices)
{
	this->mVertices = std::move(vertices);
}

/**
 * 移动赋值，会使原索引vector失效
 */
void Mesh::SetIndices(std::vector<uint32_t>&& indices)
{
	this->mIndices = std::move(indices);
}

void Mesh::SetVertices(const std::vector<Vertex>& vertices)
{
	this->mVertices = vertices;
}

void Mesh::SetIndices(const std::vector<uint32_t>& indices)
{
	this->mIndices = indices;
}

Mesh MeshPrototype::CreateCubeMesh()
{
	std::vector<Vertex> cubeVertices{
		// -x
		{ { -0.5f, -0.5f,  0.5f }, {-1,  0,  0}, { 0,  0, -1}, {0, 0} },
		{ { -0.5f, -0.5f, -0.5f }, {-1,  0,  0}, { 0,  0, -1}, {1, 0} },
		{ { -0.5f,  0.5f, -0.5f }, {-1,  0,  0}, { 0,  0, -1}, {1, 1} },
		{ { -0.5f,  0.5f,  0.5f }, {-1,  0,  0}, { 0,  0, -1}, {0, 1} },

		// +x
		{ {  0.5f, -0.5f, -0.5f }, { 1,  0,  0}, { 0,  0,  1}, {0, 0} },
		{ {  0.5f, -0.5f,  0.5f }, { 1,  0,  0}, { 0,  0,  1}, {1, 0} },
		{ {  0.5f,  0.5f,  0.5f }, { 1,  0,  0}, { 0,  0,  1}, {1, 1} },
		{ {  0.5f,  0.5f, -0.5f }, { 1,  0,  0}, { 0,  0,  1}, {0, 1} },

		// -z
		{ { -0.5f, -0.5f, -0.5f }, { 0,  0, -1}, { 1,  0,  0}, {0, 0} },
		{ {  0.5f, -0.5f, -0.5f }, { 0,  0, -1}, { 1,  0,  0}, {1, 0} },
		{ {  0.5f,  0.5f, -0.5f }, { 0,  0, -1}, { 1,  0,  0}, {1, 1} },
		{ { -0.5f,  0.5f, -0.5f }, { 0,  0, -1}, { 1,  0,  0}, {0, 1} },

		// +z
		{ {  0.5f, -0.5f,  0.5f }, { 0,  0,  1}, {-1,  0,  0}, {0, 0} },
		{ { -0.5f, -0.5f,  0.5f }, { 0,  0,  1}, {-1,  0,  0}, {1, 0} },
		{ { -0.5f,  0.5f,  0.5f }, { 0,  0,  1}, {-1,  0,  0}, {1, 1} },
		{ {  0.5f,  0.5f,  0.5f }, { 0,  0,  1}, {-1,  0,  0}, {0, 1} },

		// -y
		{ { -0.5f, -0.5f,  0.5f }, { 0, -1,  0}, { 1,  0,  0}, {0, 0} },
		{ {  0.5f, -0.5f,  0.5f }, { 0, -1,  0}, { 1,  0,  0}, {1, 0} },
		{ {  0.5f, -0.5f, -0.5f }, { 0, -1,  0}, { 1,  0,  0}, {1, 1} },
		{ { -0.5f, -0.5f, -0.5f }, { 0, -1,  0}, { 1,  0,  0}, {0, 1} },

		// +y
		{ { -0.5f,  0.5f, -0.5f }, { 0, -1,  0}, { 1,  0,  0}, {0, 0} },
		{ {  0.5f,  0.5f, -0.5f }, { 0, -1,  0}, { 1,  0,  0}, {1, 0} },
		{ {  0.5f,  0.5f,  0.5f }, { 0, -1,  0}, { 1,  0,  0}, {1, 1} },
		{ { -0.5f,  0.5f,  0.5f }, { 0, -1,  0}, { 1,  0,  0}, {0, 1} },
	};
	std::vector<uint32_t> cubeIndices = {
		// -x
		0, 1, 2,
		0, 2, 3,
		// +x
		4, 5, 6,
		4, 6, 7,
		// -z
		8, 9, 10,
		8, 10, 11,
		// +z
		12, 13, 14,
		12, 14, 15,
		//-y
		16, 17, 18,
		16, 18, 19,
		//+y
		20, 21, 22,
		20, 22, 23
	};
	std::vector<SubMesh> subMeshes = { {36, 0, 0} };
	return { std::move(cubeVertices), std::move(cubeIndices), std::move(subMeshes) };
}

std::vector<Vertex>& Mesh::Vertices()
{
	return mVertices;
}

std::vector<uint32_t>& Mesh::Indices()
{
	return mIndices;
}

std::vector<SubMesh>& Mesh::SubMeshes()
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
#endif