#pragma once

#include <GL/gl.h>
#include <vectormath/c/vectormath_aos.h>

// TODO: Drop need to link to librsx eventually
#include <rsx/rsx.h>
#include <ppu-types.h>

#include <string.h>
#include <malloc.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <ppu-asm.h>

#define MAX_TEXTURES 1024
#define MAX_PROJ_STACK 4 
#define MAX_MODV_STACK 16

// Aliases for simplicity
#define GCM_TEXTURE_CONVOLUTION_NONE 0
#define GCM_TEXTURE_CLAMP_TO_BORDER GCM_TEXTURE_BORDER


enum _ps3gl_rsx_constants
{
	// Vertex Uniforms
	PS3GL_Uniform_ModelViewMatrix,
	PS3GL_Uniform_ProjectionMatrix,

	// Fragment Uniforms
	PS3GL_Uniform_TextureEnabled,
	PS3GL_Uniform_TextureMode,
	PS3GL_Uniform_FogEnabled,
	PS3GL_Uniform_FogColor,

	// Num
	PS3GL_Uniform_Count,
};

enum _ps3gl_texenv_modes
{
	PS3GL_TEXENV_BLEND = 0,
	PS3GL_TEXENV_MODULATE = 1,
	PS3GL_TEXENV_REPLACE = 2,
};

struct ps3gl_texture {
	unsigned int id, target;
	bool allocated;
	unsigned char* data;
	gcmTexture gcmTexture;
	int minFilter, magFilter;
	int wrapS, wrapR, wrapT;
};

struct ps3gl_opengl_state
{

	struct { 
		uint8_t r, g, b, a;
	} clear_color;
	uint32_t color_mask;

	struct {
		uint16_t x,y,w,h;
		float scale[4], offset[4];
	} viewport;

	struct {
		uint16_t x,y,w,h;
	} scissor;

	float depth_near;
	float depth_far;
	bool depth_mask;
	bool depth_test;
	uint32_t depth_func;

	double clear_depth;
	uint32_t clear_stencil;

	// Matrices // TODO: Add Stack for Push/PopMatrix
	uint32_t matrix_mode;
	VmathMatrix4 modelview_matrix;
    VmathMatrix4 projection_matrix;
    VmathMatrix4 *curr_mtx;

	// Textures
	rsxProgramAttrib* texture0Unit;
	bool texture0Enabled;

	GLfloat texEnvMode; 
	struct ps3gl_texture textures[MAX_TEXTURES];
	struct ps3gl_texture *boundTexture;
	GLuint nextTextureID;

	// Lighting
	uint32_t shade_model;

	// Fog
	struct {
		bool enabled;
		int32_t mode;
		float start, end, density;
		float color[4];
	} fog;

	rsxProgramConst *prog_consts[PS3GL_Uniform_Count];

};

// Helper functions for PS3GL, these are internal only!

// From PSL1GHT, can't use it since it's RSX_INTERNAL
static inline f32 swapF32_16(f32 v)
{
	ieee32 d;
	d.f = v;
	d.u = ( ( ( d.u >> 16 ) & 0xffff ) << 0 ) | ( ( ( d.u >> 0 ) & 0xffff ) << 16 );
	return d.f;
}

static inline void rsxSetFragmentProgramParameterBool(gcmContextData *context,const rsxFragmentProgram *program,const rsxProgramConst *param,bool value,u32 offset,u32 location)
{
	f32 params[4] = {0.0f,0.0f,0.0f,0.0f};
	params[0] = swapF32_16((float)value);
	rsxConstOffsetTable *co_table = rsxFragmentProgramGetConstOffsetTable(program, param->index);
	for(int i = 0; i < co_table->num; ++i)
	{
		rsxInlineTransfer(context,
			offset + co_table->offset[i],
			params,
			1,
			location);
	}
}

static inline void rsxSetFragmentProgramParameterF32(gcmContextData *context,const rsxFragmentProgram *program,const rsxProgramConst *param,float value,u32 offset,u32 location)
{
	f32 params[4] = {0.0f,0.0f,0.0f,0.0f};
	params[0] = swapF32_16(value);
	rsxConstOffsetTable *co_table = rsxFragmentProgramGetConstOffsetTable(program, param->index);
	for(int i = 0; i < co_table->num; ++i)
	{
		rsxInlineTransfer(context,
			offset + co_table->offset[i],
			params,
			1,
			location);
	}
}

static inline void rsxSetFragmentProgramParameterF32Vec4(gcmContextData *context,const rsxFragmentProgram *program,const rsxProgramConst *param,float *value,u32 offset,u32 location)
{
	f32 params[4] = {0.0f,0.0f,0.0f,0.0f};
	params[0] = swapF32_16(value[0]);
	params[1] = swapF32_16(value[1]);
	params[2] = swapF32_16(value[2]);
	params[3] = swapF32_16(value[3]);
	rsxConstOffsetTable *co_table = rsxFragmentProgramGetConstOffsetTable(program,
		param->index);
	for(int i = 0; i < co_table->num; ++i)
	{
		rsxInlineTransfer(context,
			offset + co_table->offset[i],
			params,
			4,
			GCM_LOCATION_RSX);
	}
}
