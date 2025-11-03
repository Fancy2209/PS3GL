// PS3GL - An OpenGL 1.5 Compatibility Layer on top of the RSX API

#include <rsx/rsx.h>
#include <GL/gl.h>
#include <vectormath/c/vectormath_aos.h>

#include <string.h>
#include <malloc.h>
#include <math.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// TODO: Make the rsxutil functionality into ps3glInit
#include <rsxutil.h>

// TODO: Maybe split the shaders instead of the many ifs?
#include "ffp_shader_vpo.h"
#include "ffp_shader_fpo.h"

// Copied from OpenGX, not sure if I should modify these values
#define MAX_PROJ_STACK 4   // Proj. matrix stack depth
#define MAX_MODV_STACK 16  // Modelview matrix stack depth

enum _ps3gl_rsx_constants
{
	PS3GL_Uniform_ModelViewMatrix,
	PS3GL_Uniform_ProjectionMatrix,
	PS3GL_Uniform_TextureEnabled,
	PS3GL_Uniform_TextureMode,
	PS3GL_Uniform_TextureHasAlpha,
	PS3GL_Uniform_FogEnabled,
	PS3GL_Uniform_FogColor,
	PS3GL_Uniform_Count,
};


struct ps3gl_texture {
	GLuint id;
	GLboolean allocated;
	GLenum target;
	GLuint* data;
	GLint width, height, bpp;
	GLuint fmt;
	GLboolean hasAlpha;
};

#define MAX_TEXTURES 1024

static struct
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

	uint32_t clear_depth;
	float depth_near;
	float depth_far;
	bool depth_mask;
	bool depth_test;
	uint32_t depth_func;

	uint32_t clear_stencil;

	// Matrices // TODO: Add Stack for Push/PopMatrix
	uint32_t matrix_mode;
	VmathMatrix4 modelview_matrix;
    VmathMatrix4 projection_matrix;
    VmathMatrix4 *curr_mtx;

	// Textures

	// textures
	rsxProgramAttrib* texture0Unit;
	bool texture0Enabled;
	bool texture0HasAlpha;

	uint32_t texEnvMode; 
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

} _opengl_state;

u32 fp_offset;
u32 *fp_buffer;

void *vp_ucode = NULL;
rsxVertexProgram *vpo = (rsxVertexProgram*)ffp_shader_vpo;

void *fp_ucode = NULL;
rsxFragmentProgram *fpo = (rsxFragmentProgram*)ffp_shader_fpo;


/*
 * Miscellaneous
 */

void glClearIndex( GLfloat c );

void glClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
{
	_opengl_state.clear_color.a = ((uint8_t)(alpha * 255.0f)); 
	_opengl_state.clear_color.r = ((uint8_t)(red   * 255.0f)); 
	_opengl_state.clear_color.g = ((uint8_t)(green * 255.0f));
	_opengl_state.clear_color.b = ((uint8_t)(blue  * 255.0f));
}

void glClear( GLbitfield mask )
{
	uint32_t rsx_mask = 0;

	if(mask & GL_COLOR_BUFFER_BIT)
	{
		rsx_mask |= (GCM_CLEAR_A | GCM_CLEAR_R | GCM_CLEAR_G | GCM_CLEAR_B);
		rsxSetClearColor(context,
			(_opengl_state.clear_color.a << 24) |
        	(_opengl_state.clear_color.r << 16) |
        	(_opengl_state.clear_color.g << 8)  |
        	(_opengl_state.clear_color.b << 0)
		);
	}
	if(mask & GL_DEPTH_BUFFER_BIT)
	{
		rsxSetClearDepthStencil(context, (_opengl_state.clear_depth << 8) | (_opengl_state.clear_stencil & 0xFF));
		rsx_mask |= GCM_CLEAR_Z;
	}

	if(mask & GL_STENCIL_BUFFER_BIT)
	{
		rsxSetClearDepthStencil(context, (_opengl_state.clear_depth << 8) | (_opengl_state.clear_stencil & 0xFF));
		rsx_mask |= GCM_CLEAR_S;
	}

	rsxClearSurface(context, rsx_mask);
}

void glColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha )
{
	if(red)   _opengl_state.color_mask |= GCM_COLOR_MASK_R;
	if(green) _opengl_state.color_mask |= GCM_COLOR_MASK_G;
	if(blue)  _opengl_state.color_mask |= GCM_COLOR_MASK_B;
	if(alpha) _opengl_state.color_mask |= GCM_COLOR_MASK_A;
}

void glScissor( GLint x, GLint y, GLsizei width, GLsizei height)
{
	_opengl_state.scissor.x = x;
	_opengl_state.scissor.y = y;
	_opengl_state.scissor.w = width;
	_opengl_state.scissor.h = height;
}

void glEnable( GLenum cap )
{
	switch(cap)
	{
		case GL_FOG:
			_opengl_state.fog.enabled = true;
			break;
		case GL_DEPTH_TEST:
			_opengl_state.depth_test = true;
			break;
		case GL_TEXTURE_2D:
			_opengl_state.texture0Enabled = false;
			break;
		default:
			break;
	}
}

void glDisable( GLenum cap )
{
	switch(cap)
	{
		case GL_FOG:
			_opengl_state.fog.enabled = false;
			break;
		case GL_DEPTH_TEST:
			_opengl_state.depth_test = false;
			break;
		case GL_TEXTURE_2D:
			_opengl_state.texture0Enabled = false;
			break;
		default:
			break;
	}
}

GLenum glGetError( void ) { return GL_NO_ERROR; } // TODO?

const GLubyte * glGetString( GLenum name );

void glFinish( void ) {} // We call rsxFinish every frame

void glFlush( void ) {} // We call rsxFlushBuffer every frame

void glHint( GLenum target, GLenum mode ) {} // No idea how to implement this

/*
 * Depth Buffer
 */

void glClearDepth( GLclampd depth )
{
	_opengl_state.clear_depth = (uint32_t)(depth * 0xFFFFFF);	
}

void glDepthFunc( GLenum func )
{
	// Values is the same between what OpenGL defines 
	// and what the RSX expects
	_opengl_state.depth_func = func;
}

void glDepthMask( GLboolean flag )
{
	_opengl_state.depth_mask = flag;
}

void glDepthRange( GLclampd near_val, GLclampd far_val )
{
	_opengl_state.depth_near = near_val;
	_opengl_state.depth_far  = far_val;
}

/*
 * Transformation
 */


void glMatrixMode( GLenum mode )
{
	_opengl_state.matrix_mode = mode;
	switch(mode)
	{
		case GL_MODELVIEW:
			_opengl_state.curr_mtx = &_opengl_state.modelview_matrix;
			break;
		case GL_PROJECTION:
			_opengl_state.curr_mtx = &_opengl_state.projection_matrix;
			break;
		default:
			fprintf(stderr, "Unimplemented MatrixMode: %u", mode);
			break;
	}
}

void glOrtho( GLdouble left, GLdouble right,
                                 GLdouble bottom, GLdouble top,
                                 GLdouble near_val, GLdouble far_val )
{
	VmathMatrix4 ortho;
	vmathM4MakeOrthographic(&ortho, (GLfloat)left, (GLfloat)right, (GLfloat)bottom, (GLfloat)top, (GLfloat)near_val, (GLfloat)far_val);
	vmathM4Mul(_opengl_state.curr_mtx, _opengl_state.curr_mtx, &ortho);
}

void glFrustum( GLdouble left, GLdouble right,
                                   GLdouble bottom, GLdouble top,
                                   GLdouble near_val, GLdouble far_val )
{
	VmathMatrix4 frustum;
	vmathM4MakeFrustum(&frustum, (GLfloat)left, (GLfloat)right, (GLfloat)bottom, (GLfloat)top, (GLfloat)near_val, (GLfloat)far_val);
	
	VmathMatrix4 result;
	vmathM4Mul(&result, _opengl_state.curr_mtx, &frustum);
	vmathM4Copy(_opengl_state.curr_mtx, &result);
}

void glViewport( GLint x, GLint y, GLsizei width, GLsizei height )
{

	_opengl_state.viewport.x = x;
	_opengl_state.viewport.y = display_height - y - height;
	_opengl_state.viewport.w = width;
	_opengl_state.viewport.h = height;

	if(_opengl_state.scissor.h == 0)
	{
		_opengl_state.scissor.x = x;
		_opengl_state.scissor.y = y; // TODO: Check if this has to be display_height - y - height;
		_opengl_state.scissor.w = width;
		_opengl_state.scissor.h = height;
	}

	_opengl_state.viewport.scale[0] = width*0.5f;
	_opengl_state.viewport.scale[1] = height*-0.5f;
	_opengl_state.viewport.scale[2] = (_opengl_state.depth_far - _opengl_state.depth_near)*0.5f;
	_opengl_state.viewport.scale[3] = 0.0f;
	_opengl_state.viewport.offset[0] = x + width*0.5f;
	_opengl_state.viewport.offset[1] = y + height*0.5f;
	_opengl_state.viewport.offset[2] = (_opengl_state.depth_far + _opengl_state.depth_near)*0.5f;
	_opengl_state.viewport.offset[3] = 0.0f;
}

void glPushMatrix(void); // TODO

void glPopMatrix(void); // TODO

void glLoadIdentity(void)
{
	vmathM4MakeIdentity(_opengl_state.curr_mtx);
}

void glLoadMatrixf( const GLfloat *m )
{
	VmathVector4 col0;
	VmathVector4 col1;
	VmathVector4 col2;
	VmathVector4 col3;

	vmathV4MakeFromElems(&col0, m[0],  m[1],  m[2],  m[3]);
	vmathV4MakeFromElems(&col1, m[4],  m[5],  m[6],  m[7]);
	vmathV4MakeFromElems(&col2, m[8],  m[9],  m[10], m[11]);
	vmathV4MakeFromElems(&col3, m[12], m[13], m[14], m[15]);
	
	vmathM4MakeFromCols(_opengl_state.curr_mtx, &col0, &col1, &col2, &col3);
}


void glLoadMatrixd( const GLdouble *m )
{
	VmathVector4 col0;
	VmathVector4 col1;
	VmathVector4 col2;
	VmathVector4 col3;

	vmathV4MakeFromElems(&col0, (GLfloat)m[0],  (GLfloat)m[1],  (GLfloat)m[2],  (GLfloat)m[3]);
	vmathV4MakeFromElems(&col1, (GLfloat)m[4],  (GLfloat)m[5],  (GLfloat)m[6],  (GLfloat)m[7]);
	vmathV4MakeFromElems(&col2, (GLfloat)m[8],  (GLfloat)m[9],  (GLfloat)m[10], (GLfloat)m[11]);
	vmathV4MakeFromElems(&col3, (GLfloat)m[12], (GLfloat)m[13], (GLfloat)m[14], (GLfloat)m[15]);
	
	vmathM4MakeFromCols(_opengl_state.curr_mtx, &col0, &col1, &col2, &col3);
}

void glMultMatrixf( const GLfloat *m )
{
	VmathVector4 col0;
	VmathVector4 col1;
	VmathVector4 col2;
	VmathVector4 col3;

	vmathV4MakeFromElems(&col0, m[0],  m[1],  m[2],  m[3]);
	vmathV4MakeFromElems(&col1, m[4],  m[5],  m[6],  m[7]);
	vmathV4MakeFromElems(&col2, m[8],  m[9],  m[10], m[11]);
	vmathV4MakeFromElems(&col3, m[12], m[13], m[14], m[15]);

	VmathMatrix4 mulMatrix;
	vmathM4MakeFromCols(&mulMatrix, &col0, &col1, &col2, &col3);

	VmathMatrix4 result;
	vmathM4Mul(&result, _opengl_state.curr_mtx, &mulMatrix);
	vmathM4Copy(_opengl_state.curr_mtx, &result);
}

void glMultMatrixd( const GLdouble *m )
{
	VmathVector4 col0;
	VmathVector4 col1;
	VmathVector4 col2;
	VmathVector4 col3;
	
	vmathV4MakeFromElems(&col0, (GLfloat)m[0],  (GLfloat)m[1],  (GLfloat)m[2],  (GLfloat)m[3]);
	vmathV4MakeFromElems(&col1, (GLfloat)m[4],  (GLfloat)m[5],  (GLfloat)m[6],  (GLfloat)m[7]);
	vmathV4MakeFromElems(&col2, (GLfloat)m[8],  (GLfloat)m[9],  (GLfloat)m[10], (GLfloat)m[11]);
	vmathV4MakeFromElems(&col3, (GLfloat)m[12], (GLfloat)m[13], (GLfloat)m[14], (GLfloat)m[15]);
	
	VmathMatrix4 mulMatrix;
	vmathM4MakeFromCols(&mulMatrix, &col0, &col1, &col2, &col3);

	VmathMatrix4 result;
	vmathM4Mul(&result, _opengl_state.curr_mtx, &mulMatrix);
	vmathM4Copy(_opengl_state.curr_mtx, &result);
}

void glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
	VmathVector3 unitVec;
	vmathV3MakeFromElems(&unitVec, x, y, z);
	vmathV3Normalize(&unitVec, &unitVec);

	VmathMatrix4 rotation;
	vmathM4MakeRotationAxis(&rotation, (M_PI/180)*angle, &unitVec);

	VmathMatrix4 result;
	vmathM4Mul(&result, _opengl_state.curr_mtx, &rotation);
	vmathM4Copy(_opengl_state.curr_mtx, &result);
}

void glRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z )
{
	glRotatef((GLfloat)angle, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}

void glScalef( GLfloat x, GLfloat y, GLfloat z )
{
	VmathVector3 scale;
	vmathV3MakeFromElems(&scale, x, y, z);

	VmathMatrix4 result;
	vmathM4AppendScale(&result, _opengl_state.curr_mtx, &scale);
	vmathM4Copy(_opengl_state.curr_mtx, &result);
}

void glScaled( GLdouble x, GLdouble y, GLdouble z )
{
	glScalef((GLfloat)x, (GLfloat)y, (GLfloat)z);
}


void glTranslatef( GLfloat x, GLfloat y, GLfloat z )
{
	VmathVector3 translation;
	vmathV3MakeFromElems(&translation, x, y, z);
	
    VmathMatrix4 translationMatrix;
	vmathM4MakeIdentity(&translationMatrix);
    vmathM4MakeTranslation(&translationMatrix, &translation);

	VmathMatrix4 result;
	vmathM4Mul(&result, _opengl_state.curr_mtx, &translationMatrix);
	vmathM4Copy(_opengl_state.curr_mtx, &result);
}

void glTranslated( GLdouble x, GLdouble y, GLdouble z )
{
	glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z);
}


/*
 * Drawing Functions
 */
 
void _setup_draw_env(void);
void glBegin(GLenum mode)
{
	_setup_draw_env();
	rsxDrawVertexBegin(context, mode+1);
}

void glEnd(void)
{
	rsxDrawVertexEnd(context);
}

void glVertex2f(GLfloat x, GLfloat y)
{
	GLfloat v[2] = {x,y};
	rsxDrawVertex2f(context, GCM_VERTEX_ATTRIB_POS, v);
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	GLfloat v[3] = {x,y, z};
	rsxDrawVertex3f(context, GCM_VERTEX_ATTRIB_POS, v);
}

void glVertex3fv(const GLfloat *v)
{
	rsxDrawVertex3f(context, GCM_VERTEX_ATTRIB_POS, v);
}

void glColor3f( GLfloat red, GLfloat green, GLfloat blue )
{
	GLfloat v[3] = {red,green,blue};
	rsxDrawVertex3f(context, GCM_VERTEX_ATTRIB_COLOR0, v);
}

void glColor4f( GLfloat red, GLfloat green,
                                   GLfloat blue, GLfloat alpha )
{
	GLfloat v[4] = {red,green,blue,alpha};
	rsxDrawVertex4f(context, GCM_VERTEX_ATTRIB_COLOR0, v);
}


void glTexCoord2f(GLfloat s, GLfloat t)
{
	GLfloat v[2] = {s,t};
	rsxDrawVertex2f(context, GCM_VERTEX_ATTRIB_TEX0, v);
}

/*
 * Lighting
 */
 void glShadeModel( GLenum mode )
 {
	switch(mode)
	{
		case GL_FLAT:
			_opengl_state.shade_model = GCM_SHADE_MODEL_FLAT;
			break;
		case GL_SMOOTH:
			_opengl_state.shade_model = GCM_SHADE_MODEL_SMOOTH;
			break;
 	}
 }

/*
 * Texture mapping
 */

#if 0 // TODO
void glTexImage2D( GLenum target, GLint level,
                   GLint internalFormat,
                   GLsizei width, GLsizei height,
                   GLint border, GLenum format, GLenum type,
                   const GLvoid *pixels )
{
	if (_opengl_state.boundTexture == NULL)
		return;

	struct ps3gl_texture currentTexture = _opengl_state.boundTexture;
	currentTexture.width = width;
	currentTexture.height = height;
	if(internalFormat != 2 && internalFormat != 3 && internalFormat != 4) // InternalFormat isn't just the amount fo BPP
	{
		switch (internalFormat) {
			case GL_RGB: // There are no 24bpp textures in RSX
			case GL_RGBA:
				currentTexture.bpp = 4;
				break;
		}
	} else currentTexture.bpp = internalFormat;

	switch(format)
	{
		case GL_RGB:
			currentTexture.data = rsxMemalign(128, width*height*4);
			for(size_t i=0; i<width*height*4; i+=4) {
				currentTexture.data[i + 0] = 0xFF;
				currentTexture.data[i + 1] = *pixels++;
				currentTexture.data[i + 2] = *pixels++;
				currentTexture.data[i + 3] = *pixels++;
			}
			currentTexture.fmt = GCM_TEXTURE_FORMAT_D8R8G8B8|GCM_TEXTURE_FORMAT_LIN;
			currentTexture.hasAlpha = false;
			break;
		case GL_RGBA:
			currentTexture.data = rsxMemalign(128, width*height*4);
			for(size_t i=0; i<width*height*4; i+=4) {
				currentTexture.data[i + 1] = *pixels++;
				currentTexture.data[i + 2] = *pixels++;
				currentTexture.data[i + 3] = *pixels++;
				currentTexture.data[i + 0] = *pixels++;
			}
			currentTexture.fmt = GCM_TEXTURE_FORMAT_A8R8G8B8|GCM_TEXTURE_FORMAT_LIN;
			currentTexture.hasAlpha = true;
			break;
	}
}
#endif

void glGenTextures( GLsizei n, GLuint *textures )
{
	if(textures == NULL || n == 0)
		return;

	for(size_t i = 0; i < n; i++)
	{
		GLuint id = _opengl_state.nextTextureID++;
		textures[i] = id;
		if(id < MAX_TEXTURES)
		{
			_opengl_state.textures[i].id = id;
			_opengl_state.textures[i].allocated = true;
		}
	}
}

void glDeleteTextures( GLsizei n, const GLuint *textures);

void glBindTexture( GLenum target, GLuint texture )
{
	if (texture == 0) {
        _opengl_state.boundTexture = NULL;
        return;
    }

    if (texture < MAX_TEXTURES && !_opengl_state.textures[texture].allocated) {
        _opengl_state.textures[texture].target = target;
    }

    _opengl_state.boundTexture = &_opengl_state.textures[texture];
}


/* PS3GL Functions */

static void _program_exit_callback(void)
{
	gcmSetWaitFlip(context);
	rsxFinish(context,1);
}

void _setup_draw_env(void)
{
	rsxSetShadeModel(context, _opengl_state.shade_model);
	rsxSetColorMask(context, _opengl_state.color_mask);
	rsxSetColorMaskMrt(context,0);

	rsxSetDepthTestEnable(context, _opengl_state.depth_test);
	rsxSetDepthWriteEnable(context, _opengl_state.depth_mask);
	rsxSetDepthFunc(context, _opengl_state.depth_func);

	rsxSetViewport(context, 
		_opengl_state.viewport.x, 
		_opengl_state.viewport.y, 
		_opengl_state.viewport.w, 
		_opengl_state.viewport.h, 
		_opengl_state.depth_near,
		_opengl_state.depth_far, 
		_opengl_state.viewport.scale, 
		_opengl_state.viewport.offset);

	// TODO: Implement glScissor
	rsxSetScissor(context, _opengl_state.viewport.x, _opengl_state.viewport.y, _opengl_state.viewport.w, _opengl_state.viewport.h);

#if 0 // TODO
	_ps3gl_load_texture();
#endif

	rsxLoadVertexProgram(context,vpo,vp_ucode);
	rsxLoadFragmentProgramLocation(context,fpo,fp_offset,GCM_LOCATION_RSX);
	rsxSetVertexProgramParameter(context,vpo,_opengl_state.prog_consts[PS3GL_Uniform_ModelViewMatrix],(float*)&_opengl_state.modelview_matrix);
	rsxSetVertexProgramParameter(context,vpo,_opengl_state.prog_consts[PS3GL_Uniform_ProjectionMatrix],(float*)&_opengl_state.projection_matrix);

	rsxSetFragmentProgramParameter(context,fpo,_opengl_state.prog_consts[PS3GL_Uniform_TextureEnabled],(float*)&_opengl_state.texture0Enabled,fp_offset,GCM_LOCATION_RSX);
	rsxSetFragmentProgramParameter(context,fpo,	_opengl_state.prog_consts[PS3GL_Uniform_TextureMode],(float*)&_opengl_state.texEnvMode,fp_offset,GCM_LOCATION_RSX);
	rsxSetFragmentProgramParameter(context,fpo,	_opengl_state.prog_consts[PS3GL_Uniform_TextureHasAlpha],(float*)&_opengl_state.texture0HasAlpha,fp_offset,GCM_LOCATION_RSX);

	rsxSetFragmentProgramParameter(context,fpo,	_opengl_state.prog_consts[PS3GL_Uniform_FogEnabled],(float*)&_opengl_state.fog.enabled,fp_offset,GCM_LOCATION_RSX);
	rsxSetFragmentProgramParameter(context,fpo,	_opengl_state.prog_consts[PS3GL_Uniform_FogColor],(float*)&_opengl_state.fog.color,fp_offset,GCM_LOCATION_RSX);
}

// TODO: This is a placeholder, replace with good api, closer to vitaGL
// Also move rsxutil.c functionality over to here
void ps3glInit(void)
{
	atexit(_program_exit_callback);
	void *host_addr = memalign(1024*1024,HOST_SIZE);
	init_screen(host_addr,HOST_SIZE);

	u32 vpsize = 0;
	rsxVertexProgramGetUCode(vpo, &vp_ucode, &vpsize);
	_opengl_state.prog_consts[PS3GL_Uniform_ModelViewMatrix] = rsxVertexProgramGetConst(vpo, "uModelViewMatrix");
	_opengl_state.prog_consts[PS3GL_Uniform_ProjectionMatrix] = rsxVertexProgramGetConst(vpo, "uProjectionMatrix");

	u32 fpsize = 0;
	rsxFragmentProgramGetUCode(fpo, &fp_ucode, &fpsize);
	_opengl_state.texture0Unit = rsxFragmentProgramGetAttrib(fpo, "uTextureUnit0");
	_opengl_state.prog_consts[PS3GL_Uniform_TextureEnabled] = rsxFragmentProgramGetConst(fpo, "uTextureEnabled");
	_opengl_state.prog_consts[PS3GL_Uniform_TextureMode] = rsxFragmentProgramGetConst(fpo, "uTextureMode");

	_opengl_state.prog_consts[PS3GL_Uniform_TextureHasAlpha] = rsxFragmentProgramGetConst(fpo, "uTextureHasAlpha");
	_opengl_state.prog_consts[PS3GL_Uniform_FogEnabled] = rsxFragmentProgramGetConst(fpo, "uFogEnabled");
	_opengl_state.prog_consts[PS3GL_Uniform_FogColor] = rsxFragmentProgramGetConst(fpo, "uFogColor");

	fp_buffer = (u32*)rsxMemalign(64,fpsize);
	memcpy(fp_buffer,fp_ucode,fpsize);
	rsxAddressToOffset(fp_buffer,&fp_offset);

	// Set default state
	glColor3f(1.0f, 1.0f, 1.0f);
	glColorMask(true, true, true, true);
	glClearColor(0, 0, 0, 0);
	
	glDepthMask(true);
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthRange(0, 1);
	glClearDepth(0);

	// Clear Values
	_opengl_state.clear_stencil = 0;
	//glClearStencil(0);

	// Matrices
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Lighting
	glShadeModel(GL_SMOOTH);

	// Textures
	_opengl_state.textures[0].id = 0;
	_opengl_state.textures[0].allocated = true;
	_opengl_state.textures[0].data = NULL;
	_opengl_state.textures[0].width = 0;
	_opengl_state.textures[0].height = 0;
	_opengl_state.textures[0].bpp = 0;
	_opengl_state.textures[0].hasAlpha = false;
}

void ps3glSwapBuffers(void)
{
	flip();
}

void glFogi( GLenum pname, GLint param )
{
	switch(pname)
	{
		case GL_FOG_MODE:
			_opengl_state.fog.mode = param;
			break;
		case GL_FOG_START:
		case GL_FOG_END:
		case GL_FOG_DENSITY:
			glFogf(pname, (GLfloat)param);
			break;
		default:
			break;
	}
}

void glFogf( GLenum pname, GLfloat param )
{

	switch(pname)
	{
		case GL_FOG_MODE:
			glFogi(pname, (GLint)param);
			break;
		case GL_FOG_START:
			_opengl_state.fog.start = param;
			break;
		case GL_FOG_END:
			_opengl_state.fog.start = param;
			break;
		case GL_FOG_DENSITY:
			_opengl_state.fog.density = param;
			break;
		default:
			break;
	}
}

void glFogfv( GLenum pname, const GLfloat *params )
{
	if(pname == GL_FOG_COLOR)
	{
		if(params != NULL)
			memcpy(_opengl_state.fog.color, params, 4*sizeof(GLfloat));
	}
}


