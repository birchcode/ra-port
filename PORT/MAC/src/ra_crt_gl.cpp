#include "ra_crt_gl.h"

#if defined(RA_MOBILE_TOUCH)
// Desktop CRT uses compatibility OpenGL; mobile keeps its SDL renderer.
bool RA_CRTGL_Init(SDL_Window *) { return false; }
void RA_CRTGL_Shutdown(void) {}
bool RA_CRTGL_IsReady(void) { return false; }
void RA_CRTGL_GetOutputSize(int *, int *) {}
bool RA_CRTGL_Present(uint32_t const *, int, int, int, RAAspectViewport, RACRTGLConfig) { return false; }
bool RA_CRTGL_Capture(char const *) { return false; }
#else
#include <SDL_opengl.h>

#include <stdlib.h>
#include <string.h>

static SDL_Window *CRTWindow = 0;
static SDL_GLContext CRTContext = 0;
static GLuint CRTProgram = 0;
static GLuint CRTTexture = 0;
static int CRTTextureWidth = 0;
static int CRTTextureHeight = 0;

static PFNGLACTIVETEXTUREPROC pglActiveTexture = 0;
static PFNGLATTACHSHADERPROC pglAttachShader = 0;
static PFNGLCOMPILESHADERPROC pglCompileShader = 0;
static PFNGLCREATEPROGRAMPROC pglCreateProgram = 0;
static PFNGLCREATESHADERPROC pglCreateShader = 0;
static PFNGLDELETEPROGRAMPROC pglDeleteProgram = 0;
static PFNGLDELETESHADERPROC pglDeleteShader = 0;
static PFNGLGETPROGRAMINFOLOGPROC pglGetProgramInfoLog = 0;
static PFNGLGETPROGRAMIVPROC pglGetProgramiv = 0;
static PFNGLGETSHADERINFOLOGPROC pglGetShaderInfoLog = 0;
static PFNGLGETSHADERIVPROC pglGetShaderiv = 0;
static PFNGLGETUNIFORMLOCATIONPROC pglGetUniformLocation = 0;
static PFNGLLINKPROGRAMPROC pglLinkProgram = 0;
static PFNGLSHADERSOURCEPROC pglShaderSource = 0;
static PFNGLUNIFORM1FPROC pglUniform1f = 0;
static PFNGLUNIFORM1IPROC pglUniform1i = 0;
static PFNGLUNIFORM2FPROC pglUniform2f = 0;
static PFNGLUNIFORM4FPROC pglUniform4f = 0;
static PFNGLUSEPROGRAMPROC pglUseProgram = 0;

static char const *CRTVertexShader =
	"#version 120\n"
	"varying vec2 texture_coordinate;\n"
	"void main()\n"
	"{\n"
	"  gl_Position = gl_Vertex;\n"
	"  texture_coordinate = gl_MultiTexCoord0.xy;\n"
	"}\n";

static char const *CRTFragmentShader =
	"#version 120\n"
	"uniform sampler2D source_texture;\n"
	"uniform vec2 texture_size;\n"
	"uniform vec2 source_size;\n"
	"uniform vec4 viewport;\n"
	"uniform float output_height;\n"
	"uniform float effect_mix;\n"
	"uniform int legacy_mode;\n"
	"uniform float glass_glow;\n"
	"uniform int split_mode;\n"
	"uniform int subpixel_mask;\n"
	"uniform int bgr_panel;\n"
	"uniform int test_pattern;\n"
	"varying vec2 texture_coordinate;\n"
	"vec3 to_linear(vec3 value)\n"
	"{\n"
	"  return pow(max(value, vec3(0.0)), vec3(2.2));\n"
	"}\n"
	"vec3 to_srgb(vec3 value)\n"
	"{\n"
	"  return pow(max(value, vec3(0.0)), vec3(1.0 / 2.2));\n"
	"}\n"
	"vec3 input_colour(vec2 coordinate)\n"
	"{\n"
	"  if (test_pattern == 1) return vec3(1.0);\n"
	"  if (test_pattern == 2) return vec3(0.5);\n"
	"  if (test_pattern == 3) return vec3(1.0, 0.0, 0.0);\n"
	"  if (test_pattern == 4) return vec3(0.0, 1.0, 0.0);\n"
	"  if (test_pattern == 5) return vec3(0.0, 0.0, 1.0);\n"
	"  vec2 cell = floor(coordinate * texture_size);\n"
	"  float ramp = clamp(cell.x / max(source_size.x - 1.0, 1.0), 0.0, 1.0);\n"
	"  if (test_pattern == 6) return vec3(ramp);\n"
	"  if (test_pattern == 7) return vec3(mod(cell.y, 2.0));\n"
	"  if (test_pattern == 8) return vec3(mod(cell.x, 2.0));\n"
	"  if (test_pattern == 9) return vec3(abs(cell.x - floor(source_size.x * 0.5)) < 0.5 || abs(cell.y - floor(source_size.y * 0.5)) < 0.5 ? 1.0 : 0.0);\n"
	"  if (test_pattern == 10) {\n"
	"    float band = floor(cell.y * 3.0 / source_size.y);\n"
	"    return band < 1.0 ? vec3(ramp, 0.0, 0.0) : band < 2.0 ? vec3(0.0, ramp, 0.0) : vec3(0.0, 0.0, ramp);\n"
	"  }\n"
	"  return texture2D(source_texture, coordinate).rgb;\n"
	"}\n"
	"float luminance(vec3 value)\n"
	"{\n"
	"  return dot(value, vec3(0.2126, 0.7152, 0.0722));\n"
	"}\n"
	"float rounded_slot(float horizontal, float vertical)\n"
	"{\n"
	"  float shape = pow(abs(horizontal), 4.0) + pow(abs(vertical), 4.0);\n"
	"  return 1.0 - smoothstep(0.58, 1.0, shape);\n"
	"}\n"
	"vec4 mask_at(vec2 pixel)\n"
	"{\n"
	"  if (subpixel_mask != 0) {\n"
	"    float column = floor(pixel.x);\n"
	"    float stagger = mod(column, 2.0) * 3.0;\n"
	"    float vertical = (mod(pixel.y + stagger, 6.0) - 3.0) / 2.65;\n"
	"    float slot = rounded_slot(0.0, vertical);\n"
	"    return vec4(vec3(slot), slot);\n"
	"  }\n"
	"  float triad = floor(pixel.x / 6.0);\n"
	"  float local_x = mod(pixel.x, 6.0);\n"
	"  float phosphor = floor(local_x / 2.0);\n"
	"  float horizontal = (mod(local_x, 2.0) - 1.0) / 0.72;\n"
	"  float stagger = mod(triad, 2.0) * 4.0;\n"
	"  float vertical = (mod(pixel.y + stagger, 8.0) - 4.0) / 3.25;\n"
	"  float slot = rounded_slot(horizontal, vertical);\n"
	"  vec3 mask = phosphor < 1.0 ? vec3(slot, 0.0, 0.0) :\n"
	"              phosphor < 2.0 ? vec3(0.0, slot, 0.0) : vec3(0.0, 0.0, slot);\n"
	"  if (bgr_panel != 0) mask = mask.bgr;\n"
	"  return vec4(mask, slot);\n"
	"}\n"
	"void legacy_main()\n"
	"{\n"
	"  vec2 uv = texture_coordinate;\n"
	"  vec3 clean_srgb = input_colour(uv);\n"
	"  if (split_mode != 0 && gl_FragCoord.x < viewport.x + viewport.z * 0.5) {\n"
	"    gl_FragColor = vec4(clean_srgb, 1.0);\n"
	"    return;\n"
	"  }\n"
	"  vec2 texel = vec2(1.0 / texture_size.x, 0.0);\n"
	"  vec3 centre = to_linear(clean_srgb);\n"
	"  vec3 left = to_linear(input_colour(uv - texel * 0.65));\n"
	"  vec3 right = to_linear(input_colour(uv + texel * 0.65));\n"
	"  float focus = 0.10 + luminance(centre) * 0.12;\n"
	"  vec3 beam_colour = centre * (1.0 - 2.0 * focus) + (left + right) * focus;\n"
	"  float source_y = uv.y * source_size.y;\n"
	"  float raster_distance = fract(source_y) - 0.5;\n"
	"  float beam = 0.32 + 0.68 * exp(-raster_distance * raster_distance * 10.0);\n"
	"  vec2 pixel = vec2(gl_FragCoord.x - viewport.x, output_height - gl_FragCoord.y - viewport.y);\n"
	"  vec4 phosphor = mask_at(subpixel_mask != 0 ? pixel : pixel * 2.0);\n"
	"  vec3 emitted = beam_colour * beam * phosphor.rgb;\n"
	"  if (subpixel_mask != 0) emitted = beam_colour * beam * phosphor.a;\n"
	"  float bright = max(luminance(beam_colour) - 0.72, 0.0);\n"
	"  vec3 halation = (left + centre + right) * (bright * 0.035);\n"
	"  vec3 exposed = vec3(1.0) - exp(-(emitted * (subpixel_mask != 0 ? 1.55 : 4.2) + halation));\n"
	"  vec3 result = mix(centre, exposed, effect_mix);\n"
	"  gl_FragColor = vec4(to_srgb(result), 1.0);\n"
	"}\n"
	"\n"
	"// Exact sRGB transfer for the calibrated path. Legacy retains its 2.2 approximation.\n"
	"vec3 decode(vec3 c) {\n"
	"  return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), c));\n"
	"}\n"
	"vec3 encode(vec3 c) {\n"
	"  c = max(c, vec3(0.0));\n"
	"  return mix(12.92 * c, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), c));\n"
	"}\n"
	"vec3 source_at(vec2 cell) {\n"
	"  return decode(input_colour((clamp(cell, vec2(0.0), source_size - 1.0) + 0.5) / texture_size));\n"
	"}\n"
	"float gaussian(float d, float sigma) {\n"
	"  return exp(-0.5 * d * d / (sigma * sigma)) / (2.50662827 * sigma);\n"
	"}\n"
	"// Compact smooth kernel: zero value and slope at the sample boundary.\n"
	"float glow_weight(float distance) {\n"
	"  float t = max(1.0 - distance * distance / 2.25, 0.0);\n"
	"  return t * t;\n"
	"}\n"
	"void main() {\n"
	"  if (legacy_mode != 0) { legacy_main(); return; }\n"
	"  if (split_mode != 0 && gl_FragCoord.x < viewport.x + viewport.z * 0.5) {\n"
	"    gl_FragColor = vec4(input_colour(texture_coordinate), 1.0);\n"
	"    return;\n"
	"  }\n"
	"  vec2 position = texture_coordinate * texture_size - 0.5;\n"
	"  vec2 base = floor(position + 0.5);\n"
	"  vec2 footprint = source_size / viewport.zw;\n"
	"  // Approximate destination box integration by adding its variance to the beam.\n"
	"  float sx = sqrt(0.34 * 0.34 + footprint.x * footprint.x / 12.0);\n"
	"  vec3 light = vec3(0.0);\n"
	"  float energy = 0.0;\n"
	"  vec3 halo = vec3(0.0);\n"
	"  float halo_energy = 0.0;\n"
	"  for (int y = -2; y <= 2; ++y) {\n"
	"    vec3 row = vec3(0.0);\n"
	"    float horizontal_energy = 0.0;\n"
	"    for (int x = -1; x <= 1; ++x) {\n"
	"      float weight = gaussian(position.x - base.x - float(x), sx);\n"
	"      vec3 sample_colour = source_at(base + vec2(float(x), float(y)));\n"
	"      row += sample_colour * weight;\n"
	"      if (glass_glow > 0.0) {\n"
	"        float spread = glow_weight(position.x - base.x - float(x)) *\n"
	"                       glow_weight(position.y - base.y - float(y));\n"
	"        float peak = max(sample_colour.r, max(sample_colour.g, sample_colour.b));\n"
	"        halo += sample_colour * smoothstep(0.55, 0.9, peak) * spread;\n"
	"        halo_energy += spread;\n"
	"      }\n"
	"      horizontal_energy += weight;\n"
	"    }\n"
	"    row /= horizontal_energy;\n"
	"    // Peak channel preserves equal focus for saturated primaries.\n"
	"    float width = 0.27 + 0.16 * sqrt(max(row.r, max(row.g, row.b)));\n"
	"    float sy = sqrt(width * width + footprint.y * footprint.y / 12.0);\n"
	"    float weight = gaussian(position.y - base.y - float(y), sy);\n"
	"    light += row * weight;\n"
	"    energy += weight;\n"
	"  }\n"
	"  light /= energy;\n"
	"  // Retain raster contrast with zero-mean energy modulation; spend SDR\n"
	"  // headroom on detail rather than clipping or compressing the whole tone curve.\n"
	"  light += min(light, 1.0 - light) * clamp(energy - 1.0, -1.0, 1.0);\n"
	"  vec2 pixel = vec2(gl_FragCoord.x - viewport.x, output_height - gl_FragCoord.y - viewport.y);\n"
	"  vec4 mask = mask_at(subpixel_mask != 0 ? pixel : pixel * 2.0);\n"
	"  // Discrete means of the existing screen-anchored 3x4 RGB / 2x6 slot tiles.\n"
	"  vec3 modulation = mask.rgb / (subpixel_mask != 0 ? 0.830840960 : 0.286902313) - 1.0;\n"
	"  // The approved output-pixel pitch is retained. Fade contrast at small source\n"
	"  // scales; a physical tube-density model needs verified panel/backing scaling.\n"
	"  float visibility = smoothstep(1.5, 3.0, viewport.w / source_size.y);\n"
	"  float contrast = effect_mix * visibility;\n"
	"  // RGB modulation stays below 3; reserve highlight headroom for every channel.\n"
	"  vec3 room = min(light, (1.0 - light) / (subpixel_mask != 0 ? 1.0 : 3.0));\n"
	"  light += room * modulation * contrast;\n"
	"  // ponytail: source-highlight approximation reuses the beam taps. A wider\n"
	"  // optical halo would need a separately filtered emission buffer.\n"
	"  if (glass_glow > 0.0) light += (1.0 - light) * (halo / halo_energy) * glass_glow;\n"
	"  gl_FragColor = vec4(encode(light), 1.0);\n"
	"}\n";
template <typename T>
static bool load_gl_proc(T *target, char const *name)
{
	*target = (T)SDL_GL_GetProcAddress(name);
	if (!*target) SDL_Log("CRT OpenGL missing %s", name);
	return *target != 0;
}

static bool load_gl_functions(void)
{
	return load_gl_proc(&pglActiveTexture, "glActiveTexture") &&
		load_gl_proc(&pglAttachShader, "glAttachShader") &&
		load_gl_proc(&pglCompileShader, "glCompileShader") &&
		load_gl_proc(&pglCreateProgram, "glCreateProgram") &&
		load_gl_proc(&pglCreateShader, "glCreateShader") &&
		load_gl_proc(&pglDeleteProgram, "glDeleteProgram") &&
		load_gl_proc(&pglDeleteShader, "glDeleteShader") &&
		load_gl_proc(&pglGetProgramInfoLog, "glGetProgramInfoLog") &&
		load_gl_proc(&pglGetProgramiv, "glGetProgramiv") &&
		load_gl_proc(&pglGetShaderInfoLog, "glGetShaderInfoLog") &&
		load_gl_proc(&pglGetShaderiv, "glGetShaderiv") &&
		load_gl_proc(&pglGetUniformLocation, "glGetUniformLocation") &&
		load_gl_proc(&pglLinkProgram, "glLinkProgram") &&
		load_gl_proc(&pglShaderSource, "glShaderSource") &&
		load_gl_proc(&pglUniform1f, "glUniform1f") &&
		load_gl_proc(&pglUniform1i, "glUniform1i") &&
		load_gl_proc(&pglUniform2f, "glUniform2f") &&
		load_gl_proc(&pglUniform4f, "glUniform4f") &&
		load_gl_proc(&pglUseProgram, "glUseProgram");
}

static GLuint compile_shader(GLenum type, char const *source)
{
	GLuint shader = pglCreateShader(type);
	pglShaderSource(shader, 1, &source, 0);
	pglCompileShader(shader);
	GLint compiled = 0;
	pglGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		char log[2048];
		GLsizei length = 0;
		pglGetShaderInfoLog(shader, sizeof(log), &length, log);
		SDL_Log("CRT shader compilation failed: %s", log);
		pglDeleteShader(shader);
		return 0;
	}
	return shader;
}

static GLuint create_program(void)
{
	GLuint vertex = compile_shader(GL_VERTEX_SHADER, CRTVertexShader);
	GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, CRTFragmentShader);
	if (!vertex || !fragment) {
		if (vertex) pglDeleteShader(vertex);
		if (fragment) pglDeleteShader(fragment);
		return 0;
	}
	GLuint program = pglCreateProgram();
	pglAttachShader(program, vertex);
	pglAttachShader(program, fragment);
	pglLinkProgram(program);
	pglDeleteShader(vertex);
	pglDeleteShader(fragment);
	GLint linked = 0;
	pglGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		char log[2048];
		GLsizei length = 0;
		pglGetProgramInfoLog(program, sizeof(log), &length, log);
		SDL_Log("CRT shader link failed: %s", log);
		pglDeleteProgram(program);
		return 0;
	}
	return program;
}

bool RA_CRTGL_Init(SDL_Window *window)
{
	CRTWindow = window;
	CRTContext = SDL_GL_CreateContext(window);
	if (!CRTContext || SDL_GL_MakeCurrent(window, CRTContext) != 0 || !load_gl_functions()) {
		RA_CRTGL_Shutdown();
		return false;
	}
	SDL_GL_SetSwapInterval(0);
	CRTProgram = create_program();
	if (!CRTProgram) {
		RA_CRTGL_Shutdown();
		return false;
	}
	glGenTextures(1, &CRTTexture);
	glBindTexture(GL_TEXTURE_2D, CRTTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	SDL_Log("Red Alert CRT renderer=OpenGL shader");
	return true;
}

void RA_CRTGL_Shutdown(void)
{
	if (CRTContext && CRTWindow) SDL_GL_MakeCurrent(CRTWindow, CRTContext);
	if (CRTTexture) glDeleteTextures(1, &CRTTexture);
	if (CRTProgram && pglDeleteProgram) pglDeleteProgram(CRTProgram);
	CRTTexture = 0;
	CRTProgram = 0;
	CRTTextureWidth = 0;
	CRTTextureHeight = 0;
	if (CRTContext) SDL_GL_DeleteContext(CRTContext);
	CRTContext = 0;
	CRTWindow = 0;
}

bool RA_CRTGL_IsReady(void)
{
	return CRTWindow && CRTContext && CRTProgram && CRTTexture;
}

void RA_CRTGL_GetOutputSize(int *width, int *height)
{
	int output_width = 0;
	int output_height = 0;
	if (CRTWindow) SDL_GL_GetDrawableSize(CRTWindow, &output_width, &output_height);
	if (width) *width = output_width;
	if (height) *height = output_height;
}

static void set_uniform_1i(char const *name, int value)
{
	pglUniform1i(pglGetUniformLocation(CRTProgram, name), value);
}

static void set_uniform_1f(char const *name, float value)
{
	pglUniform1f(pglGetUniformLocation(CRTProgram, name), value);
}

bool RA_CRTGL_Present(
	uint32_t const *pixels,
	int texture_width,
	int texture_height,
	int content_width,
	RAAspectViewport viewport,
	RACRTGLConfig config)
{
	if (!RA_CRTGL_IsReady() || !pixels || texture_width <= 0 || texture_height <= 0 || content_width <= 0) return false;
	SDL_GL_MakeCurrent(CRTWindow, CRTContext);
	int output_width = 0;
	int output_height = 0;
	RA_CRTGL_GetOutputSize(&output_width, &output_height);
	if (output_width <= 0 || output_height <= 0) return false;
	glViewport(0, 0, output_width, output_height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	pglActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, CRTTexture);
	if (CRTTextureWidth != texture_width || CRTTextureHeight != texture_height) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width, texture_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
		CRTTextureWidth = texture_width;
		CRTTextureHeight = texture_height;
	} else {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture_width, texture_height, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
	}
	pglUseProgram(CRTProgram);
	set_uniform_1i("source_texture", 0);
	pglUniform2f(pglGetUniformLocation(CRTProgram, "texture_size"), (float)texture_width, (float)texture_height);
	pglUniform2f(pglGetUniformLocation(CRTProgram, "source_size"), (float)content_width, (float)texture_height);
	pglUniform4f(pglGetUniformLocation(CRTProgram, "viewport"), (float)viewport.x, (float)viewport.y, (float)viewport.w, (float)viewport.h);
	set_uniform_1f("output_height", (float)output_height);
	set_uniform_1f("effect_mix", (float)RA_ClampInt(config.strength, 0, 100) / 100.0f);
	set_uniform_1i("split_mode", config.split);
	char const *model = getenv("RA_CRT_MODEL");
	set_uniform_1i("legacy_mode", model && strcmp(model, "legacy") == 0);
	char const *glow = getenv("RA_CRT_GLOW");
	set_uniform_1f("glass_glow", (float)RA_ClampInt(glow ? atoi(glow) : 0, 0, 100) / 1000.0f);
	set_uniform_1i("subpixel_mask", config.subpixel_mask);
	set_uniform_1i("bgr_panel", config.bgr_panel);
	set_uniform_1i("test_pattern", config.test_pattern);
	float left = -1.0f + (2.0f * viewport.x / output_width);
	float right = -1.0f + (2.0f * (viewport.x + viewport.w) / output_width);
	float top = 1.0f - (2.0f * viewport.y / output_height);
	float bottom = 1.0f - (2.0f * (viewport.y + viewport.h) / output_height);
	float source_right = (float)content_width / texture_width;
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(left, top);
	glTexCoord2f(source_right, 0.0f); glVertex2f(right, top);
	glTexCoord2f(source_right, 1.0f); glVertex2f(right, bottom);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(left, bottom);
	glEnd();
	pglUseProgram(0);
	if (config.split) {
		float divider = -1.0f + (2.0f * (output_width / 2) / output_width);
		glColor3f(1.0f, 1.0f, 1.0f);
		glBegin(GL_LINES);
		glVertex2f(divider, -1.0f);
		glVertex2f(divider, 1.0f);
		glEnd();
	}
	SDL_GL_SwapWindow(CRTWindow);
	return true;
}

bool RA_CRTGL_Capture(char const *path)
{
	if (!RA_CRTGL_IsReady() || !path || !path[0]) return false;
	int width = 0;
	int height = 0;
	RA_CRTGL_GetOutputSize(&width, &height);
	SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
	if (!surface) return false;
	unsigned char *row = (unsigned char *)malloc((size_t)surface->pitch);
	if (!row) {
		SDL_FreeSurface(surface);
		return false;
	}
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
	for (int y = 0; y < height / 2; ++y) {
		unsigned char *top = (unsigned char *)surface->pixels + y * surface->pitch;
		unsigned char *bottom = (unsigned char *)surface->pixels + (height - 1 - y) * surface->pitch;
		memcpy(row, top, surface->pitch);
		memcpy(top, bottom, surface->pitch);
		memcpy(bottom, row, surface->pitch);
	}
	free(row);
	bool saved = SDL_SaveBMP(surface, path) == 0;
	SDL_FreeSurface(surface);
	return saved;
}

#endif
