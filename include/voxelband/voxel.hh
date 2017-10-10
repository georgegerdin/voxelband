#ifndef voxel_hh__
#define voxel_hh__

#include <cassert>
#include <array>
#include <vector>
#include <map>

#include "renderer.hh"

const int VOXEL_CHUNK_WIDTH = 32;
const int VOXEL_CHUNK_HEIGHT = 32;
const int VOXEL_CHUNK_DEPTH = 32;

const int VOXEL_SLICE_SIZE = VOXEL_CHUNK_WIDTH * VOXEL_CHUNK_HEIGHT;

const int NUM_VOXELS = VOXEL_CHUNK_WIDTH*VOXEL_CHUNK_HEIGHT*VOXEL_CHUNK_DEPTH;

struct PosNormalTangentTexcoordVertex
{
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_normal;
	uint32_t m_tangent;
	int16_t m_u;
	int16_t m_v;

	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal, 4, bgfx::AttribType::Uint8, true, true)
			.add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Uint8, true, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Int16, true, true)
			.end();
	}

	static bgfx::VertexDecl ms_decl;
};


enum class VoxelType : unsigned int {
	V_EMPTY = 0,
	V_DIRT,
	V_ROCK,
	V_GRASS
};

class VoxelChunk {
protected:
	std::array<VoxelType, NUM_VOXELS> m_voxel = {};
	struct VoxelBuffer {
		VoxelBuffer(const VoxelBuffer&) = delete;
		VoxelBuffer(VoxelBuffer&&) = default;
		DynamicVertexBuffer vertex_buffer;
		DynamicIndexBuffer index_buffer;
		std::vector<PosNormalTangentTexcoordVertex> vertices;
		std::vector<uint16_t> indices;
	};
	using BufferContainer = std::map<VoxelType, VoxelBuffer>;
	BufferContainer m_buffers;
	bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;

	void update_vertex_buffer(VoxelBuffer&);
	void update_index_buffer(VoxelBuffer&);

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
			std::copy(other.m_voxel.begin(), other.m_voxel.end(), m_voxel.begin());
			m_buffers = std::move(other.m_buffers);
			other.m_program = BGFX_INVALID_HANDLE;
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

	BufferContainer const& get_buffers() const {
		return m_buffers;
	}

	template<typename Tfun>
	VoxelBuffer& get_buffer_or(VoxelType type, Tfun fun) {
		auto find_buffer = m_buffers.find(type);
		if (find_buffer == m_buffers.end()) {
			auto [buffer, b] = m_buffers.emplace(std::make_pair(type, fun()));
			if (!b)
				throw std::runtime_error("Unable to emplace voxel buffer");
			return buffer->second;
		}
		return find_buffer->second;
	}

	VoxelBuffer const& get_buffer(VoxelType type) const {
		auto find_buffer = m_buffers.find(type);
		if (find_buffer == m_buffers.end()) 
			throw std::runtime_error("Unable to find buffer");
		return find_buffer->second;
	}

	bgfx::ProgramHandle const& get_program() const {
		return m_program;
	}

};


#endif // !voxel_hh__
