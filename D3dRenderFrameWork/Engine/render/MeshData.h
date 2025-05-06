#pragma once
#include "SubMesh.h"
#include "Engine/pch.h"
#include "Engine/common/Exception.h"

struct MeshData
{
	MeshData() = default;

	MeshData(VertexBufferRef vertexBuffer, VertexBufferRef indexBuffer, uint32_t vertexCount,
		uint32_t indexCount, const SubMesh* subMeshes, uint8_t subMeshCount)
		: mVertexBuffer(vertexBuffer),
		  mIndexBuffer(indexBuffer),
		  mVertexCount(vertexCount),
		  mIndexCount(indexCount),
		  mSubMeshCount(subMeshCount)
	{
		if (subMeshes && subMeshCount)
		{
			mSubMeshes.reset(new SubMesh[subMeshCount]);
			memcpy(mSubMeshes.get(), subMeshes, subMeshCount);
		}
	}

	MeshData(const MeshData& other)
	{
		mVertexBuffer = other.mVertexBuffer;
		mIndexBuffer = other.mIndexBuffer;
		mVertexCount = other.mVertexCount;
		mIndexCount = other.mIndexCount;
		mSubMeshes = std::make_unique<SubMesh[]>(other.mSubMeshCount);
		mSubMeshCount = other.mSubMeshCount;
		memcpy(mSubMeshes.get(), other.mSubMeshes.get(), mSubMeshCount);
	}

	MeshData& operator=(const MeshData& other)
	{
		if (this != &other)
		{
			mVertexBuffer = other.mVertexBuffer;
			mIndexBuffer = other.mIndexBuffer;
			mVertexCount = other.mVertexCount;
			mIndexCount = other.mIndexCount;
			mSubMeshes = std::make_unique<SubMesh[]>(other.mSubMeshCount);
			mSubMeshCount = other.mSubMeshCount;
			memcpy(mSubMeshes.get(), other.mSubMeshes.get(), mSubMeshCount);
		}
		return *this;
	}
	~MeshData() = default;

	VertexBufferRef mVertexBuffer;
	VertexBufferRef mIndexBuffer;
	uint32_t mVertexCount;
	uint32_t mIndexCount;
	std::unique_ptr<SubMesh[]> mSubMeshes;
	uint8_t mSubMeshCount;
};

struct Mesh
{
public:
    const std::vector<DirectX::XMFLOAT3>& vertex();
    const std::vector<DirectX::XMFLOAT3>& color();
    const std::vector<DirectX::XMFLOAT3>& normal();
    const std::vector<DirectX::XMFLOAT3>& tangent();
    const std::vector<DirectX::XMFLOAT3>& bitangent();
    std::vector<float> tex(uint8_t semanticIdx, uint8_t* pNumComponent) const;
    const std::vector<uint32_t>& indices();
    const std::vector<SubMesh>& subMeshes();
    uint64_t numVertex() const;
    uint64_t numIndex() const;
    uint32_t calcVertexSize() const;
	uint64_t vertexBufferSize() const;
	void setVertex();
	void emplaceVertex(std::vector<DirectX::XMFLOAT3>&& vertex);
	void emplaceNormal(std::vector<DirectX::XMFLOAT3>&& normal);
	void emplaceColor(std::vector<DirectX::XMFLOAT3>&& color);
	void emplaceTangent(std::vector<DirectX::XMFLOAT3>&& tangent);
	void emplaceBitangent(std::vector<DirectX::XMFLOAT3>&& bitangent);
	void emplaceIndex(std::vector<uint32_t>&& index);
	void emplaceTex(uint8_t semanticIdx, uint8_t numComponent, std::vector<float>&& tex);
	void setSubMeshes(const std::vector<SubMesh>& subMeshes);
    std::vector<float> getFilteredVertexBuffer() const;

    Mesh();
    Mesh(DirectX::XMFLOAT3* vertexData, uint32_t numVertices, uint32_t* indexData, uint64_t numIndices);
    ~Mesh() = default;

    DEFAULT_COPY_CONSTRUCTOR(Mesh)
    DEFAULT_COPY_OPERATOR(Mesh)
    DEFAULT_MOVE_CONSTRUCTOR(Mesh)
    DEFAULT_MOVE_OPERATOR(Mesh)

private:
    std::vector<DirectX::XMFLOAT3> mVertex;
    std::vector<DirectX::XMFLOAT3> mColor;
    std::vector<DirectX::XMFLOAT3> mNormal;
    std::vector<DirectX::XMFLOAT3> mTangent;
    std::vector<DirectX::XMFLOAT3> mBiTangent;
	std::vector<float> mTex[5];
	uint8_t mTexComponents[5];
	std::vector<float> mVertexBuffer;
    std::vector<uint32_t> mIndices;
    std::vector<SubMesh> mSubMeshes;
};

namespace MeshPrototype
{
    Mesh CreateCubeMesh();
}

inline Mesh::Mesh() = default;

inline Mesh::Mesh(DirectX::XMFLOAT3* vertexData, uint32_t numVertices, uint32_t* indexData, uint64_t numIndices) :
	mVertex(vertexData, vertexData + numVertices),
	mIndices(indexData, indexData + numIndices), mTex{}, mTexComponents{}{ }


inline const std::vector<DirectX::XMFLOAT3>& Mesh::vertex()
{
	return mVertex;
}

inline const std::vector<DirectX::XMFLOAT3>& Mesh::color()
{
	return mColor;
}

inline const std::vector<DirectX::XMFLOAT3>& Mesh::normal()
{
	return mNormal;
}

inline const std::vector<DirectX::XMFLOAT3>& Mesh::tangent()
{
	return mTangent;
}

inline const std::vector<DirectX::XMFLOAT3>& Mesh::bitangent()
{
	return mTangent;
}

inline std::vector<float> Mesh::tex(uint8_t semanticIdx, uint8_t* pNumComponent) const
{
#if defined(DEBUG) or defined(_DEBUG)
	ASSERT(semanticIdx < 5, TEXT("semantic index out of bound(0~4)\n"))
#endif
	*pNumComponent = mTexComponents[semanticIdx];
	return mTex[semanticIdx];
}

inline uint32_t Mesh::calcVertexSize() const
{
	uint32_t size = 0;
	if (!mVertex.empty()) size += sizeof(DirectX::XMFLOAT3);
	if (!mNormal.empty()) size += sizeof(DirectX::XMFLOAT3);
	if (!mTangent.empty()) size += sizeof(DirectX::XMFLOAT3);
	if (!mBiTangent.empty()) size += sizeof(DirectX::XMFLOAT3);
	if (!mColor.empty()) size += sizeof(DirectX::XMFLOAT3);
	for (int i = 0; i < 5; ++i)
	{
		if (!mTex[i].empty()) size += mTexComponents[i] * sizeof(float);
	}
	return size;
}

inline uint64_t Mesh::vertexBufferSize() const
{
	return mVertexBuffer.size() * sizeof(float);
}

inline void Mesh::emplaceVertex(std::vector<DirectX::XMFLOAT3>&& vertex)
{
	mVertex = std::move(vertex);
}

inline void Mesh::emplaceNormal(std::vector<DirectX::XMFLOAT3>&& normal)
{
	mNormal = std::move(normal);
}

inline void Mesh::emplaceColor(std::vector<DirectX::XMFLOAT3>&& color)
{
	mColor = std::move(color);
}

inline void Mesh::emplaceTangent(std::vector<DirectX::XMFLOAT3>&& tangent)
{
	mTangent = std::move(tangent);
}

inline void Mesh::emplaceBitangent(std::vector<DirectX::XMFLOAT3>&& bitangent)
{
	mBiTangent = std::move(bitangent);
}

inline void Mesh::emplaceIndex(std::vector<uint32_t>&& index)
{
	mIndices = std::move(index);
}

inline void Mesh::emplaceTex(uint8_t semanticIdx, uint8_t numComponent, std::vector<float>&& tex)
{
#if defined(DEBUG) or defined(_DEBUG)
	ASSERT(semanticIdx < 5, TEXT("semantic index out of bound(0~4)\n"));
#endif
	mTex[semanticIdx] = std::move(tex);
	mTex->resize(mTex->size() / numComponent * numComponent);
}

inline void Mesh::setSubMeshes(const std::vector<SubMesh>& subMeshes)
{
	mSubMeshes = subMeshes;
}

// TODO: filter the vertex data according to mShader 
inline std::vector<float> Mesh::getFilteredVertexBuffer() const
{
	std::vector<float> vertexBuffer{};
	// vertexBuffer.resize(mVertex.Size() * calcVertexSize());
	// we use fixed vertex data temporally, so the vertex Size is fixed.
	auto a = mVertexBuffer.size();
	vertexBuffer.resize(mVertex.size() * 8);
	
	// we should filter the vertex props by mShader input.
	// now we just let it be the combination of POSITION & NORMAL & TEXCOORD0
	// ----------------------Temp Filter----------------------------
	ASSERT(!mVertex.empty(), TEXT("vertex data missed"))
	float* vertexPos = vertexBuffer.data();
	float* zeroBuffer = new float[4]{0, 0, 0, 0};
	for (size_t i = 0; i < mVertex.size(); ++i)
	{
		memcpy(vertexPos, mVertex.data() + i, sizeof(DirectX::XMFLOAT3));
		vertexPos += 3;
		if (i >= mNormal.size())
		{
			WARN("missing normal data");
			memcpy(vertexPos, zeroBuffer, sizeof(DirectX::XMFLOAT3));
		}
		else
		{
			memcpy(vertexPos, mNormal.data() + i, sizeof(DirectX::XMFLOAT3));
		}
		vertexPos += 3;
		if (i >= mTex->size())
		{
			WARN("missing tex0 data");
			memcpy(vertexPos, zeroBuffer, sizeof(DirectX::XMFLOAT2));
		}
		else
		{
			memcpy(vertexPos, mTex->data() + i, sizeof(float[2]));
		}
		vertexPos += 2;
	}

	delete[] zeroBuffer;
	return vertexBuffer;
}

inline const std::vector<uint32_t>& Mesh::indices()
{
	return mIndices;
}

inline const std::vector<SubMesh>& Mesh::subMeshes()
{
	return mSubMeshes;
}

inline uint64_t Mesh::numVertex() const
{
	return mVertex.size();
}

inline uint64_t Mesh::numIndex() const
{
	return mIndices.size();
}

inline Mesh MeshPrototype::CreateCubeMesh()
{
	Mesh cubeMeshTemp{ };
	cubeMeshTemp.setSubMeshes({ {36, 0, 0} });
#pragma region CubeMeshInitialize
	cubeMeshTemp.emplaceVertex(std::vector<DirectX::XMFLOAT3>{
		// -x
		{ -0.5f, -0.5f,  0.5f }, // {-1,  0,  0}, { 0,  0, -1}, {0, 0} },
		{ -0.5f, -0.5f, -0.5f }, // {-1,  0,  0}, { 0,  0, -1}, {1, 0} },
		{ -0.5f,  0.5f, -0.5f }, // {-1,  0,  0}, { 0,  0, -1}, {1, 1} },
		{ -0.5f,  0.5f,  0.5f }, // {-1,  0,  0}, { 0,  0, -1}, {0, 1} },

		// +x
		{  0.5f, -0.5f, -0.5f }, // { 1,  0,  0}, { 0,  0,  1}, {0, 0} },
		{  0.5f, -0.5f,  0.5f }, // { 1,  0,  0}, { 0,  0,  1}, {1, 0} },
		{  0.5f,  0.5f,  0.5f }, // { 1,  0,  0}, { 0,  0,  1}, {1, 1} },
		{  0.5f,  0.5f, -0.5f }, // { 1,  0,  0}, { 0,  0,  1}, {0, 1} },

		// -z
		{ -0.5f, -0.5f, -0.5f }, // { 0,  0, -1}, { 1,  0,  0}, {0, 0} },
		{  0.5f, -0.5f, -0.5f }, // { 0,  0, -1}, { 1,  0,  0}, {1, 0} },
		{  0.5f,  0.5f, -0.5f }, // { 0,  0, -1}, { 1,  0,  0}, {1, 1} },
		{ -0.5f,  0.5f, -0.5f }, // { 0,  0, -1}, { 1,  0,  0}, {0, 1} },

		// +z
		{  0.5f, -0.5f,  0.5f }, // { 0,  0,  1}, {-1,  0,  0}, {0, 0} },
		{ -0.5f, -0.5f,  0.5f }, // { 0,  0,  1}, {-1,  0,  0}, {1, 0} },
		{ -0.5f,  0.5f,  0.5f }, // { 0,  0,  1}, {-1,  0,  0}, {1, 1} },
		{  0.5f,  0.5f,  0.5f }, // { 0,  0,  1}, {-1,  0,  0}, {0, 1} },

		// -y
		{ -0.5f, -0.5f,  0.5f }, // { 0, -1,  0}, { -1,  0,  0}, {0, 0} },
		{  0.5f, -0.5f,  0.5f }, // { 0, -1,  0}, { -1,  0,  0}, {1, 0} },
		{  0.5f, -0.5f, -0.5f }, // { 0, -1,  0}, { -1,  0,  0}, {1, 1} },
		{ -0.5f, -0.5f, -0.5f }, // { 0, -1,  0}, { -1,  0,  0}, {0, 1} },

		// +y
		{ -0.5f,  0.5f, -0.5f }, // { 0, -1,  0}, { 1,  0,  0}, {0, 0} },
		{  0.5f,  0.5f, -0.5f }, // { 0, -1,  0}, { 1,  0,  0}, {1, 0} },
		{  0.5f,  0.5f,  0.5f }, // { 0, -1,  0}, { 1,  0,  0}, {1, 1} },
		{ -0.5f,  0.5f,  0.5f }  // { 0, -1,  0}, { 1,  0,  0}, {0, 1} },
	} );
	cubeMeshTemp.emplaceNormal({
		// -x
		{-1,  0,  0},
		{-1,  0,  0},
		{-1,  0,  0},
		{-1,  0,  0},

		// +x
		{ 1,  0,  0},
		{ 1,  0,  0},
		{ 1,  0,  0},
		{ 1,  0,  0},

		// -z
		{ 0,  0, -1},
		{ 0,  0, -1},
		{ 0,  0, -1},
		{ 0,  0, -1},

		// +z
		{ 0,  0,  1},
		{ 0,  0,  1},
		{ 0,  0,  1},
		{ 0,  0,  1},

		// -y
		{ 0, -1,  0},
		{ 0, -1,  0},
		{ 0, -1,  0},
		{ 0, -1,  0},

		// +y
		{ 0, -1,  0},
		{ 0, -1,  0},
		{ 0, -1,  0},
		{ 0, -1,  0},
	});

	cubeMeshTemp.emplaceTangent({
		// -x
		{ 0,  1,  0},
		{ 0,  1,  0},
		{ 0,  1,  0},
		{ 0,  1,  0},

		// +x
		{ 0,  1,  0},
		{ 0,  1,  0},
		{ 0,  1,  0},
		{ 0,  1,  0},

		// -z
		{ 0,  1,  0},
		{ 0,  1,  0},
		{ 0,  1,  0},
		{ 0,  1,  0},

		// +z
		{ 0,  1,  0},
		{ 0,  1,  0},
		{ 0,  1,  0},
		{ 0,  1,  0},
		
		// -y
		{-1,  0,  0},
		{-1,  0,  0},
		{-1,  0,  0},
		{-1,  0,  0},

		// +y
		{ 1,  0,  0},
		{ 1,  0,  0},
		{ 1,  0,  0},
		{ 1,  0,  0},
	});

	cubeMeshTemp.emplaceBitangent( {
		// +x
		{0,  0,  -1},
		{0,  0,  -1},
		{0,  0,  -1},
		{0,  0,  -1},

		// -x
		{ 0,  0,  1},
		{ 0,  0,  1},
		{ 0,  0,  1},
		{ 0,  0,  1},

		// -y
		{ 1,  0,  0},
		{ 1,  0,  0},
		{ 1,  0,  0},
		{ 1,  0,  0},

		// +y
		{ 1,  0,  0},
		{ 1,  0,  0},
		{ 1,  0,  0},
		{ 1,  0,  0},
			
		// -z
		{ 1,  0,  0},
		{ 1,  0,  0},
		{ 1,  0,  0},
		{ 1,  0,  0},

		// +z
		{-1,  0,  0},
		{-1,  0,  0},
		{-1,  0,  0},
		{-1,  0,  0},
	} ),
	cubeMeshTemp.emplaceColor({
		// -x
		{0.7f,  0.7f,  0.7f},
		{0.7f,  0.7f,  0.7f},
		{0.7f,  0.7f,  0.7f},
		{0.7f,  0.7f,  0.7f},

		// +x
		{ 0.7f,  0.7f,  0.7f},
		{ 0.7f,  0.7f,  0.7f},
		{ 0.7f,  0.7f,  0.7f},
		{ 0.7f,  0.7f,  0.7f},

		// -z
		{ 0.7f,  0.7f, 0.7f},
		{ 0.7f,  0.7f, 0.7f},
		{ 0.7f,  0.7f, 0.7f},
		{ 0.7f,  0.7f, 0.7f},

		// +z
		{ 0.7f,  0.7f,  0.7f},
		{ 0.7f,  0.7f,  0.7f},
		{ 0.7f,  0.7f,  0.7f},
		{ 0.7f,  0.7f,  0.7f},

		// -y
		{ 0.7f, 0.7f,  0.7f},
		{ 0.7f, 0.7f,  0.7f},
		{ 0.7f, 0.7f,  0.7f},
		{ 0.7f, 0.7f,  0.7f},

		// +y
		{ 0.7f, 0.7f,  0.7f},
		{ 0.7f, 0.7f,  0.7f},
		{ 0.7f, 0.7f,  0.7f},
		{ 0.7f, 0.7f,  0.7f},
	}),

	cubeMeshTemp.emplaceTex(0, 2, {
		// -x
		0, 0,	1, 0,	1, 1,	0, 1,

		// +x
		0, 0,	1, 0,	1, 1,	0, 1,

		// -z
		0, 0,	1, 0,	1, 1,	0, 1,

		// +z
		0, 0,	1, 0,	1, 1,	0, 1,

		// -y
		0, 0,	1, 0,	1, 1,	0, 1,

		// +y
		0, 0,	1, 0,	1, 1,	0, 1
	});
	
	cubeMeshTemp.emplaceIndex({
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
	});
#pragma endregion cubeMeshInitialize
	return cubeMeshTemp;
}