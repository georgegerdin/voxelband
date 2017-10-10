#ifndef renderer_hh__
#define renderer_hh__

template<typename T>
class SafeWrapper {
private:
	T m_handle = BGFX_INVALID_HANDLE;
public:
	explicit SafeWrapper() {}
	explicit SafeWrapper(T handle) : m_handle(handle) {}
	explicit SafeWrapper(SafeWrapper<T> const&) = delete;
	explicit SafeWrapper(SafeWrapper<T> && other) {
		*this = std::move(other);
	}
	~SafeWrapper() {
		if (isValid(m_handle)) {
			bgfx::destroy(m_handle);
			m_handle = BGFX_INVALID_HANDLE;
		}
	}
	SafeWrapper<T>& operator=(SafeWrapper<T> const&) = delete;
	SafeWrapper<T>& operator=(SafeWrapper<T>&& other) {
		if (this != &other) {
			m_handle = other.m_handle;
			other.m_handle = BGFX_INVALID_HANDLE;
		}
		return *this;
	}

	T const& handle() const { return m_handle; }
	void set(T const& value) {
		if (isValid(m_handle)) {
			bgfx::destroy(m_handle);
			m_handle = BGFX_INVALID_HANDLE;
		}
		m_handle = value;
	}

	bool is_valid() {
		return isValid(m_handle);
	}
};

struct  Texture : SafeWrapper<bgfx::TextureHandle> {
	using SafeWrapper<bgfx::TextureHandle>::SafeWrapper;

	bool load(const char* filename) {
		auto handle = loadTexture(filename);
		if (!isValid(handle))
			return false;
		set(handle);
		return true;
	}
};

struct Uniform : SafeWrapper<bgfx::UniformHandle> {
	using SafeWrapper<bgfx::UniformHandle>::SafeWrapper;
	
	bool create(const char* name, bgfx::UniformType::Enum type, uint16_t num = (uint16_t) 1U) {
		auto handle = bgfx::createUniform(name, type, num);
		if (!isValid(handle))
			return false;
		set(handle);
		return true;
	}
};

struct DynamicVertexBuffer : SafeWrapper<bgfx::DynamicVertexBufferHandle> {
	using SafeWrapper<bgfx::DynamicVertexBufferHandle>::SafeWrapper;
	
	void update(uint32_t _startVertex, const bgfx::Memory* _mem) {
		bgfx::updateDynamicVertexBuffer(handle(), _startVertex, _mem);
	}

	bool create(const bgfx::Memory* _mem
		, const bgfx::VertexDecl& _decl
		, uint16_t _flags = BGFX_BUFFER_NONE
	) {
		auto handle = bgfx::createDynamicVertexBuffer(
			_mem
			, _decl
			, _flags);

		if (!isValid(handle))
			return false;
		set(handle);
		return true;
	}
};

struct DynamicIndexBuffer : SafeWrapper<bgfx::DynamicIndexBufferHandle> {
	using SafeWrapper<bgfx::DynamicIndexBufferHandle>::SafeWrapper;

	void update(uint32_t _startIndex, const bgfx::Memory* _mem) {
		bgfx::updateDynamicIndexBuffer(handle(), _startIndex, _mem);
	}

	bool create(const bgfx::Memory* _mem
		, uint16_t _flags = BGFX_BUFFER_NONE)
	{
		auto handle = bgfx::createDynamicIndexBuffer(_mem, _flags);
		if (!isValid(handle))
			return false;
		set(handle);
		return true;
	}
};

struct ShaderProgram : SafeWrapper<bgfx::ProgramHandle>
{
	using SafeWrapper<bgfx::ProgramHandle>::SafeWrapper;

	bool load(const char* vertexshader, const char* fragmentshader) {
		auto handle = loadProgram(vertexshader, fragmentshader);
		if (!isValid(handle))
			return false;
		set(handle);
		return true;
	}
};

class VoxelChunk;

class Renderer {
public:
	void init(boost::filesystem::path path);
	void init_frame(float stime);
	void render(VoxelChunk const&);
	void render_rock(DynamicVertexBuffer const& vb, DynamicIndexBuffer const& ib);
protected:
	Texture m_texture_color, m_texture_normal;
	ShaderProgram m_bump_mapping_shader;
	Uniform s_texColor;
	Uniform s_texNormal;
	Uniform u_lightPosRadius;
	Uniform u_lightRgbInnerR;
	uint16_t m_numLights;
};

#endif // !renderer_hh__
