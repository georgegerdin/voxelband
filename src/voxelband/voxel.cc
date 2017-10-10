#include <iostream>
#include <variant>
#include <boost/filesystem.hpp>

#include "bgfx/bgfx.h"
#include "bgfx_utils.h"
#include "voxel.hh"

static const uint16_t s_cubeTriList[] =
{
	0, 1, 2, // back
	1, 3, 2,
	4, 6, 5, // front
	5, 6, 7,
	0, 2, 4, // left
	4, 2, 6,
	1, 5, 3, // right
	5, 7, 3,
	0, 4, 1, // top
	4, 5, 1,
	2, 3, 6, // bottom
	6, 3, 7,
};


static const uint16_t s_cubeTriStrip[] =
{
	0, 1, 2,
	3,
	7,
	1,
	5,
	0,
	4,
	2,
	6,
	7,
	4,
	5,
};

VoxelChunk::VoxelChunk()
{
}


VoxelChunk::~VoxelChunk() {
}

bool solid_block(VoxelType block_type) {
	if (block_type == VoxelType::V_EMPTY)
		return false;
	return true;
}

bool solid_block(VoxelType* block_type) {
	return solid_block(*block_type);
}

struct Normal { float x, y, z; Normal(float ix, float iy, float iz) : x(ix), y(iy), z(iz) { } };
struct FrontNormal : Normal {	FrontNormal()	: Normal(0.0f,	0.0f,	-1.0f)	{} };
struct BackNormal : Normal {	BackNormal()	: Normal(0.0f,	0.0f,	 1.0f)	{} };
struct LeftNormal : Normal {	LeftNormal()	: Normal(-1.0f, 0.0f,	 0.0f)  {} };
struct RightNormal : Normal {	RightNormal()	: Normal(1.0f,	0.0f,	 0.0f)	{} };
struct UpNormal : Normal {		UpNormal()		: Normal(0.0f,	1.0f,	 0.0f)  {} };
struct DownNormal : Normal {	DownNormal()	: Normal(0.0f, -1.0f,	 0.0f)  {} };

using NormalDirection = std::variant<
	FrontNormal,
	BackNormal,
	LeftNormal,
	RightNormal,
	UpNormal,
	DownNormal
>;

uint32_t get_normal(NormalDirection direction) {
	return std::visit([](Normal const& n) {return encodeNormalRgba8(n.x, n.y, n.z); }, direction);
}

PosNormalTangentTexcoordVertex make_vertex(
	unsigned int x, unsigned int y, unsigned int z, 
	NormalDirection normal,
	int16_t u, int16_t v) {
	return PosNormalTangentTexcoordVertex{ float(x), float(y), float(z), get_normal(normal), 0, u, v };
}

auto add_vertex = [](auto &t, unsigned int x, unsigned int y, unsigned int z, NormalDirection normal, int16_t u, int16_t v) {
	t.emplace_back(make_vertex( x, y, z, normal, u, v));
};

auto add_triangle = [] (auto& t, size_t base, unsigned int v1, unsigned int v2, unsigned int v3) {
	t.emplace_back(uint16_t(base + v1));
	t.emplace_back(uint16_t(base + v2));
	t.emplace_back(uint16_t(base + v3));
};

auto add_left_face = [](auto& vertices, auto& indices, auto& index_base, auto x, auto y, auto z) {
	add_vertex(vertices, x,		y + 1,	z + 1,	LeftNormal{},	0x7fff,		 0);
	add_vertex(vertices, x,		y + 1,	z,		LeftNormal{},	0x7fff,	0x7fff);
	add_vertex(vertices, x,		y,		z + 1,	LeftNormal{},	     0,		 0);
	add_vertex(vertices, x,		y,		z,		LeftNormal{},		 0,	0x7fff);
	add_triangle(indices, index_base, 0, 2, 1);
	add_triangle(indices, index_base, 1, 2, 3);
	index_base += 4;
};

auto add_right_face = [](auto& vertices, auto& indices, auto& index_base, auto x, auto y, auto z) {
	add_vertex(vertices, x + 1, y + 1,	z + 1,	RightNormal{},	0x7fff,	     0);
	add_vertex(vertices, x + 1, y + 1,	z,		RightNormal{},	0x7fff,	0x7fff);
	add_vertex(vertices, x + 1, y,		z + 1,	RightNormal{},	     0,	     0);
	add_vertex(vertices, x + 1, y,		z,		RightNormal{},	     0,	0x7fff);
	add_triangle(indices, index_base, 0, 1, 2);
	add_triangle(indices, index_base, 1, 3, 2);
	index_base += 4;
};

auto add_bottom_face = [](auto& vertices, auto& indices, auto& index_base, auto x, auto y, auto z) {
	auto n = indices.size();
	add_vertex(vertices, x,		y,		z + 1,	DownNormal{},	     0,	     0);
	add_vertex(vertices, x + 1, y,		z + 1,	DownNormal{},	0x7fff,	     0);
	add_vertex(vertices, x,		y,		z,		DownNormal{},	     0,	0x7fff);
	add_vertex(vertices, x + 1, y,		z,		DownNormal{},	0x7fff,	0x7fff);
	add_triangle(indices, index_base, 0, 1, 2);
	add_triangle(indices, index_base, 2, 1, 3);
	index_base += 4;
};

auto add_front_face = [](auto& vertices, auto& indices, auto& index_base, auto x, auto y, auto z) {
	auto n = indices.size();
	add_vertex(vertices,     x,	1 + y,	z,	FrontNormal{},	     0,	     0);
	add_vertex(vertices,     x,	    y,	z,	FrontNormal{},	     0,	0x7fff);
	add_vertex(vertices, 1 + x, 1 + y,	z,	FrontNormal{},	0x7fff,		 0);
	add_vertex(vertices, 1 + x,     y,	z,	FrontNormal{},	0x7fff,	0x7fff);
	add_triangle(indices, index_base, 0, 1, 2);
	add_triangle(indices, index_base, 2, 1, 3);
	index_base += 4;
};

auto add_back_face = [](auto& vertices, auto& indices, auto& index_base, auto x, auto y, auto z) {
	auto n = indices.size();
	add_vertex(vertices,     x,	y + 1,	z + 1,	BackNormal{},	     0,	     0);
	add_vertex(vertices, x + 1, y + 1,	z + 1,	BackNormal{},	0x7fff,	     0);
	add_vertex(vertices,     x,	    y,	z + 1,	BackNormal{},	     0,	0x7fff);
	add_vertex(vertices, x + 1,     y,	z + 1,	BackNormal{},	0x7fff,	0x7fff);
	add_triangle(indices, index_base, 0, 1, 2);
	add_triangle(indices, index_base, 3, 2, 1);
	index_base += 4;
};

auto add_top_face = [](auto& vertices, auto& indices, auto& index_base, auto x, auto y, auto z) {
	auto n = indices.size();
	add_vertex(vertices, x,		y + 1,	z,		UpNormal{},	     0,	0x7fff);
	add_vertex(vertices, x + 1, y + 1,	z,		UpNormal{},	0x7fff,	0x7fff);
	add_vertex(vertices, x + 1, y + 1,	z + 1,	UpNormal{},	0x7fff,	     0);
	add_vertex(vertices, x,		y + 1,	z + 1,	UpNormal{},	     0,	     0);
	add_triangle(indices, index_base, 0, 1, 2);
	add_triangle(indices, index_base, 3, 0, 2);
	index_base += 4;
};

void VoxelChunk::update_buffers(
	VoxelChunk* left,
	VoxelChunk* right,
	VoxelChunk* above,
	VoxelChunk* below,
	VoxelChunk* front,
	VoxelChunk* back) {

	for (auto& buffer : m_buffers) {
		buffer.second.vertices.clear();
		buffer.second.indices.clear();
	}

	unsigned int x = 0, y = 0, z = 0;
	
	for (VoxelType* current_voxel = m_voxel.data(); 
		 z < VOXEL_CHUNK_DEPTH;
		 current_voxel++) {

		if ( solid_block(*current_voxel) ) {
			auto& voxel_buffer = get_buffer_or(*current_voxel, []() {return VoxelBuffer{}; });
			auto& vertices = voxel_buffer.vertices;
			auto& indices = voxel_buffer.indices;

			auto index_base = vertices.size();
			//left face
			if (x == 0 ) {
				if (left) {
					if (!solid_block(left->get(VOXEL_CHUNK_WIDTH - 1, y, z)))
						add_left_face(vertices, indices, index_base, x, y, z);
				}
			}
			else if(!solid_block(current_voxel - 1)) {
				add_left_face(vertices, indices, index_base, x, y, z);
			}
			//right face
			if (x == VOXEL_CHUNK_WIDTH - 1) {
				if (right) {
					if (!solid_block(right->get(0, y, z)))
						add_right_face(vertices, indices, index_base, x, y, z);
				}
			}
			else if(!solid_block(current_voxel + 1)) {
				add_right_face(vertices, indices, index_base, x, y, z);
			}
			//top face
			if (y == VOXEL_CHUNK_HEIGHT - 1) {
				if (above) {
					if (!solid_block(above->get(x, 0, z)))
						add_top_face(vertices, indices, index_base, x, y, z);
				}
			}
			else if(!solid_block(current_voxel + VOXEL_CHUNK_WIDTH)) {
				add_top_face(vertices, indices, index_base, x, y, z);
			}
			//bottom face
			if (y == 0) {
				if (below) {
					if (!solid_block(below->get(x, VOXEL_CHUNK_HEIGHT - 1, z)))
						add_bottom_face(vertices, indices, index_base, x, y, z);
				}
			}
			else if(!solid_block(current_voxel - VOXEL_CHUNK_HEIGHT)) {
				add_bottom_face(vertices, indices, index_base, x, y, z);
			}
			//front face
			if (z == 0) {
				if (front) {
					if (!solid_block(front->get(x, y, VOXEL_CHUNK_DEPTH - 1)))
						add_front_face(vertices, indices, index_base, x, y, z);
				}
			}
			else if(!solid_block(current_voxel - VOXEL_SLICE_SIZE)) {
				add_front_face(vertices, indices, index_base, x, y, z);
			}
			//back face
			if (z == VOXEL_CHUNK_DEPTH - 1) {
				if (back) {
					if (!solid_block(back->get(x, y, 0)))
						add_back_face(vertices, indices, index_base, x, y, z);
				}
			}
			else if(!solid_block(current_voxel + VOXEL_SLICE_SIZE)) {	
				add_back_face(vertices, indices, index_base, x, y, z);
			}
		}
		
		++x;
		if (x >= VOXEL_CHUNK_WIDTH) {
			x = 0;
			++y;
			if (y >= VOXEL_CHUNK_HEIGHT) {
				y = 0;
				++z;
			}
		}
	}

	for (auto& buffer : m_buffers) {
		auto& vertices = buffer.second.vertices;
		if (vertices.size() == 0)
			continue;
		auto& indices = buffer.second.indices;
		calcTangents(&vertices[0]
			, vertices.size()
			, PosNormalTangentTexcoordVertex::ms_decl
			, &indices[0]
			, indices.size()
		);

		update_vertex_buffer(buffer.second);
		update_index_buffer(buffer.second);
	}

	
}


void VoxelChunk::update_vertex_buffer(VoxelBuffer& vb) {
	if (vb.vertex_buffer.is_valid()) {
		vb.vertex_buffer.update(
			0,
			bgfx::makeRef(vb.vertices.data()
				, vb.vertices.size() * sizeof(PosNormalTangentTexcoordVertex))
		);
	}
	else {
		if (!vb.vertex_buffer.create(
			bgfx::makeRef(vb.vertices.data(), vb.vertices.size() * sizeof(PosNormalTangentTexcoordVertex))
			, PosNormalTangentTexcoordVertex::ms_decl
			, BGFX_BUFFER_ALLOW_RESIZE
		))
			throw std::runtime_error("Unable to create vertex buffer.");
	}
}

void VoxelChunk::update_index_buffer(VoxelBuffer& vb) {
	if (vb.index_buffer.is_valid()) {
		vb.index_buffer.update(0,
			bgfx::makeRef(vb.indices.data(), vb.indices.size() * sizeof(uint16_t))
		);
	}
	else {
		vb.index_buffer.create(
			bgfx::makeRef(vb.indices.data(), vb.indices.size() * sizeof(uint16_t))
			, BGFX_BUFFER_ALLOW_RESIZE
		);
	}
}
