#include <boost/filesystem.hpp>
#include "bgfx/bgfx.h"
#include "bgfx_utils.h"
#include "voxel.hh"
#include "common.h"
#include "renderer.hh"

namespace fs = boost::filesystem;

enum Material {
	Rock,
	Dirt,
	Grass
};

void Renderer::init(fs::path data_path) {
	auto load_texture_or_throw = [&](auto &dest, auto const& name) {
		auto final_path = data_path / name;

		if (!dest.load(final_path.string().c_str()))
			throw std::runtime_error(std::string("Unable to load ") + "data/fieldstone-rgba.tga");
		
	};

	auto load_shader_or_throw = [&](auto& dest, auto const& vs, auto const& fs) {
		if (!dest.load(vs, fs))
			throw std::runtime_error(std::string("Unable to load ")
				+ vs
				+ std::string(" and ")
				+ fs);
	};

	load_texture_or_throw(m_texture_color,	"fieldstone-rgba.tga");
	load_texture_or_throw(m_texture_normal, "fieldstone-n.tga");
	load_shader_or_throw(m_bump_mapping_shader, "vs_bump", "fs_bump");

	// Create texture sampler uniforms.
	s_texColor.create("s_texColor", bgfx::UniformType::Int1);
	s_texNormal.create("s_texNormal", bgfx::UniformType::Int1);
	m_numLights = 4;
	u_lightPosRadius.create("u_lightPosRadius", bgfx::UniformType::Vec4, m_numLights);
	u_lightRgbInnerR.create("u_lightRgbInnerR", bgfx::UniformType::Vec4, m_numLights);
}

void Renderer::init_frame(float stime) {
	float lightPosRadius[4][4];
	for (uint32_t ii = 0; ii < m_numLights; ++ii)
	{
		lightPosRadius[ii][0] = bx::fsin((stime*(0.1f + ii*0.17f) + ii*bx::kPiHalf*1.37f))*3.0f;
		lightPosRadius[ii][1] = bx::fcos((stime*(0.2f + ii*0.29f) + ii*bx::kPiHalf*1.49f))*3.0f;
		lightPosRadius[ii][2] = -2.5f;
		lightPosRadius[ii][3] = 3.0f;
	}

	bgfx::setUniform(u_lightPosRadius.handle(), lightPosRadius, m_numLights);

	float lightRgbInnerR[4][4] =
	{
		{ 1.0f, 0.7f, 0.2f, 0.8f },
		{ 0.7f, 0.2f, 1.0f, 0.8f },
		{ 0.2f, 1.0f, 0.7f, 0.8f },
		{ 1.0f, 0.4f, 0.2f, 0.8f },
	};

	bgfx::setUniform(u_lightRgbInnerR.handle(), lightRgbInnerR, m_numLights);
}

void Renderer::render(VoxelChunk const& chunk) {
	for (auto const& buffer : chunk.get_buffers()) {
		switch (buffer.first) {
		case VoxelType::V_EMPTY:
			break;
		case VoxelType::V_DIRT:
			render_rock(buffer.second.vertex_buffer, buffer.second.index_buffer);
			break;
		case VoxelType::V_ROCK:
			break;
		case VoxelType::V_GRASS:
			break;
		}
	}
}

void Renderer::render_rock(DynamicVertexBuffer const& vb, DynamicIndexBuffer const& ib) {
	// Bind textures.
	bgfx::setTexture(0, s_texColor.handle(), m_texture_color.handle());
	bgfx::setTexture(1, s_texNormal.handle(), m_texture_normal.handle());

	bgfx::setVertexBuffer(0, vb.handle());
	bgfx::setIndexBuffer(ib.handle());

	// Set render states.
	bgfx::setState(0
		| BGFX_STATE_RGB_WRITE
		| BGFX_STATE_ALPHA_WRITE
		| BGFX_STATE_DEPTH_WRITE
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_MSAA
	);

	// Submit primitive for rendering to view 0.
	bgfx::submit(0, m_bump_mapping_shader.handle());
}