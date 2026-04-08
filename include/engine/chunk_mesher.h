#ifndef CHUNK_MESHER_H
#define CHUNK_MESHER_H

class Chunk;
class ChunkManager;
class ChunkMeshRegistry;

class ChunkMesher {
public:
    void updateChunk(Chunk& p_chunk, ChunkManager& p_chunk_manager, ChunkMeshRegistry& p_chunk_mesh_registry);
    void updateAll(ChunkManager& p_chunk_manager, ChunkMeshRegistry& p_chunk_mesh_registry);
};

#endif
