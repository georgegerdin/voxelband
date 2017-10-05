/*
* Copyright 2011-2017 Branimir Karadzic. All rights reserved.
* License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
*/
#include <sstream>
#include <boost/hana/functional/overload.hpp>
#include <boost/filesystem.hpp>
#include <variant>
#include <vector>
#include <string>
#include <algorithm>
#include <optional>
#include <iostream>
#include <direct.h>
#include "namegen.h"
#include <nlohmann/json.hpp>

#include <bx/uint32_t.h>
#include <bgfx/bgfx.h>
#include "debugdraw/debugdraw.h"
#include "common.h"
#include "bgfx_utils.h"
#include "logo.h"
#include "imgui/imgui.h"
#include "birth.hh"
#include "camera.h"
#include "voxel.hh"

bgfx::VertexDecl PosNormalTangentTexcoordVertex::ms_decl;
namespace
{
	namespace fs = boost::filesystem;

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

	struct Menu {
		struct MainMenu {};
		struct Credits {};
		struct Options {};

		using ActiveMenu = std::variant<
			MainMenu,
			Credits/*,
				   Options*/
		>;

		ActiveMenu active = MainMenu{};

		Menu() {  }
	};

	struct Playing {};

	using GameState = std::variant<Menu, Birth, Playing>;

	GameState game_state;
	fs::path data_path;


	using std::to_string;

	std::string to_string(const char* s) {
		return std::string(s);
	}

	template<typename T>
	void UniqueButton(T text, const char* id) {
		std::string str = to_string(text);
		str += "##";
		str += id;
		ImGui::Button(str.c_str());
	}

	bool draw_ui(Menu& menu) {
		bool running = true;

		std::visit(boost::hana::overload(
			[&](Menu::MainMenu& m) {
			ImGui::Begin("Main Menu");
			ImGui::Button("Resume Quest");
			if (ImGui::Button("New Character"))
				game_state = Birth{};
			if (ImGui::Button("Quit"))
				running = false;
			if (ImGui::Button("Credits"))
				menu.active = Menu::Credits{};
			ImGui::End();
		},
		[&](Menu::Credits&) {ImGui::ShowTestWindow(); }

		), menu.active);

		return running;
	}

	bool draw_ui(Birth& birth) {
		if (!birth.frame())
			game_state = Menu{};
		return true;
	}

	bool draw_ui(Playing& menu) {
		return true;
	}

	bool ui_frame() {
		auto draw_ui_ = [](auto& state) {
			return draw_ui(state);
		};

		return std::visit(draw_ui_, game_state);
	}

	bgfx::TextureHandle load_texture(fs::path path) {
		return loadTexture(path.string().c_str());
	}


	class Voxelband : public entry::AppI
	{
	public:
		Voxelband(const char* _name, const char* _description)
			: entry::AppI(_name, _description)
		{
		}

		void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
		{
			Args args(_argc, _argv);

			m_width = _width;
			m_height = _height;
			m_debug = BGFX_DEBUG_TEXT;
			m_reset = BGFX_RESET_VSYNC;

			bgfx::init(args.m_type, args.m_pciId);
			bgfx::reset(m_width, m_height, m_reset);

			// Enable debug text.
			bgfx::setDebug(m_debug);

			// Set view 0 clear state.
			bgfx::setViewClear(0
				, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0
			);

			m_timeOffset = bx::getHPCounter();

			cameraCreate();

			const float initialPos[3] = { 0.0f, 2.0f, -12.0f };
			cameraSetPosition(initialPos);
			cameraSetVerticalAngle(0.0f);

			imguiCreate();

			// Create vertex stream declaration.
			PosNormalTangentTexcoordVertex::init();

			ddInit();

			std::ifstream cfg_txt("config.txt");
			if (cfg_txt.is_open()) {
				/*
				std::stringstream buffer;
				buffer << cfg_txt.rdbuf();
				auto cfg_json = json::JSON::Load(buffer.str());
				auto path = cfg_json["general"]["base-dir"].ToString();
				*/
				
				nlohmann::json j;
				cfg_txt >> j;
				auto general = j.find("general");
				if (general != j.end()) {
					auto base_dir = general->find("base-dir");
					if (base_dir != general->end()) {
						std::string path = *base_dir;
						if (fs::exists(path))
							data_path = path;
					}
				}

			}

			// Load diffuse texture.
			m_textureColor = load_texture(data_path / "data/fieldstone-rgba.tga");

			// Load normal texture.
			m_textureNormal = load_texture(data_path / "data/fieldstone-n.tga");

			// Create texture sampler uniforms.
			s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Int1);
			s_texNormal = bgfx::createUniform("s_texNormal", bgfx::UniformType::Int1);

			m_numLights = 4;
			u_lightPosRadius = bgfx::createUniform("u_lightPosRadius", bgfx::UniformType::Vec4, m_numLights);
			u_lightRgbInnerR = bgfx::createUniform("u_lightRgbInnerR", bgfx::UniformType::Vec4, m_numLights);



			auto chunk = VoxelChunk{};
			chunk.set(0, 0, 0, VoxelType::V_DIRT);
			chunk.set(1, 1, 0, VoxelType::V_DIRT);
			chunk.set(1, 1, 1, VoxelType::V_DIRT);
			chunk.set(1, 1, 2, VoxelType::V_DIRT);
			chunk.set(1, 2, 2, VoxelType::V_DIRT);
			chunk.set(5, 6, 7, VoxelType::V_DIRT);
			chunk.set(31, 0, 0, VoxelType::V_DIRT);
			chunk.set(0, 31, 0, VoxelType::V_DIRT);
			chunk.set(31, 31, 31, VoxelType::V_DIRT);

			for (auto x : { 8,9,10 }) 
				for (auto y : { 8,9,10 }) 
					for (auto z : { 8, 9, 10 }) 
						chunk.set(x, y, z, VoxelType::V_DIRT);
			
			create_voxel_chunk(-1, 0, 0);
			create_voxel_chunk(1, 0, 0);
			create_voxel_chunk(0, -1, 0);
			create_voxel_chunk(0, 0, -1);
			create_voxel_chunk(0, 0, 1);
			create_voxel_chunk(0, 1, 0);

			chunk.update_buffers(
				get_voxel_chunk(-1, 0, 0),
				get_voxel_chunk(1, 0, 0),
				get_voxel_chunk(0, 1, 0),
				get_voxel_chunk(0, -1, 0),
				get_voxel_chunk(0, 0, -1),
				get_voxel_chunk(0, 0, 1)
			);
			set_voxel_chunk(0, 0, 0, std::move(chunk));
		}

		virtual int shutdown() override
		{
			bgfx::destroy(m_textureColor);
			bgfx::destroy(m_textureNormal);
			bgfx::destroy(s_texColor);
			bgfx::destroy(s_texNormal);
			bgfx::destroy(u_lightPosRadius);
			bgfx::destroy(u_lightRgbInnerR);

			ddShutdown();

			imguiDestroy();

			cameraDestroy();

			m_voxel_world.clear();
			
			// Shutdown bgfx.
			bgfx::shutdown();

			return 0;
		}

		bool update() override
		{
			char inputChar = '\000';
			if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState, &inputChar))
			{
				imguiBeginFrame(m_mouseState.m_mx
					, m_mouseState.m_my
					, (m_mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0)
					| (m_mouseState.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0)
					| (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
					, m_mouseState.m_mz
					, uint16_t(m_width)
					, uint16_t(m_height)
					, inputChar
				);

				//showExampleDialog(this);
				bool running = ui_frame();

				imguiEndFrame();

				int64_t now = bx::getHPCounter() - m_timeOffset;
				static int64_t last = now;
				const int64_t frameTime = now - last;
				last = now;
				const double freq = double(bx::getHPFrequency());
				const float deltaTime = float(frameTime / freq);
				const float stime = (float)(now / freq);

				// Update camera.
				cameraUpdate(deltaTime, m_mouseState);

				float view[16];
				cameraGetViewMtx(view);

				float proj[16];

				// Set view and projection matrix for view 0.
				const bgfx::HMD* hmd = bgfx::getHMD();
				if (NULL != hmd && 0 != (hmd->flags & BGFX_HMD_RENDERING))
				{
					float eye[3];
					cameraGetPosition(eye);
					bx::mtxQuatTranslationHMD(view, hmd->eye[0].rotation, eye);
					bgfx::setViewTransform(0, view, hmd->eye[0].projection, BGFX_VIEW_STEREO, hmd->eye[1].projection);
					bgfx::setViewRect(0, 0, 0, hmd->width, hmd->height);
				}
				else
				{
					bx::mtxProj(proj, 60.0f, float(m_width) / float(m_height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

					bgfx::setViewTransform(0, view, proj);
					bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));
				}

				// Set view 0 default viewport.
				bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));

				// This dummy draw call is here to make sure that view 0 is cleared
				// if no other draw calls are submitted to view 0.
				bgfx::touch(0);

				float lightPosRadius[4][4];
				for (uint32_t ii = 0; ii < m_numLights; ++ii)
				{
					lightPosRadius[ii][0] = bx::fsin((stime*(0.1f + ii*0.17f) + ii*bx::kPiHalf*1.37f))*3.0f;
					lightPosRadius[ii][1] = bx::fcos((stime*(0.2f + ii*0.29f) + ii*bx::kPiHalf*1.49f))*3.0f;
					lightPosRadius[ii][2] = -2.5f;
					lightPosRadius[ii][3] = 3.0f;
				}

				bgfx::setUniform(u_lightPosRadius, lightPosRadius, m_numLights);

				float lightRgbInnerR[4][4] =
				{
					{ 1.0f, 0.7f, 0.2f, 0.8f },
					{ 0.7f, 0.2f, 1.0f, 0.8f },
					{ 0.2f, 1.0f, 0.7f, 0.8f },
					{ 1.0f, 0.4f, 0.2f, 0.8f },
				};

				bgfx::setUniform(u_lightRgbInnerR, lightRgbInnerR, m_numLights);



				float mtx[16];
				bx::mtxIdentity(mtx);

				// Set model matrix for rendering.
				bgfx::setTransform(mtx);

				// Set vertex and index buffer.
				auto* chunk = get_voxel_chunk(0, 0, 0);
				if (chunk) {
					bgfx::setVertexBuffer(0, chunk->get_vertex_buffer());
					bgfx::setIndexBuffer(chunk->get_index_buffer());

					// Bind textures.
					bgfx::setTexture(0, s_texColor, m_textureColor);
					bgfx::setTexture(1, s_texNormal, m_textureNormal);

					// Set render states.
					bgfx::setState(0
						| BGFX_STATE_RGB_WRITE
						| BGFX_STATE_ALPHA_WRITE
						| BGFX_STATE_DEPTH_WRITE
						| BGFX_STATE_DEPTH_TEST_LESS
						| BGFX_STATE_MSAA
					);

					// Submit primitive for rendering to view 0.
					bgfx::submit(0, chunk->get_program());
				}

				// Use debug font to print information about this example.
				bgfx::dbgTextClear();
				/*bgfx::dbgTextImage(
					bx::uint16_max(uint16_t(m_width / 2 / 8), 20) - 20
					, bx::uint16_max(uint16_t(m_height / 2 / 16), 6) - 6
					, 40
					, 12
					, s_logo
					, 160
				);*/
				bgfx::dbgTextPrintf(0, 1, 0x0f, "Color can be changed with ANSI \x1b[9;me\x1b[10;ms\x1b[11;mc\x1b[12;ma\x1b[13;mp\x1b[14;me\x1b[0m code too.");

				const bgfx::Stats* stats = bgfx::getStats();
				bgfx::dbgTextPrintf(0, 2, 0x0f, "Backbuffer %dW x %dH in pixels, debug text %dW x %dH in characters."
					, stats->width
					, stats->height
					, stats->textWidth
					, stats->textHeight
				);

				ddBegin(0);
				ddDrawAxis(0.0f, 0.0f, 0.0f);
				float center[3] = { 0.0f, 0.0f, 0.0f };

				ddDrawGrid(Axis::Y, center, 20, 1.0f);
				ddEnd();
				// Advance to next frame. Rendering thread will be kicked to
				// process submitted rendering primitives.
				bgfx::frame();

				return running;
			}

			return false;
		}

		entry::MouseState m_mouseState;
		int64_t m_timeOffset;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_debug;
		uint32_t m_reset;
		bgfx::UniformHandle s_texColor;
		bgfx::UniformHandle s_texNormal;
		bgfx::UniformHandle u_lightPosRadius;
		bgfx::UniformHandle u_lightRgbInnerR;
		uint16_t m_numLights;

		bgfx::TextureHandle m_textureColor, m_textureNormal;

		using VoxelLine = std::map<int, VoxelChunk>;
		VoxelLine hello;
		using VoxelSlice = std::map<int, VoxelLine>;
		VoxelSlice p;
		using VoxelWorld = std::map<int, VoxelSlice>;

		VoxelWorld m_voxel_world;

		VoxelSlice* get_voxel_slice(int z) {
			auto slice = m_voxel_world.find(z);
			if (slice == m_voxel_world.end()) {
				return nullptr;
			}
			return &slice->second;
		}

		template<typename T>
		VoxelSlice& get_voxel_slice_or(int z, T fn) {
			auto slice = m_voxel_world.find(z);
			if (slice == m_voxel_world.end()) {
				auto[slice, b] = m_voxel_world.emplace(std::make_pair(z, fn()));
				if (!b) {
					throw std::runtime_error("Unable to create voxel slice.");
				}
				return slice->second;
			}
			return slice->second;
		}

		VoxelLine* get_voxel_line(VoxelSlice& slice, int y) {
			auto line = slice.find(y);
			if (line == slice.end()) {
				return nullptr;
			}
			return &line->second;
		}

		template<typename T>
		VoxelLine& get_voxel_line_or(VoxelSlice& slice, int y, T fn) {
			auto line = slice.find(y);
			if (line == slice.end()) {
				auto[line, b] = slice.emplace(std::make_pair(y, fn()));
				if (!b) {
					throw std::runtime_error("Unable to create voxel line.");
				}
				return line->second;
			}
			return line->second;
		}


		VoxelChunk* get_voxel_chunk(VoxelLine& line, int x) {
			auto chunk = line.find(x);
			if (chunk == line.end()) {
				return nullptr;
			}
			return &chunk->second;
		}

		void set_voxel_chunk(VoxelLine& line, int x, VoxelChunk chunk) {
			line.emplace(std::make_pair(x, std::move(chunk)));
		}

		void set_voxel_chunk(int x, int y, int z, VoxelChunk chunk) {
			auto& slice = get_voxel_slice_or(z, []() {return VoxelSlice{}; });
			auto& line = get_voxel_line_or(slice, y, []() {return VoxelLine{}; });

			set_voxel_chunk(line, x, std::move(chunk));
		}

		VoxelChunk* get_voxel_chunk(int x, int y, int z) {
			auto* slice = get_voxel_slice(z);
			if (!slice) return nullptr;

			auto* line = get_voxel_line(*slice, y);
			if (!line) return nullptr;

			return get_voxel_chunk(*line, x);
		}

		void create_voxel_chunk(int x, int y, int z) {
			set_voxel_chunk(x, y, z, std::move(VoxelChunk{}));
		}
	};

} // namespace

ENTRY_IMPLEMENT_MAIN(Voxelband, "voxelband", "Voxelband");
