//=============================================================================
//
// [OptimizedMarchingCubesMesher.cpp]
// Author : 
//
//=============================================================================
#include "Effects/OptimizedMarchingCubesMesher.h"
#include "Effects/MarchingCubesTables.h"

void OptimizedMarchingCubesMesher::BuildMesh(const SimpleArray<SimpleArray<SimpleArray<float>>>& scalarField, const VolumeReconstructionParams& params)
{
    // 安全チェック（外部バッファ未設定時）
    if (!m_targetVertices || !m_targetIndices)
        return;

    // メッシュの頂点とインデックスをクリア
    m_targetVertices->clear();
    m_targetIndices->clear();

    const int gridX = params.gridX;
    const int gridY = params.gridY;
    const int gridZ = params.gridZ;

    const float cellSize = params.cellSize;
    const XMFLOAT3 origin = params.origin;
    const float isoLevel = params.isoLevel;

    // flat 配列キャッシュの初期化（全エッジに対して -1 で初期化）
    const int totalEdges = gridX * gridY * gridZ * 12;
    m_edgeVertexCache.reserve(totalEdges);
    m_edgeVertexCache.setSize(totalEdges);
    for (int i = 0; i < totalEdges; ++i)
    {
        m_edgeVertexCache[i] = -1;
    }

    for (int x = 0; x < gridX - 1; ++x)
    {
        for (int y = 0; y < gridY - 1; ++y)
        {
            for (int z = 0; z < gridZ - 1; ++z)
            {
                float cube[8] = {
                    scalarField[x][y][z],
                    scalarField[x + 1][y][z],
                    scalarField[x + 1][y][z + 1],
                    scalarField[x][y][z + 1],
                    scalarField[x][y + 1][z],
                    scalarField[x + 1][y + 1][z],
                    scalarField[x + 1][y + 1][z + 1],
                    scalarField[x][y + 1][z + 1],
                };

                int cubeIndex = 0;
                for (int i = 0; i < 8; ++i)
                {
                    if (cube[i] < isoLevel)
                        cubeIndex |= (1 << i);
                }

                if (edgeTable[cubeIndex] == 0)
                    continue;

                XMFLOAT3 corner[8];
                corner[0] = { origin.x + x * cellSize,     origin.y + y * cellSize,     origin.z + z * cellSize };
                corner[1] = { origin.x + (x + 1) * cellSize, origin.y + y * cellSize,     origin.z + z * cellSize };
                corner[2] = { origin.x + (x + 1) * cellSize, origin.y + y * cellSize,     origin.z + (z + 1) * cellSize };
                corner[3] = { origin.x + x * cellSize,     origin.y + y * cellSize,     origin.z + (z + 1) * cellSize };
                corner[4] = { origin.x + x * cellSize,     origin.y + (y + 1) * cellSize, origin.z + z * cellSize };
                corner[5] = { origin.x + (x + 1) * cellSize, origin.y + (y + 1) * cellSize, origin.z + z * cellSize };
                corner[6] = { origin.x + (x + 1) * cellSize, origin.y + (y + 1) * cellSize, origin.z + (z + 1) * cellSize };
                corner[7] = { origin.x + x * cellSize,     origin.y + (y + 1) * cellSize, origin.z + (z + 1) * cellSize };

                XMFLOAT3 vertList[12];

                for (int i = 0; i < 12; ++i)
                {
                    if (!(edgeTable[cubeIndex] & (1 << i))) continue;

                    // エッジキャッシュチェック
                    int cacheIndex = GetEdgeCacheIndex(x, y, z, i, params);
                    int cachedVertex = m_edgeVertexCache[cacheIndex];

                    if (cachedVertex >= 0)
                    {
                        vertList[i] = (*m_targetVertices)[cachedVertex].position;
                    }
                    else
                    {
                        // 補間点計算（エッジに対応する頂点ペアを取得）
                        static const int edgeVertexMap[12][2] = {
                            {0,1},{1,2},{2,3},{3,0},
                            {4,5},{5,6},{6,7},{7,4},
                            {0,4},{1,5},{2,6},{3,7}
                        };
                        int v0 = edgeVertexMap[i][0];
                        int v1 = edgeVertexMap[i][1];

                        XMFLOAT3 pos = InterpolateVertex(corner[v0], corner[v1], cube[v0], cube[v1]);
                        XMFLOAT3 norm = ComputeNormal(scalarField, x, y, z);
                        MarchingCubesVertex vert = { pos, norm };
                        int newIndex = static_cast<int>(m_targetVertices->getSize());
                        m_targetVertices->push_back(vert);
                        m_edgeVertexCache[cacheIndex] = newIndex;

                        vertList[i] = pos;
                    }
                }

                for (int i = 0; triTable[cubeIndex][i] != -1; i += 3)
                {
                    for (int j = 0; j < 3; ++j)
                    {
                        int edgeIdx = triTable[cubeIndex][i + j];
                        int cacheIndex = GetEdgeCacheIndex(x, y, z, edgeIdx, params);
                        int vertIndex = m_edgeVertexCache[cacheIndex];
                        m_targetIndices->push_back(vertIndex);
                    }
                }
            }
        }
    }
}

void OptimizedMarchingCubesMesher::ComputeMeshRequirements(const SimpleArray<SimpleArray<SimpleArray<float>>>& scalarField, const VolumeReconstructionParams& params, UINT* outVertexCount, UINT* outIndexCount)
{
    // 正確な数を事前に求めるのは難しいため、最大値の概算を提供
    const int gridX = params.gridX;
    const int gridY = params.gridY;
    const int gridZ = params.gridZ;

    const int cellCount = (gridX - 1) * (gridY - 1) * (gridZ - 1);

    // 各セル最大15個の三角形、頂点は最大 5 * 3 ≒ 15、インデックスも3つずつ
    const int maxTrianglesPerCell = 5;

    *outVertexCount = cellCount * maxTrianglesPerCell;
    *outIndexCount = cellCount * maxTrianglesPerCell * 3;
}

void OptimizedMarchingCubesMesher::SetTargetBuffer(SimpleArray<MarchingCubesVertex>& vertices, SimpleArray<UINT>& indices)
{
    // 頂点バッファとインデックスバッファを外部から渡して使う
    m_targetVertices = &vertices;
    m_targetIndices = &indices;
}

int OptimizedMarchingCubesMesher::GetEdgeCacheIndex(int x, int y, int z, int edge, const VolumeReconstructionParams& params) const
{
    return (((z * params.gridY + y) * params.gridX + x) * 12) + edge;
}

XMFLOAT3 OptimizedMarchingCubesMesher::InterpolateVertex(const XMFLOAT3& p1, const XMFLOAT3& p2,
    float valp1, float valp2)
{
    if (fabs(valp1 - valp2) < 1e-5f)
        return p1;

    float t = (0.5f - valp1) / (valp2 - valp1); // isoLevel = 0.5 に固定
    return XMFLOAT3(
        p1.x + t * (p2.x - p1.x),
        p1.y + t * (p2.y - p1.y),
        p1.z + t * (p2.z - p1.z)
    );
}

XMFLOAT3 OptimizedMarchingCubesMesher::ComputeNormal(const SimpleArray<SimpleArray<SimpleArray<float>>>& field, int x, int y, int z)
{
    const int maxX = static_cast<int>(field.getSize());
    const int maxY = static_cast<int>(field[0].getSize());
    const int maxZ = static_cast<int>(field[0][0].getSize());

    float dx = 0.0f, dy = 0.0f, dz = 0.0f;

    if (x > 0 && x < maxX - 1)
        dx = (field[x + 1][y][z] - field[x - 1][y][z]) * 0.5f;
    if (y > 0 && y < maxY - 1)
        dy = (field[x][y + 1][z] - field[x][y - 1][z]) * 0.5f;
    if (z > 0 && z < maxZ - 1)
        dz = (field[x][y][z + 1] - field[x][y][z - 1]) * 0.5f;

    XMFLOAT3 normal = { -dx, -dy, -dz }; // 密度の負勾配（外向き）

    XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&normal));
    XMStoreFloat3(&normal, n);
    return normal;
}
