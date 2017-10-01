#include <iostream>

#include "bgfx/bgfx.h"
#include "bgfx_utils.h"
#include "voxel.hh"

static PosColorVertex s_cubeVertices[] =
{
	{ -1.0f,  1.0f,  1.0f, 0xff000000 },
	{ 1.0f,  1.0f,  1.0f, 0xff0000ff },
	{ -1.0f, -1.0f,  1.0f, 0xff00ff00 },
	{ 1.0f, -1.0f,  1.0f, 0xff00ffff },
	{ -1.0f,  1.0f, -1.0f, 0xffff0000 },
	{ 1.0f,  1.0f, -1.0f, 0xffff00ff },
	{ -1.0f, -1.0f, -1.0f, 0xffffff00 },
	{ 1.0f, -1.0f, -1.0f, 0xffffffff },
};

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
	// Create static vertex buffer.
/*	

	// Create static index buffer.
	
*/
	// Create program from shaders
	m_program = loadProgram("vs_cubes", "fs_cubes");
}


VoxelChunk::~VoxelChunk() {
	if (isValid(m_ibh)) {
		bgfx::destroy(m_ibh);
		m_ibh = BGFX_INVALID_HANDLE;
	}

	if (isValid(m_vbh)) {
		bgfx::destroy(m_vbh);
		m_vbh = BGFX_INVALID_HANDLE;
	}

	if (isValid(m_program)) {
		bgfx::destroy(m_program);
		m_program = BGFX_INVALID_HANDLE;
	}
}

bool solid_block(VoxelType block_type) {
	if (block_type == VoxelType::V_EMPTY)
		return false;
	return true;
}

bool solid_block(VoxelType* block_type) {
	return solid_block(*block_type);
}

PosColorVertex make_vertex(unsigned int x, unsigned int y, unsigned int z, uint32_t color) {
	return PosColorVertex{ float(x), float(y), float(z), color };
}

auto add_vertex = [](auto &t, unsigned int x, unsigned int y, unsigned int z, uint32_t color) {
	t.emplace_back(make_vertex( x, y, z, color));
};

auto add_triangle = [] (auto& t, size_t base, unsigned int v1, unsigned int v2, unsigned int v3) {
	t.emplace_back(uint16_t(base + v1));
	t.emplace_back(uint16_t(base + v2));
	t.emplace_back(uint16_t(base + v3));
};

auto add_left_face = [](auto& vertices, auto& indices, auto& index_base, auto x, auto y, auto z) {
	add_vertex(vertices, x,		y + 1,	z + 1,	0xff000000);
	add_vertex(vertices, x,		y + 1,	z,		0xffff0000);
	add_vertex(vertices, x,		y,		z + 1,	0xff00ff00);
	add_vertex(vertices, x,		y,		z,		0xff000000);
	add_triangle(indices, index_base, 0, 2, 1);
	add_triangle(indices, index_base, 1, 2, 3);
	index_base += 4;
};

auto add_right_face = [](auto& vertices, auto& indices, auto& index_base, auto x, auto y, auto z) {
	add_vertex(vertices, x + 1, y + 1,	z + 1,	0xff0000ff);
	add_vertex(vertices, x + 1, y + 1,	z,		0xffff00ff);
	add_vertex(vertices, x + 1, y,		z + 1,	0xff00ffff);
	add_vertex(vertices, x + 1, y,		z,		0xffffffff);
	add_triangle(indices, index_base, 0, 1, 2);
	add_triangle(indices, index_base, 1, 3, 2);
	index_base += 4;
};

auto add_bottom_face = [](auto& vertices, auto& indices, auto& index_base, auto x, auto y, auto z) {
	add_vertex(vertices, x,		y,		z + 1,	0xff00ff00);
	add_vertex(vertices, x + 1, y,		z + 1,	0xff00ffff);
	add_vertex(vertices, x,		y,		z,		0xffffff00);
	add_vertex(vertices, x + 1, y,		z,		0xffffffff);
	add_triangle(indices, index_base, 0, 1, 2);
	add_triangle(indices, index_base, 2, 1, 3);
	index_base += 4;
};

auto add_front_face = [](auto& vertices, auto& indices, auto& index_base, auto x, auto y, auto z) {
	add_vertex(vertices, x,		y + 1,	z,		0xffff0000);
	add_vertex(vertices, x,		y,		z,		0xffffff00);
	add_vertex(vertices, x + 1, y + 1,	z,		0xffff00ff);
	add_vertex(vertices, x + 1, y,		z,		0xffffffff);
	add_triangle(indices, index_base, 0, 1, 2);
	add_triangle(indices, index_base, 2, 1, 3);
	index_base += 4;
};

auto add_back_face = [](auto& vertices, auto& indices, auto& index_base, auto x, auto y, auto z) {
	add_vertex(vertices, x,		y + 1,	z + 1,	0xff000000);
	add_vertex(vertices, x + 1, y + 1,	z + 1,	0xff0000ff);
	add_vertex(vertices, x,		y,		z + 1,	0xff00ff00);
	add_vertex(vertices, x + 1, y,		z + 1,	0xff00ffff);
	add_triangle(indices, index_base, 0, 1, 2);
	add_triangle(indices, index_base, 3, 2, 1);
	index_base += 4;
};

auto add_top_face = [](auto& vertices, auto& indices, auto& index_base, auto x, auto y, auto z) {
	add_vertex(vertices, x,		y + 1,	z,		0xffff0000);
	add_vertex(vertices, x + 1, y + 1,	z,		0xffff00ff);
	add_vertex(vertices, x + 1, y + 1,	z + 1,	0xff0000ff);
	add_vertex(vertices, x,		y + 1,	z + 1,	0xff000000);
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

	m_vertices.clear();
	m_indices.clear();

	unsigned int x = 0, y = 0, z = 0;
	
	for (VoxelType* current_voxel = m_voxel.data(); 
		 z < VOXEL_CHUNK_DEPTH;
		 current_voxel++) {

		if ( solid_block(*current_voxel) ) {
			auto index_base = m_vertices.size();
			//left face
			if (x == 0 ) {
				if (left) {
					if (!solid_block(left->get(VOXEL_CHUNK_WIDTH - 1, y, z)))
						add_left_face(m_vertices, m_indices, index_base, x, y, z);
				}
			}
			else if(!solid_block(current_voxel - 1)) {
				add_left_face(m_vertices, m_indices, index_base, x, y, z);
			}
			//right face
			if (x == VOXEL_CHUNK_WIDTH - 1) {
				if (right) {
					if (!solid_block(right->get(0, y, z)))
						add_right_face(m_vertices, m_indices, index_base, x, y, z);
				}
			}
			else if(!solid_block(current_voxel + 1)) {
				add_right_face(m_vertices, m_indices, index_base, x, y, z);
			}
			//top face
			if (y == VOXEL_CHUNK_HEIGHT - 1) {
				if (above) {
					if (!solid_block(above->get(x, 0, z)))
						add_top_face(m_vertices, m_indices, index_base, x, y, z);
				}
			}
			else if(!solid_block(current_voxel + VOXEL_CHUNK_WIDTH)) {
				add_top_face(m_vertices, m_indices, index_base, x, y, z);
			}
			//bottom face
			if (y == 0) {
				if (below) {
					if (!solid_block(below->get(x, VOXEL_CHUNK_HEIGHT - 1, z)))
						add_bottom_face(m_vertices, m_indices, index_base, x, y, z);
				}
			}
			else if(!solid_block(current_voxel - VOXEL_CHUNK_HEIGHT)) {
				add_bottom_face(m_vertices, m_indices, index_base, x, y, z);
			}
			//front face
			if (z == 0) {
				if (front) {
					if (!solid_block(front->get(x, y, VOXEL_CHUNK_DEPTH - 1)))
						add_front_face(m_vertices, m_indices, index_base, x, y, z);
				}
			}
			else if(!solid_block(current_voxel - VOXEL_SLICE_SIZE)) {
				add_front_face(m_vertices, m_indices, index_base, x, y, z);
			}
			//back face
			if (z == VOXEL_CHUNK_DEPTH - 1) {
				if (back) {
					if (!solid_block(back->get(x, y, 0)))
						add_back_face(m_vertices, m_indices, index_base, x, y, z);
				}
			}
			else if(!solid_block(current_voxel + VOXEL_SLICE_SIZE)) {	
				add_back_face(m_vertices, m_indices, index_base, x, y, z);
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

	update_vertex_buffer();
	update_index_buffer();
}


void VoxelChunk::update_vertex_buffer() {
	if (isValid(m_vbh)) {
		bgfx::updateDynamicVertexBuffer(
			m_vbh,
			0,
			bgfx::makeRef(m_vertices.data(), m_vertices.size() * sizeof(PosColorVertex))
		);
	}
	else {
		m_vbh = bgfx::createDynamicVertexBuffer(
			bgfx::makeRef(m_vertices.data(), m_vertices.size()*sizeof(PosColorVertex))
			, PosColorVertex::ms_decl
			, BGFX_BUFFER_ALLOW_RESIZE
		);
	}
}

void VoxelChunk::update_index_buffer() {
	if (isValid(m_ibh)) {
		bgfx::updateDynamicIndexBuffer(m_ibh,
			0,
			bgfx::makeRef(m_indices.data(), m_indices.size() * sizeof(uint16_t))
		);
	}
	else {
		m_ibh = bgfx::createDynamicIndexBuffer(
			bgfx::makeRef(m_indices.data(), m_indices.size() * sizeof(uint16_t))
			, BGFX_BUFFER_ALLOW_RESIZE
		);
	}
}
