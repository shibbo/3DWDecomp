#include <nn/vfx/vfx_VertexBuffer.h>

#include <nn/gfx/gfx_Buffer.h>
#include <nn/gfx/gfx_MemoryPool.h>
#include <nn/gfx/gfx_Sampler.h>
#include <nn/gfx/gfx_Texture.h>
#include <nn/gfx/util/gfx_PrimitiveShape.h>
#include <nn/util/util_Constants.h>
#include <nvn/nvn_FuncPtrInline.h>

namespace nn {
namespace vfx {
namespace detail {

/**
 * Constructs an attribute without any memory attached.
 */
Attribute::Attribute() {
    for (int i = 0; i < BufferSide_Max; i++) {
        m_GpuAddress[i].ToData()->value = 0;
        m_GpuAddress[i].ToData()->impl = 0;
    }

    m_pCpuAddress[BufferSide_FrontBuffer] = nullptr;
    m_pCpuAddress[BufferSide_BackBuffer] = nullptr;
}

/**
 * Allocates one copy of the attribute per buffer side.
 * @param pAllocator allocator the memory is taken from
 * @param bufferCount number of buffer sides (2 or 3)
 * @param size size of one copy in bytes
 * @return true on success, false when the allocation failed
 */
bool Attribute::Initialize(BufferAllocator* pAllocator, int bufferCount, size_t size) {
    size_t alignment = pAllocator->GetAlignment();

    m_BufferCount = bufferCount;
    m_Size = (size + alignment - 1) & ~(alignment - 1);
    m_Cutter.m_pAllocator = pAllocator;
    m_Cutter.m_Size = m_Size * bufferCount;
    m_Cutter.m_pBuffer = pAllocator->Alloc(m_Cutter.m_Size, pAllocator->GetAlignment());

    if (m_Cutter.m_pBuffer == nullptr) {
        return false;
    }

    m_Cutter.m_Offset = 0;

    m_pCpuAddress[BufferSide_FrontBuffer] = m_Cutter.Cut(m_Size);
    pAllocator->GetGpuAddress(&m_GpuAddress[BufferSide_FrontBuffer],
                              m_pCpuAddress[BufferSide_FrontBuffer]);

    m_pCpuAddress[BufferSide_BackBuffer] = m_Cutter.Cut(m_Size);
    pAllocator->GetGpuAddress(&m_GpuAddress[BufferSide_BackBuffer],
                              m_pCpuAddress[BufferSide_BackBuffer]);

    if (m_BufferCount == 3) {
        m_pCpuAddress[BufferSide_ThirdBuffer] = m_Cutter.Cut(m_Size);
        pAllocator->GetGpuAddress(&m_GpuAddress[BufferSide_ThirdBuffer],
                                  m_pCpuAddress[BufferSide_ThirdBuffer]);
    }

    return true;
}

/**
 * Returns the memory of the attribute to its allocator.
 */
void Attribute::Finalize() {
    if (m_Cutter.m_pBuffer != nullptr) {
        m_Cutter.m_pAllocator->Free(m_Cutter.m_pBuffer, false);
        m_Cutter.m_pBuffer = nullptr;
    }
}

/**
 * @param side buffer side to access
 * @return the CPU address of the given buffer side
 */
void* Attribute::Map(BufferSide side) {
    return m_pCpuAddress[side];
}

/**
 * Ends CPU access to the attribute. Nothing to do: the memory is coherent.
 */
void Attribute::Unmap() {}

/**
 * @param side buffer side to access
 * @return the GPU address of the given buffer side
 */
const nn::gfx::GpuAddress* Attribute::GetGpuAddress(BufferSide side) const {
    return &m_GpuAddress[side];
}

}  // namespace detail
}  // namespace vfx
}  // namespace nn

namespace nn::gfx::util {

/**
 * Sets the debug label of an NVN memory pool.
 * @param pMemoryPool memory pool to label
 * @param label the new label
 */
template <>
void SetMemoryPoolDebugLabel<ApiVariationNvn8>(TMemoryPool<ApiVariationNvn8>* pMemoryPool,
                                               const char* label) {
    nvnMemoryPoolSetDebugLabel(pMemoryPool->ToData()->pNvnMemoryPool, label);
}

/**
 * Sets the debug label of an NVN buffer.
 * @param pBuffer buffer to label
 * @param label the new label
 */
template <>
void SetBufferDebugLabel<ApiVariationNvn8>(TBuffer<ApiVariationNvn8>* pBuffer, const char* label) {
    nvnBufferSetDebugLabel(pBuffer->ToData()->pNvnBuffer, label);
}

/**
 * Sets the debug label of an NVN texture.
 * @param pTexture texture to label
 * @param label the new label
 */
template <>
void SetTextureDebugLabel<ApiVariationNvn8>(TTexture<ApiVariationNvn8>* pTexture,
                                            const char* label) {
    nvnTextureSetDebugLabel(pTexture->ToData()->pNvnTexture, label);
}

/**
 * Sets the debug label of an NVN sampler.
 * @param pSampler sampler to label
 * @param label the new label
 */
template <>
void SetSamplerDebugLabel<ApiVariationNvn8>(TSampler<ApiVariationNvn8>* pSampler,
                                            const char* label) {
    nvnSamplerSetDebugLabel(pSampler->ToData()->pNvnSampler, label);
}

}  // namespace nn::gfx::util

// The SDK exports these as constants, which lets the shape generators keep them in registers.
namespace nn::util {

typedef uint32_t AngleIndex;

namespace detail {

struct SinCosSample {
    float cosValue;
    float sinValue;
    float cosDelta;
    float sinDelta;
};

extern const float SinCoefficients[5];
extern const float CosCoefficients[5];
extern const AngleIndex AngleIndexHalfRound;
extern const float FloatPiDivided2;
extern const float Float1Divided2Pi;
extern const float FloatPi;
extern const float Float2Pi;
extern const float FloatDegree180;
extern const SinCosSample SinCosSampleTable[256];

}  // namespace detail
}  // namespace nn::util

namespace nn::gfx::util {
namespace {

/**
 * Converts degrees to radians.
 * @param degree angle in degrees
 * @return the angle in radians
 */
inline float DegreeToRadian(float degree) {
    return degree * (nn::util::detail::FloatPi / nn::util::detail::FloatDegree180);
}

/**
 * Converts radians to a table angle index.
 * @param radian angle in radians
 * @return the angle index
 */
inline nn::util::AngleIndex RadianToAngleIndex(float radian) {
    return static_cast<int64_t>(
        radian * (nn::util::detail::AngleIndexHalfRound / nn::util::detail::FloatPi));
}

/**
 * Evaluates the sine through the interpolated sample table.
 * @param angleIndex the angle
 * @return the sine of the angle
 */
inline float SinTable(nn::util::AngleIndex angleIndex) {
    uint32_t sampleTableIndex = (angleIndex >> 24) & 0xFF;
    float rest = static_cast<float>(angleIndex & 0xFFFFFF) / 0x1000000;
    const nn::util::detail::SinCosSample* pSample =
        &nn::util::detail::SinCosSampleTable[sampleTableIndex];
    return pSample->sinValue + pSample->sinDelta * rest;
}

/**
 * Evaluates the cosine through the interpolated sample table.
 * @param angleIndex the angle
 * @return the cosine of the angle
 */
inline float CosTable(nn::util::AngleIndex angleIndex) {
    uint32_t sampleTableIndex = (angleIndex >> 24) & 0xFF;
    float rest = static_cast<float>(angleIndex & 0xFFFFFF) / 0x1000000;
    const nn::util::detail::SinCosSample* pSample =
        &nn::util::detail::SinCosSampleTable[sampleTableIndex];
    return pSample->cosValue + pSample->cosDelta * rest;
}

/**
 * Reduces an angle to the range [-pi, pi].
 * @param radian the angle in radians
 * @return the equivalent angle in [-pi, pi]
 */
inline float ModTwoPi(float radian) {
    using namespace nn::util::detail;

    float quotient = Float1Divided2Pi * radian + (radian >= 0.0f ? 0.5f : -0.5f);
    return radian - Float2Pi * static_cast<int>(quotient);
}

/**
 * Estimates the cosine with a polynomial.
 * @param radian the angle in radians
 * @return the cosine of the angle
 */
inline float CosEst(float radian) {
    using namespace nn::util::detail;

    float value = ModTwoPi(radian);
    float sign;

    if (value > FloatPiDivided2) {
        value = FloatPi - value;
        sign = -1.0f;
    } else if (value < -FloatPiDivided2) {
        value = -FloatPi - value;
        sign = -1.0f;
    } else {
        sign = 1.0f;
    }

    float square = value * value;
    return sign * (((((CosCoefficients[1] - CosCoefficients[0] * square) * square -
                      CosCoefficients[2]) * square + CosCoefficients[3]) * square -
                    CosCoefficients[4]) * square + 1.0f);
}

/**
 * Estimates the sine with a polynomial.
 * @param radian the angle in radians
 * @return the sine of the angle
 */
inline float SinEst(float radian) {
    using namespace nn::util::detail;

    float value = ModTwoPi(radian);

    if (value > FloatPiDivided2) {
        value = FloatPi - value;
    } else if (value < -FloatPiDivided2) {
        value = -FloatPi - value;
    }

    float square = value * value;
    return value * (((((SinCoefficients[1] - SinCoefficients[0] * square) * square -
                       SinCoefficients[2]) * square + SinCoefficients[3]) * square -
                     SinCoefficients[4]) * square + 1.0f);
}

/**
 * Counts the floats of one vertex.
 * @param format attributes written for every vertex
 * @return the number of floats per vertex
 */
inline int GetFloatStride(PrimitiveShapeFormat format) {
    int stride = 0;

    if (format & PrimitiveShapeFormat_Pos) {
        stride += 3;
    }

    if (format & PrimitiveShapeFormat_Normal) {
        stride += 3;
    }

    if (format & PrimitiveShapeFormat_Uv) {
        stride += 2;
    }

    return stride;
}

/**
 * Picks the smallest index format able to address the given number of indices.
 * @param indexCount number of indices
 * @return the index format
 */
inline IndexFormat GetIndexFormat(int indexCount) {
    if (indexCount < 0x10000) {
        return IndexFormat_Uint16;
    }

    return IndexFormat_Uint32;
}

}  // namespace

/**
 * Constructs an empty shape.
 * @param vertexFormat attributes written for every vertex
 * @param topology primitive topology of the index buffer
 */
PrimitiveShape::PrimitiveShape(PrimitiveShapeFormat vertexFormat, PrimitiveTopology topology)
    : m_pIndexBuffer(nullptr), m_pVertexBuffer(nullptr), m_VertexFormat(vertexFormat),
      m_IndexBufferFormat(IndexFormat_Uint32), m_PrimitiveTopology(topology), m_VertexCount(0),
      m_IndexCount(0), m_VertexBufferSize(0), m_IndexBufferSize(0) {}

/**
 * Destroys the shape.
 */
PrimitiveShape::~PrimitiveShape() {}

/** @return the index buffer */
void* PrimitiveShape::GetIndexBuffer() const {
    return m_pIndexBuffer;
}

/** @return the vertex buffer */
void* PrimitiveShape::GetVertexBuffer() const {
    return m_pVertexBuffer;
}

/** @return the size of one vertex in bytes */
size_t PrimitiveShape::GetStride() const {
    size_t stride = 0;

    if (m_VertexFormat & PrimitiveShapeFormat_Pos) {
        stride += sizeof(float) * 3;
    }

    if (m_VertexFormat & PrimitiveShapeFormat_Normal) {
        stride += sizeof(float) * 3;
    }

    if (m_VertexFormat & PrimitiveShapeFormat_Uv) {
        stride += sizeof(float) * 2;
    }

    return stride;
}

/** @return the required vertex buffer size in bytes */
size_t PrimitiveShape::GetVertexBufferSize() const {
    return m_VertexBufferSize;
}

/** @return the required index buffer size in bytes */
size_t PrimitiveShape::GetIndexBufferSize() const {
    return m_IndexBufferSize;
}

/** @return the primitive topology */
PrimitiveTopology PrimitiveShape::GetPrimitiveTopology() const {
    return m_PrimitiveTopology;
}

/** @return the vertex format */
PrimitiveShapeFormat PrimitiveShape::GetVertexFormat() const {
    return m_VertexFormat;
}

/** @return the index format */
IndexFormat PrimitiveShape::GetIndexBufferFormat() const {
    return m_IndexBufferFormat;
}

/** @return the number of vertices */
int PrimitiveShape::GetVertexCount() const {
    return m_VertexCount;
}

/** @return the number of indices */
int PrimitiveShape::GetIndexCount() const {
    return m_IndexCount;
}

/**
 * Fills the given vertex and index buffers.
 * @param pVertexMemory destination of the vertices
 * @param vertexSize size of the vertex memory
 * @param pIndexMemory destination of the indices
 * @param indexSize size of the index memory
 */
void PrimitiveShape::Calculate(void* pVertexMemory, size_t vertexSize, void* pIndexMemory,
                               size_t indexSize) {
    CalculateImpl(pVertexMemory, vertexSize, pIndexMemory, indexSize);
}

/** @param pVertexBuffer the vertex buffer */
void PrimitiveShape::SetVertexBuffer(void* pVertexBuffer) {
    m_pVertexBuffer = pVertexBuffer;
}

/** @param pIndexBuffer the index buffer */
void PrimitiveShape::SetIndexBuffer(void* pIndexBuffer) {
    m_pIndexBuffer = pIndexBuffer;
}

/** @param vertexBufferSize the vertex buffer size in bytes */
void PrimitiveShape::SetVertexBufferSize(size_t vertexBufferSize) {
    m_VertexBufferSize = vertexBufferSize;
}

/** @param indexBufferSize the index buffer size in bytes */
void PrimitiveShape::SetIndexBufferSize(size_t indexBufferSize) {
    m_IndexBufferSize = indexBufferSize;
}

/** @param vertexCount the number of vertices */
void PrimitiveShape::SetVertexCount(int vertexCount) {
    m_VertexCount = vertexCount;
}

/**
 * Sets the number of indices and picks the matching index format.
 * @param indexCount the number of indices
 */
void PrimitiveShape::SetIndexCount(int indexCount) {
    m_IndexBufferFormat = GetIndexFormat(indexCount);
    m_IndexCount = indexCount;
}

/**
 * Constructs a UV sphere of radius 1.
 * @param vertexFormat attributes written for every vertex
 * @param topology LineList or TriangleList
 * @param sliceCount number of subdivisions around the Y axis
 * @param stackCount number of subdivisions from pole to pole
 */
SphereShape::SphereShape(PrimitiveShapeFormat vertexFormat, PrimitiveTopology topology,
                         int sliceCount, int stackCount)
    : PrimitiveShape(vertexFormat, topology), m_SliceCount(sliceCount), m_StackCount(stackCount) {
    SetVertexCount(CalculateVertexCount());
    SetIndexCount(CalculateIndexCount());
    SetVertexBufferSize(GetStride() * GetVertexCount());
    SetIndexBufferSize(sizeof(uint32_t) * GetIndexCount());
}

/** @return the number of vertices of the sphere */
int SphereShape::CalculateVertexCount() {
    return (m_StackCount + 1) * (m_SliceCount + 1);
}

/** @return the number of indices of the sphere */
int SphereShape::CalculateIndexCount() {
    switch (GetPrimitiveTopology()) {
    case PrimitiveTopology_LineList:
        return m_SliceCount * (m_StackCount * 2 - 1) * 2;
    case PrimitiveTopology_TriangleList:
        return ((m_StackCount - 2) * (m_SliceCount * 2) + m_SliceCount * 2) * 3;
    default:
        return 0;
    }
}

/**
 * Destroys the sphere.
 */
SphereShape::~SphereShape() {}

/**
 * Writes the sphere vertices into the vertex buffer.
 * @return the end of the written vertices
 */
void* SphereShape::CalculateVertexBuffer() {
    float* pVertex = static_cast<float*>(GetVertexBuffer());

    for (int stack = 0; stack <= m_StackCount; stack++) {
        float v = static_cast<float>(stack) / m_StackCount;
        float theta = v * nn::util::detail::FloatPi;
        float cosTheta = CosEst(theta);
        float sinTheta = SinEst(theta);

        int sliceCount = m_SliceCount;

        for (int slice = 0; slice <= sliceCount; slice++) {
            float phi =
                -(nn::util::detail::Float2Pi * (static_cast<float>(slice) / m_SliceCount));

            if (slice == m_SliceCount) {
                phi = 0.0f;
            }

            float cosPhi = CosEst(phi);
            float sinPhi = SinEst(phi);

            if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
                pVertex[0] = sinTheta * cosPhi;
                pVertex[1] = cosTheta;
                pVertex[2] = sinTheta * sinPhi;
                pVertex += 3;
            }

            if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
                pVertex[0] = sinTheta * cosPhi;
                pVertex[1] = cosTheta;
                pVertex[2] = sinTheta * sinPhi;
                pVertex += 3;
            }

            if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
                pVertex[0] = static_cast<float>(slice) / m_SliceCount;
                pVertex[1] = v;
                pVertex += 2;
            }
        }
    }

    return pVertex;
}

/**
 * Fills the vertex and index buffers of the sphere.
 * @param pVertexMemory destination of the vertices
 * @param vertexSize size of the vertex memory
 * @param pIndexMemory destination of the indices
 * @param indexSize size of the index memory
 */
void SphereShape::CalculateImpl(void* pVertexMemory, size_t vertexSize, void* pIndexMemory,
                                size_t indexSize) {
    SetVertexBuffer(pVertexMemory);
    CalculateVertexBuffer();
    SetIndexBuffer(pIndexMemory);

    switch (GetIndexBufferFormat()) {
    case IndexFormat_Uint16:
        CalculateIndexBuffer<uint16_t>();
        break;
    case IndexFormat_Uint32:
        CalculateIndexBuffer<uint32_t>();
        break;
    default:
        NN_UNEXPECTED_DEFAULT;
    }
}

/**
 * Writes the sphere indices into the index buffer.
 * @tparam T index type
 */
template <typename T>
void SphereShape::CalculateIndexBuffer() {
    T* pIndexBuffer = static_cast<T*>(GetIndexBuffer());
    int index = 0;

    switch (GetPrimitiveTopology()) {
    case PrimitiveTopology_LineList:
        for (int slice = 0; slice < m_SliceCount; slice++) {
            int rowSize = m_SliceCount + 1;

            for (int stack = 0; stack < m_StackCount; stack++) {
                pIndexBuffer[index++] = stack * rowSize + slice;
                pIndexBuffer[index++] = (stack + 1) * rowSize + slice;
            }
        }

        for (int stack = 1; stack < m_StackCount; stack++) {
            int rowStart = stack * (m_SliceCount + 1);

            for (int slice = 0; slice < m_SliceCount; slice++) {
                pIndexBuffer[index++] = rowStart + slice;
                pIndexBuffer[index++] = rowStart + slice + 1;
            }
        }

        break;
    case PrimitiveTopology_TriangleList: {
        int triangle = 0;

        for (int slice = 0; slice < m_SliceCount; slice++) {
            pIndexBuffer[triangle * 3 + 0] = slice;
            pIndexBuffer[triangle * 3 + 1] = slice + m_SliceCount + 1;
            pIndexBuffer[triangle * 3 + 2] = slice + m_SliceCount + 2;
            triangle++;
        }

        int stack = 1;

        for (; stack < m_StackCount - 1; stack++) {
            for (int slice = 0; slice < m_SliceCount; slice++) {
                pIndexBuffer[triangle * 3 + 0] = stack * (m_SliceCount + 1) + slice;
                pIndexBuffer[triangle * 3 + 1] =
                    stack * (m_SliceCount + 1) + m_SliceCount + 1 + slice + 1;
                pIndexBuffer[triangle * 3 + 2] = stack * (m_SliceCount + 1) + slice + 1;
                triangle++;

                pIndexBuffer[triangle * 3 + 0] = stack * (m_SliceCount + 1) + slice + 1;
                pIndexBuffer[triangle * 3 + 1] =
                    stack * (m_SliceCount + 1) + m_SliceCount + 1 + slice;
                pIndexBuffer[triangle * 3 + 2] =
                    stack * (m_SliceCount + 1) + m_SliceCount + 1 + slice + 1;
                triangle++;
            }
        }

        for (int slice = 0; slice < m_SliceCount; slice++) {
            pIndexBuffer[triangle * 3 + 0] = stack * (m_SliceCount + 1) + slice;
            pIndexBuffer[triangle * 3 + 1] = stack * (m_SliceCount + 1) + m_SliceCount + 1 + slice;
            pIndexBuffer[triangle * 3 + 2] = stack * (m_SliceCount + 1) + slice + 1;
            triangle++;
        }

        break;
    }
    default:
        break;
    }
}

/**
 * Constructs a flat circle of radius 1 in the XY plane.
 * @param vertexFormat attributes written for every vertex
 * @param topology LineStrip or TriangleList
 * @param sliceCount number of subdivisions of the circumference
 */
CircleShape::CircleShape(PrimitiveShapeFormat vertexFormat, PrimitiveTopology topology,
                         int sliceCount)
    : PrimitiveShape(vertexFormat, topology), m_SliceCount(sliceCount) {
    SetVertexCount(CalculateVertexCount());
    SetIndexCount(CalculateIndexCount());
    SetVertexBufferSize(GetStride() * GetVertexCount());
    SetIndexBufferSize(sizeof(uint32_t) * GetIndexCount());
}

/** @return the number of vertices of the circle */
int CircleShape::CalculateVertexCount() {
    return m_SliceCount + 1;
}

/** @return the number of indices of the circle */
int CircleShape::CalculateIndexCount() {
    switch (GetPrimitiveTopology()) {
    case PrimitiveTopology_LineStrip:
        return m_SliceCount + 1;
    case PrimitiveTopology_TriangleList:
        return m_SliceCount * 3;
    default:
        return 0;
    }
}

/**
 * Destroys the circle.
 */
CircleShape::~CircleShape() {}

/**
 * Writes the circle vertices (rim first, center last) into the vertex buffer.
 * @return the end of the written vertices
 */
void* CircleShape::CalculateVertexBuffer() {
    float* pVertex = static_cast<float*>(GetVertexBuffer());

    for (int i = 0; i < m_SliceCount; i++) {
        nn::util::AngleIndex angle =
            RadianToAngleIndex(DegreeToRadian(360.0f) * i / m_SliceCount);
        float cos = CosTable(angle);
        float sin = SinTable(angle);

        if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
            pVertex[0] = cos;
            pVertex[1] = sin;
            pVertex[2] = 0.0f;
            pVertex += 3;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
            pVertex[0] = 0.0f;
            pVertex[1] = 0.0f;
            pVertex[2] = 1.0f;
            pVertex += 3;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
            pVertex[0] = cos * 0.5f + 0.5f;
            pVertex[1] = 1.0f - (sin * 0.5f + 0.5f);
            pVertex += 2;
        }
    }

    if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
        pVertex[0] = 0.0f;
        pVertex[1] = 0.0f;
        pVertex[2] = 0.0f;
        pVertex += 3;
    }

    if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
        pVertex[0] = 0.0f;
        pVertex[1] = 0.0f;
        pVertex[2] = 1.0f;
        pVertex += 3;
    }

    if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
        pVertex[0] = 0.5f;
        pVertex[1] = 0.5f;
        pVertex += 2;
    }

    return pVertex;
}

/**
 * Writes the circle indices into the index buffer.
 * @tparam T index type
 */
template <typename T>
void CircleShape::CalculateIndexBuffer() {
    T* pIndexBuffer = static_cast<T*>(GetIndexBuffer());

    switch (GetPrimitiveTopology()) {
    case PrimitiveTopology_LineStrip:
        for (int i = 0; i < GetIndexCount(); i++) {
            pIndexBuffer[i] = i % m_SliceCount;
        }

        break;
    case PrimitiveTopology_TriangleList: {
        uint32_t index = 0;

        for (int i = 0; i < m_SliceCount; i++) {
            pIndexBuffer[index++] = i;
            pIndexBuffer[index++] = (i + 1) % m_SliceCount;
            pIndexBuffer[index++] = m_SliceCount;
        }

        break;
    }
    default:
        break;
    }
}

/**
 * Fills the vertex and index buffers of the circle.
 * @param pVertexMemory destination of the vertices
 * @param vertexSize size of the vertex memory
 * @param pIndexMemory destination of the indices
 * @param indexSize size of the index memory
 */
void CircleShape::CalculateImpl(void* pVertexMemory, size_t vertexSize, void* pIndexMemory,
                                size_t indexSize) {
    SetVertexBuffer(pVertexMemory);
    CalculateVertexBuffer();
    SetIndexBuffer(pIndexMemory);

    switch (GetIndexBufferFormat()) {
    case IndexFormat_Uint16:
        CalculateIndexBuffer<uint16_t>();
        break;
    case IndexFormat_Uint32:
        CalculateIndexBuffer<uint32_t>();
        break;
    default:
        NN_UNEXPECTED_DEFAULT;
    }
}

namespace {

/** Corners of the cube, scaled by 0.5 when written. */
const float CubeVertices[8][3] = {
    {1.0f, 1.0f, -1.0f},  {1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}, {-1.0f, 1.0f, -1.0f},
    {1.0f, 1.0f, 1.0f},   {1.0f, -1.0f, 1.0f},  {-1.0f, -1.0f, 1.0f},  {-1.0f, 1.0f, 1.0f},
};

/** Normal of each face. */
const float CubeNormals[6][3] = {
    {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 0.0f},  {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
};

/** Corner indices of each face. */
const int CubeFaceIndices[6][4] = {
    {0, 1, 2, 3}, {3, 2, 6, 7}, {7, 6, 5, 4}, {4, 5, 1, 0}, {1, 5, 6, 2}, {4, 0, 3, 7},
};

/** Texture coordinates of the four corners of a face. */
const float CubeUvs[4][2] = {
    {0.0f, 0.0f},
    {0.0f, 1.0f},
    {1.0f, 1.0f},
    {1.0f, 0.0f},
};

}  // namespace

/**
 * Constructs a cube of edge length 1 centered on the origin.
 * @param vertexFormat attributes written for every vertex
 * @param topology LineList or TriangleList
 */
CubeShape::CubeShape(PrimitiveShapeFormat vertexFormat, PrimitiveTopology topology)
    : PrimitiveShape(vertexFormat, topology) {
    SetVertexCount(CalculateVertexCount());
    SetIndexCount(CalculateIndexCount());
    SetVertexBufferSize(GetStride() * GetVertexCount());
    SetIndexBufferSize(sizeof(uint32_t) * GetIndexCount());
}

/** @return the number of vertices of the cube */
int CubeShape::CalculateVertexCount() {
    switch (GetPrimitiveTopology()) {
    case PrimitiveTopology_LineList:
        return CubeVertexCount_Wired;
    case PrimitiveTopology_TriangleList:
        return CubeVertexCount_Solid;
    default:
        return 0;
    }
}

/** @return the number of indices of the cube */
int CubeShape::CalculateIndexCount() {
    switch (GetPrimitiveTopology()) {
    case PrimitiveTopology_LineList:
        return CubeIndexCount_Wired;
    case PrimitiveTopology_TriangleList:
        return CubeIndexCount_Solid;
    default:
        return 0;
    }
}

/**
 * Destroys the cube.
 */
CubeShape::~CubeShape() {}

/**
 * Writes the cube vertices into the vertex buffer. The wire frame only has positions.
 * @return the end of the written vertices
 */
void* CubeShape::CalculateVertexBuffer() {
    float* pVertex = static_cast<float*>(GetVertexBuffer());

    if (GetPrimitiveTopology() == PrimitiveTopology_LineList) {
        for (int i = 0; i < CubeVertexCount_Wired; i++) {
            pVertex[0] = CubeVertices[i][0] * 0.5f;
            pVertex[1] = CubeVertices[i][1] * 0.5f;
            pVertex[2] = CubeVertices[i][2] * 0.5f;
            pVertex += 3;
        }

        return pVertex;
    }

    for (int face = 0; face < 6; face++) {
        for (int corner = 0; corner < 4; corner++) {
            const float* pPosition = CubeVertices[CubeFaceIndices[face][corner]];

            if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
                pVertex[0] = pPosition[0] * 0.5f;
                pVertex[1] = pPosition[1] * 0.5f;
                pVertex[2] = pPosition[2] * 0.5f;
                pVertex += 3;
            }

            if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
                pVertex[0] = CubeNormals[face][0];
                pVertex[1] = CubeNormals[face][1];
                pVertex[2] = CubeNormals[face][2];
                pVertex += 3;
            }

            if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
                pVertex[0] = CubeUvs[corner][0];
                pVertex[1] = CubeUvs[corner][1];
                pVertex += 2;
            }
        }
    }

    return pVertex;
}

/**
 * Fills the vertex and index buffers of the cube.
 * @param pVertexMemory destination of the vertices
 * @param vertexSize size of the vertex memory
 * @param pIndexMemory destination of the indices
 * @param indexSize size of the index memory
 */
void CubeShape::CalculateImpl(void* pVertexMemory, size_t vertexSize, void* pIndexMemory,
                              size_t indexSize) {
    SetVertexBuffer(pVertexMemory);
    CalculateVertexBuffer();
    SetIndexBuffer(pIndexMemory);

    switch (GetIndexBufferFormat()) {
    case IndexFormat_Uint16:
        CalculateIndexBuffer<uint16_t>();
        break;
    case IndexFormat_Uint32:
        CalculateIndexBuffer<uint32_t>();
        break;
    default:
        NN_UNEXPECTED_DEFAULT;
    }
}

/**
 * Writes the cube indices into the index buffer.
 * @tparam T index type
 */
template <typename T>
void CubeShape::CalculateIndexBuffer() {
    T* pIndexBuffer = static_cast<T*>(GetIndexBuffer());
    int index = 0;

    switch (GetPrimitiveTopology()) {
    case PrimitiveTopology_LineList:
        for (int face = 0; face < 6; face++) {
            for (int corner = 0; corner < 4; corner++) {
                pIndexBuffer[index++] = CubeFaceIndices[face][corner];
                pIndexBuffer[index++] = CubeFaceIndices[face][(corner + 1) % 4];
            }
        }

        break;
    case PrimitiveTopology_TriangleList:
        for (int face = 0; face < 6; face++) {
            int base = face * 4;
            pIndexBuffer[index++] = base;
            pIndexBuffer[index++] = base + 1;
            pIndexBuffer[index++] = base + 2;
            pIndexBuffer[index++] = base;
            pIndexBuffer[index++] = base + 2;
            pIndexBuffer[index++] = base + 3;
        }

        break;
    default:
        break;
    }
}

/**
 * Constructs a quad of edge length 2 in the XY plane.
 * @param vertexFormat attributes written for every vertex
 * @param topology LineStrip or TriangleList
 */
QuadShape::QuadShape(PrimitiveShapeFormat vertexFormat, PrimitiveTopology topology)
    : PrimitiveShape(vertexFormat, topology) {
    SetVertexCount(CalculateVertexCount());
    SetIndexCount(CalculateIndexCount());
    SetVertexBufferSize(GetStride() * GetVertexCount());
    SetIndexBufferSize(sizeof(uint32_t) * GetIndexCount());
}

/** @return the number of vertices of the quad */
int QuadShape::CalculateVertexCount() {
    return QuadVertexCount;
}

/** @return the number of indices of the quad */
int QuadShape::CalculateIndexCount() {
    switch (GetPrimitiveTopology()) {
    case PrimitiveTopology_LineStrip:
        return QuadIndexCount_Wired;
    case PrimitiveTopology_TriangleList:
        return QuadIndexCountt_Solid;
    default:
        return 0;
    }
}

/**
 * Destroys the quad.
 */
QuadShape::~QuadShape() {}

namespace {

/** Corners of the quad. */
const float QuadVertices[4][3] = {
    {-1.0f, 1.0f, 0.0f},
    {1.0f, 1.0f, 0.0f},
    {-1.0f, -1.0f, 0.0f},
    {1.0f, -1.0f, 0.0f},
};

/** Texture coordinates of the corners of the quad. */
const float QuadUvs[4][2] = {
    {0.0f, 0.0f},
    {1.0f, 0.0f},
    {0.0f, 1.0f},
    {1.0f, 1.0f},
};

}  // namespace

/**
 * Writes the quad vertices into the vertex buffer. The wire frame only has positions.
 * @return the end of the written vertices
 */
void* QuadShape::CalculateVertexBuffer() {
    float* pVertex = static_cast<float*>(GetVertexBuffer());

    if (GetPrimitiveTopology() == PrimitiveTopology_LineStrip) {
        for (int i = 0; i < QuadVertexCount; i++) {
            pVertex[0] = QuadVertices[i][0];
            pVertex[1] = QuadVertices[i][1];
            pVertex[2] = QuadVertices[i][2];
            pVertex += 3;
        }

        return pVertex;
    }

    for (int i = 0; i < QuadVertexCount; i++) {
        if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
            pVertex[0] = QuadVertices[i][0];
            pVertex[1] = QuadVertices[i][1];
            pVertex[2] = QuadVertices[i][2];
            pVertex += 3;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
            pVertex[0] = 0.0f;
            pVertex[1] = 0.0f;
            pVertex[2] = 1.0f;
            pVertex += 3;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
            pVertex[0] = QuadUvs[i][0];
            pVertex[1] = QuadUvs[i][1];
            pVertex += 2;
        }
    }

    return pVertex;
}

/**
 * Writes the quad indices into the index buffer.
 * @tparam T index type
 */
template <typename T>
void QuadShape::CalculateIndexBuffer() {
    T* pIndexBuffer = static_cast<T*>(GetIndexBuffer());

    switch (GetPrimitiveTopology()) {
    case PrimitiveTopology_LineStrip:
        pIndexBuffer[0] = 0;
        pIndexBuffer[1] = 1;
        pIndexBuffer[2] = 3;
        pIndexBuffer[3] = 2;
        pIndexBuffer[4] = 0;
        break;
    case PrimitiveTopology_TriangleList:
        pIndexBuffer[0] = 0;
        pIndexBuffer[1] = 2;
        pIndexBuffer[2] = 1;
        pIndexBuffer[3] = 1;
        pIndexBuffer[4] = 2;
        pIndexBuffer[5] = 3;
        break;
    default:
        break;
    }
}

/**
 * Fills the vertex and index buffers of the quad.
 * @param pVertexMemory destination of the vertices
 * @param vertexSize size of the vertex memory
 * @param pIndexMemory destination of the indices
 * @param indexSize size of the index memory
 */
void QuadShape::CalculateImpl(void* pVertexMemory, size_t vertexSize, void* pIndexMemory,
                              size_t indexSize) {
    SetVertexBuffer(pVertexMemory);
    CalculateVertexBuffer();
    SetIndexBuffer(pIndexMemory);

    switch (GetIndexBufferFormat()) {
    case IndexFormat_Uint16:
        CalculateIndexBuffer<uint16_t>();
        break;
    case IndexFormat_Uint32:
        CalculateIndexBuffer<uint32_t>();
        break;
    default:
        NN_UNEXPECTED_DEFAULT;
    }
}

/**
 * Constructs the upper half of a UV sphere of radius 1.
 * @param vertexFormat attributes written for every vertex
 * @param topology LineList or TriangleList
 * @param sliceCount number of subdivisions around the Y axis
 */
HemiSphereShape::HemiSphereShape(PrimitiveShapeFormat vertexFormat, PrimitiveTopology topology,
                                 int sliceCount)
    : PrimitiveShape(vertexFormat, topology), m_SliceCount(sliceCount) {
    SetVertexCount(CalculateVertexCount());
    SetIndexCount(CalculateIndexCount());
    SetVertexBufferSize(GetStride() * GetVertexCount());
    SetIndexBufferSize(sizeof(uint32_t) * GetIndexCount());
}

/** @return the number of vertices of the hemisphere */
int HemiSphereShape::CalculateVertexCount() {
    return (m_SliceCount / 2 + 1) * (m_SliceCount + 1);
}

/** @return the number of indices of the hemisphere */
int HemiSphereShape::CalculateIndexCount() {
    int stackCount = m_SliceCount / 2;

    switch (GetPrimitiveTopology()) {
    case PrimitiveTopology_LineList:
        return m_SliceCount * stackCount * 4;
    case PrimitiveTopology_TriangleList:
        return (m_SliceCount + m_SliceCount * (stackCount - 1) * 2) * 3;
    default:
        return 0;
    }
}

/**
 * Destroys the hemisphere.
 */
HemiSphereShape::~HemiSphereShape() {}

/**
 * Writes the hemisphere vertices into the vertex buffer.
 * @return the end of the written vertices
 */
void* HemiSphereShape::CalculateVertexBuffer() {
    float* pVertex = static_cast<float*>(GetVertexBuffer());
    int stackCount = m_SliceCount / 2;

    for (int stack = 0; stack <= stackCount; stack++) {
        float v = static_cast<float>(stack) / stackCount;
        float theta = v * (nn::util::detail::FloatPi * 0.5f);
        float cosTheta = CosEst(theta);
        float sinTheta = SinEst(theta);

        int sliceCount = m_SliceCount;

        for (int slice = 0; slice <= sliceCount; slice++) {
            float phi =
                -(nn::util::detail::Float2Pi * (static_cast<float>(slice) / m_SliceCount));

            if (slice == m_SliceCount) {
                phi = 0.0f;
            }

            float cosPhi = CosEst(phi);
            float sinPhi = SinEst(phi);

            if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
                pVertex[0] = sinTheta * cosPhi;
                pVertex[1] = cosTheta;
                pVertex[2] = sinTheta * sinPhi;
                pVertex += 3;
            }

            if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
                pVertex[0] = sinTheta * cosPhi;
                pVertex[1] = cosTheta;
                pVertex[2] = sinTheta * sinPhi;
                pVertex += 3;
            }

            if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
                pVertex[0] = static_cast<float>(slice) / m_SliceCount;
                pVertex[1] = v;
                pVertex += 2;
            }
        }
    }

    return pVertex;
}

/**
 * Fills the vertex and index buffers of the hemisphere.
 * @param pVertexMemory destination of the vertices
 * @param vertexSize size of the vertex memory
 * @param pIndexMemory destination of the indices
 * @param indexSize size of the index memory
 */
void HemiSphereShape::CalculateImpl(void* pVertexMemory, size_t vertexSize, void* pIndexMemory,
                                    size_t indexSize) {
    SetVertexBuffer(pVertexMemory);
    CalculateVertexBuffer();
    SetIndexBuffer(pIndexMemory);

    switch (GetIndexBufferFormat()) {
    case IndexFormat_Uint16:
        CalculateIndexBuffer<uint16_t>();
        break;
    case IndexFormat_Uint32:
        CalculateIndexBuffer<uint32_t>();
        break;
    default:
        NN_UNEXPECTED_DEFAULT;
    }
}

/**
 * Writes the hemisphere indices into the index buffer.
 * @tparam T index type
 */
template <typename T>
void HemiSphereShape::CalculateIndexBuffer() {
    T* pIndexBuffer = static_cast<T*>(GetIndexBuffer());
    int stackCount = m_SliceCount / 2;
    int index = 0;

    switch (GetPrimitiveTopology()) {
    case PrimitiveTopology_LineList:
        for (int slice = 0; slice < m_SliceCount; slice++) {
            int rowSize = m_SliceCount + 1;

            for (int stack = 0; stack < stackCount; stack++) {
                pIndexBuffer[index++] = stack * rowSize + slice;
                pIndexBuffer[index++] = (stack + 1) * rowSize + slice;
            }
        }

        for (int stack = 1; stack <= stackCount; stack++) {
            int rowStart = stack * (m_SliceCount + 1);

            for (int slice = 0; slice < m_SliceCount; slice++) {
                pIndexBuffer[index++] = rowStart + slice;
                pIndexBuffer[index++] = rowStart + slice + 1;
            }
        }

        break;
    case PrimitiveTopology_TriangleList: {
        int triangle = 0;

        for (int slice = 0; slice < m_SliceCount; slice++) {
            pIndexBuffer[triangle * 3 + 0] = slice;
            pIndexBuffer[triangle * 3 + 1] = slice + m_SliceCount + 1;
            pIndexBuffer[triangle * 3 + 2] = slice + m_SliceCount + 2;
            triangle++;
        }

        for (int stack = 1; stack < stackCount; stack++) {
            for (int slice = 0; slice < m_SliceCount; slice++) {
                pIndexBuffer[triangle * 3 + 0] = stack * (m_SliceCount + 1) + slice;
                pIndexBuffer[triangle * 3 + 1] =
                    stack * (m_SliceCount + 1) + m_SliceCount + 1 + slice + 1;
                pIndexBuffer[triangle * 3 + 2] = stack * (m_SliceCount + 1) + slice + 1;
                triangle++;

                pIndexBuffer[triangle * 3 + 0] = stack * (m_SliceCount + 1) + slice + 1;
                pIndexBuffer[triangle * 3 + 1] =
                    stack * (m_SliceCount + 1) + m_SliceCount + 1 + slice;
                pIndexBuffer[triangle * 3 + 2] =
                    stack * (m_SliceCount + 1) + m_SliceCount + 1 + slice + 1;
                triangle++;
            }
        }

        break;
    }
    default:
        break;
    }
}

/**
 * Constructs an open tube of radius 1 and height 1.
 * @param vertexFormat attributes written for every vertex
 * @param topology LineList or TriangleList
 * @param sliceCount number of subdivisions of the circumference
 */
PipeShape::PipeShape(PrimitiveShapeFormat vertexFormat, PrimitiveTopology topology,
                     int sliceCount)
    : PrimitiveShape(vertexFormat, topology), m_SliceCount(sliceCount) {
    SetVertexCount(CalculateVertexCount());
    SetIndexCount(CalculateIndexCount());
    SetVertexBufferSize(GetStride() * GetVertexCount());
    SetIndexBufferSize(sizeof(uint32_t) * GetIndexCount());
}

/** @return the number of vertices of the pipe */
int PipeShape::CalculateVertexCount() {
    return m_SliceCount * 2;
}

/** @return the number of indices of the pipe */
int PipeShape::CalculateIndexCount() {
    return m_SliceCount * 6;
}

/**
 * Destroys the pipe.
 */
PipeShape::~PipeShape() {}

/**
 * Writes the pipe vertices (bottom ring, then top ring) into the vertex buffer.
 * @return the end of the vertex buffer
 */
void* PipeShape::CalculateVertexBuffer() {
    float* pVertex = static_cast<float*>(GetVertexBuffer());
    int stride = GetFloatStride(GetVertexFormat());
    int index = 0;

    for (int i = 0; i < m_SliceCount; i++) {
        nn::util::AngleIndex angle =
            RadianToAngleIndex(nn::util::FloatPi * (2 * (i + 1)) / m_SliceCount);
        float sin = SinTable(angle);
        float cos = CosTable(angle);

        if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
            pVertex[index++] = cos;
            pVertex[index++] = 0.0f;
            pVertex[index++] = sin;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
            pVertex[index++] = cos;
            pVertex[index++] = 0.0f;
            pVertex[index++] = sin;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
            pVertex[index++] = cos * 0.5f + 0.5f;
            pVertex[index++] = 1.0f;
        }

        int topIndex = (m_SliceCount + i) * stride;

        if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
            pVertex[topIndex++] = cos;
            pVertex[topIndex++] = 1.0f;
            pVertex[topIndex++] = sin;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
            pVertex[topIndex++] = cos;
            pVertex[topIndex++] = 0.0f;
            pVertex[topIndex++] = sin;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
            pVertex[topIndex++] = cos * 0.5f + 0.5f;
            pVertex[topIndex++] = 0.0f;
        }
    }

    return pVertex + GetVertexBufferSize();
}

/**
 * Writes the pipe indices into the index buffer.
 * @tparam T index type
 */
template <typename T>
void PipeShape::CalculateIndexBuffer() {
    T* pIndexBuffer = static_cast<T*>(GetIndexBuffer());
    PrimitiveTopology topology = GetPrimitiveTopology();
    uint32_t index = 0;

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_SliceCount); i++) {
        uint32_t top = (i + m_SliceCount) % GetVertexCount();
        uint32_t next = (i + 1) % m_SliceCount;
        uint32_t nextTop = next + m_SliceCount;

        if (topology == PrimitiveTopology_LineList) {
            pIndexBuffer[index++] = i;
            pIndexBuffer[index++] = next;
            pIndexBuffer[index++] = top;
            pIndexBuffer[index++] = nextTop;
            pIndexBuffer[index++] = top;
            pIndexBuffer[index++] = i;
        } else if (topology == PrimitiveTopology_TriangleList) {
            pIndexBuffer[index++] = i;
            pIndexBuffer[index++] = top;
            pIndexBuffer[index++] = next;
            pIndexBuffer[index++] = next;
            pIndexBuffer[index++] = top;
            pIndexBuffer[index++] = nextTop;
        }
    }
}

/**
 * Fills the vertex and index buffers of the pipe.
 * @param pVertexMemory destination of the vertices
 * @param vertexSize size of the vertex memory
 * @param pIndexMemory destination of the indices
 * @param indexSize size of the index memory
 */
void PipeShape::CalculateImpl(void* pVertexMemory, size_t vertexSize, void* pIndexMemory,
                              size_t indexSize) {
    SetVertexBuffer(pVertexMemory);
    CalculateVertexBuffer();
    SetIndexBuffer(pIndexMemory);

    switch (GetIndexBufferFormat()) {
    case IndexFormat_Uint16:
        CalculateIndexBuffer<uint16_t>();
        break;
    case IndexFormat_Uint32:
        CalculateIndexBuffer<uint32_t>();
        break;
    default:
        NN_UNEXPECTED_DEFAULT;
    }
}

/**
 * Constructs a closed cylinder of radius 1 and height 1.
 * @param vertexFormat attributes written for every vertex
 * @param topology LineList or TriangleList
 * @param sliceCount number of subdivisions of the circumference
 */
CylinderShape::CylinderShape(PrimitiveShapeFormat vertexFormat, PrimitiveTopology topology,
                             int sliceCount)
    : PrimitiveShape(vertexFormat, topology), m_SliceCount(sliceCount) {
    SetVertexCount(CalculateVertexCount());
    SetIndexCount(CalculateIndexCount());
    SetVertexBufferSize(GetStride() * GetVertexCount());
    SetIndexBufferSize(sizeof(uint32_t) * GetIndexCount());
}

/** @return the number of vertices of the cylinder */
int CylinderShape::CalculateVertexCount() {
    return m_SliceCount * 4 + 2;
}

/** @return the number of indices of the cylinder */
int CylinderShape::CalculateIndexCount() {
    switch (GetPrimitiveTopology()) {
    case PrimitiveTopology_LineList:
        return m_SliceCount * 18;
    case PrimitiveTopology_TriangleList:
        return m_SliceCount * 12;
    default:
        return 0;
    }
}

/**
 * Destroys the cylinder.
 */
CylinderShape::~CylinderShape() {}

/**
 * Writes the cylinder vertices (side rings, cap rings, cap centers) into the vertex buffer.
 * @return the end of the vertex buffer
 */
void* CylinderShape::CalculateVertexBuffer() {
    float* pVertex = static_cast<float*>(GetVertexBuffer());
    int stride = GetFloatStride(GetVertexFormat());
    int index = 0;

    for (int i = 1; i <= m_SliceCount; i++) {
        nn::util::AngleIndex angle =
            RadianToAngleIndex(nn::util::FloatPi * (2 * i) / m_SliceCount);
        float sin = SinTable(angle);
        float cos = CosTable(angle);

        if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
            pVertex[index++] = cos;
            pVertex[index++] = 0.0f;
            pVertex[index++] = sin;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
            pVertex[index++] = cos;
            pVertex[index++] = 0.0f;
            pVertex[index++] = sin;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
            pVertex[index++] = cos * 0.5f + 0.5f;
            pVertex[index++] = 1.0f;
        }

        int topIndex = (m_SliceCount + i - 1) * stride;

        if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
            pVertex[topIndex++] = cos;
            pVertex[topIndex++] = 1.0f;
            pVertex[topIndex++] = sin;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
            pVertex[topIndex++] = cos;
            pVertex[topIndex++] = 0.0f;
            pVertex[topIndex++] = sin;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
            pVertex[topIndex++] = cos * 0.5f + 0.5f;
            pVertex[topIndex++] = 0.0f;
        }
    }

    int bottomIndex = stride * m_SliceCount * 2;
    int topIndex = (m_SliceCount * 3 + 1) * stride;

    for (int i = 0; i < m_SliceCount; i++) {
        nn::util::AngleIndex angle =
            RadianToAngleIndex(nn::util::FloatPi * (2 * (i + 1)) / m_SliceCount);
        float sin = SinTable(angle);
        float cos = CosTable(angle);

        if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
            pVertex[bottomIndex++] = cos;
            pVertex[bottomIndex++] = 0.0f;
            pVertex[bottomIndex++] = sin;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
            pVertex[bottomIndex++] = cos;
            pVertex[bottomIndex++] = -1.0f;
            pVertex[bottomIndex++] = sin;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
            pVertex[bottomIndex++] = cos * 0.5f + 0.5f;
            pVertex[bottomIndex++] = 1.0f - (sin * 0.5f + 0.5f);
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
            pVertex[topIndex++] = cos;
            pVertex[topIndex++] = 1.0f;
            pVertex[topIndex++] = sin;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
            pVertex[topIndex++] = cos;
            pVertex[topIndex++] = 0.0f;
            pVertex[topIndex++] = sin;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
            pVertex[topIndex++] = cos * 0.5f + 0.5f;
            pVertex[topIndex++] = 1.0f - (sin * 0.5f + 0.5f);
        }
    }

    if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
        pVertex[bottomIndex++] = 0.0f;
        pVertex[bottomIndex++] = 0.0f;
        pVertex[bottomIndex++] = 0.0f;
    }

    if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
        pVertex[bottomIndex++] = 0.0f;
        pVertex[bottomIndex++] = -1.0f;
        pVertex[bottomIndex++] = 0.0f;
    }

    if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
        pVertex[bottomIndex++] = 0.5f;
        pVertex[bottomIndex++] = 0.5f;
    }

    if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
        pVertex[topIndex++] = 0.0f;
        pVertex[topIndex++] = 1.0f;
        pVertex[topIndex++] = 0.0f;
    }

    if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
        pVertex[topIndex++] = 0.0f;
        pVertex[topIndex++] = 1.0f;
        pVertex[topIndex++] = 0.0f;
    }

    if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
        pVertex[topIndex++] = 0.5f;
        pVertex[topIndex++] = 0.5f;
    }

    return pVertex + GetVertexBufferSize();
}

/**
 * Fills the vertex and index buffers of the cylinder.
 * @param pVertexMemory destination of the vertices
 * @param vertexSize size of the vertex memory
 * @param pIndexMemory destination of the indices
 * @param indexSize size of the index memory
 */
void CylinderShape::CalculateImpl(void* pVertexMemory, size_t vertexSize, void* pIndexMemory,
                                  size_t indexSize) {
    SetVertexBuffer(pVertexMemory);
    CalculateVertexBuffer();
    SetIndexBuffer(pIndexMemory);

    switch (GetIndexBufferFormat()) {
    case IndexFormat_Uint16:
        CalculateIndexBuffer<uint16_t>();
        break;
    case IndexFormat_Uint32:
        CalculateIndexBuffer<uint32_t>();
        break;
    default:
        NN_UNEXPECTED_DEFAULT;
    }
}

/**
 * Writes the cylinder indices into the index buffer.
 * @tparam T index type
 */
template <typename T>
void CylinderShape::CalculateIndexBuffer() {
    T* pIndexBuffer = static_cast<T*>(GetIndexBuffer());
    PrimitiveTopology topology = GetPrimitiveTopology();
    uint32_t sideModulo = m_SliceCount * m_SliceCount;
    uint32_t index = 0;

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_SliceCount); i++) {
        uint32_t top = (i + m_SliceCount) % sideModulo;
        uint32_t next = (i + 1) % m_SliceCount;
        uint32_t nextTop = next + m_SliceCount;

        if (topology == PrimitiveTopology_LineList) {
            pIndexBuffer[index++] = i;
            pIndexBuffer[index++] = next;
            pIndexBuffer[index++] = top;
            pIndexBuffer[index++] = nextTop;
            pIndexBuffer[index++] = top;
            pIndexBuffer[index++] = i;
        } else if (topology == PrimitiveTopology_TriangleList) {
            pIndexBuffer[index++] = i;
            pIndexBuffer[index++] = top;
            pIndexBuffer[index++] = next;
            pIndexBuffer[index++] = next;
            pIndexBuffer[index++] = top;
            pIndexBuffer[index++] = nextTop;
        }
    }

    uint32_t bottomStart = m_SliceCount * 2;

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_SliceCount); i++) {
        uint32_t center = m_SliceCount + bottomStart;
        uint32_t current = bottomStart + i;
        uint32_t next = (i + 1) % m_SliceCount + bottomStart;

        if (topology == PrimitiveTopology_LineList) {
            pIndexBuffer[m_SliceCount * 6 + i * 6 + 0] = center;
            pIndexBuffer[m_SliceCount * 6 + i * 6 + 1] = current;
            pIndexBuffer[m_SliceCount * 6 + i * 6 + 2] = center;
            pIndexBuffer[m_SliceCount * 6 + i * 6 + 3] = next;
            pIndexBuffer[m_SliceCount * 6 + i * 6 + 4] = current;
            pIndexBuffer[m_SliceCount * 6 + i * 6 + 5] = next;
        } else if (topology == PrimitiveTopology_TriangleList) {
            pIndexBuffer[m_SliceCount * 6 + i * 3 + 0] = center;
            pIndexBuffer[m_SliceCount * 6 + i * 3 + 1] = current;
            pIndexBuffer[m_SliceCount * 6 + i * 3 + 2] = next;
        }
    }

    uint32_t topStart = m_SliceCount + bottomStart + 1;

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_SliceCount); i++) {
        uint32_t center = m_SliceCount + topStart;
        uint32_t current = topStart + i;
        uint32_t next = (i + 1) % m_SliceCount + topStart;

        if (topology == PrimitiveTopology_LineList) {
            pIndexBuffer[m_SliceCount * 12 + i * 6 + 0] = center;
            pIndexBuffer[m_SliceCount * 12 + i * 6 + 1] = current;
            pIndexBuffer[m_SliceCount * 12 + i * 6 + 2] = center;
            pIndexBuffer[m_SliceCount * 12 + i * 6 + 3] = next;
            pIndexBuffer[m_SliceCount * 12 + i * 6 + 4] = current;
            pIndexBuffer[m_SliceCount * 12 + i * 6 + 5] = next;
        } else if (topology == PrimitiveTopology_TriangleList) {
            pIndexBuffer[m_SliceCount * 9 + i * 3 + 0] = center;
            pIndexBuffer[m_SliceCount * 9 + i * 3 + 1] = next;
            pIndexBuffer[m_SliceCount * 9 + i * 3 + 2] = current;
        }
    }
}

/**
 * Constructs a cone of radius 1 and height 1 with its apex on the Y axis.
 * @param vertexFormat attributes written for every vertex
 * @param topology LineList or TriangleList
 * @param sliceCount number of subdivisions of the circumference
 */
ConeShape::ConeShape(PrimitiveShapeFormat vertexFormat, PrimitiveTopology topology,
                     int sliceCount)
    : PrimitiveShape(vertexFormat, topology), m_SliceCount(sliceCount) {
    SetVertexCount(CalculateVertexCount());
    SetIndexCount(CalculateIndexCount());
    SetVertexBufferSize(GetStride() * GetVertexCount());
    SetIndexBufferSize(sizeof(uint32_t) * GetIndexCount());
}

/** @return the number of vertices of the cone */
int ConeShape::CalculateVertexCount() {
    return m_SliceCount * 2 + 2;
}

/** @return the number of indices of the cone */
int ConeShape::CalculateIndexCount() {
    switch (GetPrimitiveTopology()) {
    case PrimitiveTopology_LineList:
        return m_SliceCount * 4;
    case PrimitiveTopology_TriangleList:
        return m_SliceCount * 6;
    default:
        return 0;
    }
}

/**
 * Destroys the cone.
 */
ConeShape::~ConeShape() {}

/**
 * Writes the cone vertices (base ring, base center, side ring, apex) into the vertex buffer.
 * @return the end of the written vertices
 */
void* ConeShape::CalculateVertexBuffer() {
    float* pVertex = static_cast<float*>(GetVertexBuffer());

    for (int i = 0; i < m_SliceCount; i++) {
        nn::util::AngleIndex angle =
            RadianToAngleIndex(DegreeToRadian(360.0f) / m_SliceCount * i);
        float cos = CosTable(angle);
        float sin = SinTable(angle);

        if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
            pVertex[0] = cos;
            pVertex[1] = 0.0f;
            pVertex[2] = sin;
            pVertex += 3;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
            pVertex[0] = 0.0f;
            pVertex[1] = -1.0f;
            pVertex[2] = 0.0f;
            pVertex += 3;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
            pVertex[0] = cos * 0.5f + 0.5f;
            pVertex[1] = 1.0f;
            pVertex += 2;
        }
    }

    if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
        pVertex[0] = 0.0f;
        pVertex[1] = 0.0f;
        pVertex[2] = 0.0f;
        pVertex += 3;
    }

    if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
        pVertex[0] = 0.0f;
        pVertex[1] = -1.0f;
        pVertex[2] = 0.0f;
        pVertex += 3;
    }

    if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
        pVertex[0] = 0.5f;
        pVertex[1] = 1.0f;
        pVertex += 2;
    }

    for (int i = 0; i < m_SliceCount; i++) {
        nn::util::AngleIndex angle =
            RadianToAngleIndex(DegreeToRadian(360.0f) / m_SliceCount * i);
        float cos = CosTable(angle);
        float sin = SinTable(angle);

        if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
            pVertex[0] = cos;
            pVertex[1] = 0.0f;
            pVertex[2] = sin;
            pVertex += 3;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
            pVertex[0] = cos;
            pVertex[1] = 0.0f;
            pVertex[2] = sin;
            pVertex += 3;
        }

        if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
            pVertex[0] = cos * 0.5f + 0.5f;
            pVertex[1] = 1.0f;
            pVertex += 2;
        }
    }

    if (GetVertexFormat() & PrimitiveShapeFormat_Pos) {
        pVertex[0] = 0.0f;
        pVertex[1] = 1.0f;
        pVertex[2] = 0.0f;
        pVertex += 3;
    }

    if (GetVertexFormat() & PrimitiveShapeFormat_Normal) {
        pVertex[0] = 0.0f;
        pVertex[1] = 1.0f;
        pVertex[2] = 0.0f;
        pVertex += 3;
    }

    if (GetVertexFormat() & PrimitiveShapeFormat_Uv) {
        pVertex[0] = 0.5f;
        pVertex[1] = 0.0f;
        pVertex += 2;
    }

    return pVertex;
}

/**
 * Fills the vertex and index buffers of the cone.
 * @param pVertexMemory destination of the vertices
 * @param vertexSize size of the vertex memory
 * @param pIndexMemory destination of the indices
 * @param indexSize size of the index memory
 */
void ConeShape::CalculateImpl(void* pVertexMemory, size_t vertexSize, void* pIndexMemory,
                              size_t indexSize) {
    SetVertexBuffer(pVertexMemory);
    CalculateVertexBuffer();
    SetIndexBuffer(pIndexMemory);

    switch (GetIndexBufferFormat()) {
    case IndexFormat_Uint16:
        CalculateIndexBuffer<uint16_t>();
        break;
    case IndexFormat_Uint32:
        CalculateIndexBuffer<uint32_t>();
        break;
    default:
        NN_UNEXPECTED_DEFAULT;
    }
}

/**
 * Writes the cone indices into the index buffer.
 * @tparam T index type
 */
template <typename T>
void ConeShape::CalculateIndexBuffer() {
    T* pIndexBuffer = static_cast<T*>(GetIndexBuffer());
    PrimitiveTopology topology = GetPrimitiveTopology();
    uint32_t index = 0;

    for (int i = 0; i < m_SliceCount; i++) {
        if (topology == PrimitiveTopology_LineList) {
            pIndexBuffer[i * 2 + 0] = i;
            pIndexBuffer[i * 2 + 1] = (i + 1) % m_SliceCount;
        } else if (topology == PrimitiveTopology_TriangleList) {
            pIndexBuffer[index++] = (i + 1) % m_SliceCount;
            pIndexBuffer[index++] = m_SliceCount;
            pIndexBuffer[index++] = i;
        }
    }

    for (int i = 0; i < m_SliceCount; i++) {
        if (topology == PrimitiveTopology_LineList) {
            pIndexBuffer[m_SliceCount * 2 + i * 2 + 0] = i;
            pIndexBuffer[m_SliceCount * 2 + i * 2 + 1] = m_SliceCount * 2 + 1;
        } else if (topology == PrimitiveTopology_TriangleList) {
            pIndexBuffer[m_SliceCount * 3 + i * 3 + 0] = m_SliceCount + 1 + i;
            pIndexBuffer[m_SliceCount * 3 + i * 3 + 1] = m_SliceCount * 2 + 1;
            pIndexBuffer[m_SliceCount * 3 + i * 3 + 2] =
                (i + 1) % m_SliceCount + m_SliceCount + 1;
        }
    }
}

}  // namespace nn::gfx::util
