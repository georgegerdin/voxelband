#ifndef voxel_hh__
#define voxel_h__

#include <cassert>
#include <array>
#include <vector>

const int VOXEL_CHUNK_WIDTH = 32;
const int VOXEL_CHUNK_HEIGHT = 32;
const int VOXEL_CHUNK_DEPTH = 32;

const int VOXEL_SLICE_SIZE = VOXEL_CHUNK_WIDTH * VOXEL_CHUNK_HEIGHT;

const int NUM_VOXELS = VOXEL_CHUNK_WIDTH*VOXEL_CHUNK_HEIGHT*VOXEL_CHUNK_DEPTH;

struct PosColorVertex
{
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_abgr;

	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.end();
	};

	static bgfx::VertexDecl ms_decl;
};


enum class VoxelType : unsigned int {
	V_EMPTY = 0,
	V_DIRT,
	V_GRASS
};

class VoxelChunk {
protected:
	std::array<VoxelType, NUM_VOXELS> m_voxel = {};

	bgfx::DynamicVertexBufferHandle m_vbh = BGFX_INVALID_HANDLE;
	bgfx::DynamicIndexBufferHandle m_ibh = BGFX_INVALID_HANDLE;
	bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;

	std::vector<PosColorVertex> m_vertices;
	std::vector<uint16_t> m_indices;

	void update_vertex_buffer();
	void update_index_buffer();

public:
	VoxelChunk();
	VoxelChunk(const VoxelChunk&) = delete;
	VoxelChunk(VoxelChunk&& other) {
		*this = std::move(other);
	}
	~VoxelChunk();

	VoxelChunk& operator=(VoxelChunk const&) = delete;
	VoxelChunk& operator=(VoxelChunk&& other) {
		if (this != &other) {
			m_program = other.m_program;
			m_ibh = other.m_ibh;
			m_vbh = other.m_vbh;
			std::copy(other.m_voxel.begin(), other.m_voxel.end(), m_voxel.begin());
			m_vertices = std::move(other.m_vertices);
			m_indices = std::move(other.m_indices);
			other.m_program = BGFX_INVALID_HANDLE;
			other.m_ibh = BGFX_INVALID_HANDLE;
			other.m_vbh = BGFX_INVALID_HANDLE;
		}
		return *this;
	}


	void set(unsigned int x, unsigned int y, unsigned int z, VoxelType voxel_type) {
		assert(x < VOXEL_CHUNK_WIDTH);
		assert(y < VOXEL_CHUNK_HEIGHT);
		assert(z < VOXEL_CHUNK_DEPTH);

		m_voxel[z*VOXEL_CHUNK_WIDTH*VOXEL_CHUNK_HEIGHT + y*VOXEL_CHUNK_WIDTH + x] = voxel_type;
	}

	VoxelType get(unsigned int x, unsigned int y, unsigned int z) const {
		assert(x < VOXEL_CHUNK_WIDTH);
		assert(y < VOXEL_CHUNK_HEIGHT);
		assert(z < VOXEL_CHUNK_DEPTH);

		return m_voxel[z*VOXEL_CHUNK_WIDTH*VOXEL_CHUNK_HEIGHT + y*VOXEL_CHUNK_WIDTH + x];
	}

	void update_buffers(
		VoxelChunk* left = nullptr,
		VoxelChunk* right = nullptr,
		VoxelChunk* above = nullptr,
		VoxelChunk* below = nullptr,
		VoxelChunk* front = nullptr,
		VoxelChunk* back = nullptr
	);

	bgfx::DynamicVertexBufferHandle const& get_vertex_buffer() const {
		return m_vbh;
	}

	bgfx::DynamicIndexBufferHandle const& get_index_buffer() const {
		return m_ibh;
	}

	bgfx::ProgramHandle const& get_program() const {
		return m_program;
	}

};


#endif
