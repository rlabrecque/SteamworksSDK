#include "stdafx.h"


#include <GL/glew.h>

#include "steamvrglhelper.h"


using vr::HmdMatrix44_t;
using vr::HmdMatrix34_t;


CSteamVRGLHelper::CSteamVRGLHelper()
{
	m_UIRenderTarget = 0;
	m_UIRenderTargetTexture = 0;

	m_PredistortRenderTarget = 0;
	m_PredistortRenderTargetDepth = 0;
	m_PredistortRenderTargetTexture = 0;

	m_bDistortionInitialized = false;
	m_rgflQuadData = NULL;
	m_rgflQuadColorData = NULL;
	m_rgflQuadTextureData = NULL;
}

CSteamVRGLHelper::~CSteamVRGLHelper()
{
}

bool CSteamVRGLHelper::BInit( vr::IHmd *pHmd )
{
	m_pHmd = pHmd;

	return true;
}

void CSteamVRGLHelper::Shutdown()
{
	if( m_UIRenderTarget != 0 )
	{
		glDeleteTextures( 1, &m_UIRenderTargetTexture );
		glDeleteFramebuffers( 1, &m_UIRenderTarget );

		m_UIRenderTarget = 0;
		m_UIRenderTargetTexture = 0;
	}

	if( m_PredistortRenderTarget != 0 )
	{
		glDeleteTextures( 1, &m_PredistortRenderTargetTexture );
		glDeleteFramebuffers( 1, &m_PredistortRenderTargetDepth );
		glDeleteFramebuffers( 1, &m_PredistortRenderTarget );

		m_PredistortRenderTargetDepth = 0;
		m_PredistortRenderTargetTexture = 0;
		m_PredistortRenderTarget = 0;
	}

	if( m_bDistortionInitialized )
	{
		glDeleteProgram( m_DistortShader );
		glDeleteTextures( 2, m_DistortTexture );

		delete [] m_rgflQuadData;
		delete [] m_rgflQuadColorData;
		delete [] m_rgflQuadTextureData;
		m_rgflQuadData = NULL;
		m_rgflQuadColorData = NULL;
		m_rgflQuadTextureData = NULL;
	}
}

bool CSteamVRGLHelper::InitUIRenderTarget( uint32_t unWidth, uint32_t unHeight )
{
	if( m_UIRenderTarget != 0 )
		return false;

	// create the frame buffer itself
	glGenFramebuffers(1, &m_UIRenderTarget);
	glBindFramebuffer(GL_FRAMEBUFFER, m_UIRenderTarget);

	// The texture we're going to render to
	glGenTextures(1, &m_UIRenderTargetTexture);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, m_UIRenderTargetTexture);

	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, unWidth, unHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

	// Set "renderedTexture" as our colour attachement #0
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_UIRenderTargetTexture, 0 );

	// Set the list of draw buffers.
	GLenum rDrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(ARRAYSIZE( rDrawBuffers ), rDrawBuffers);

	m_unUIWidth = unWidth;
	m_unUIHeight = unHeight;

	return true;
}

bool CSteamVRGLHelper::BPushUIRenderTarget()
{
	if( m_UIRenderTarget == 0 )
		return false;

	glBindFramebuffer(GL_FRAMEBUFFER, m_UIRenderTarget);
	glClearColor( 0, 0, 0, (float)1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	return true;
}

void HmdMatrix_TransposeInPlace( HmdMatrix44_t & mat )
{
	for( int x=0; x<3; x++ )
	{
		for( int y= x+1 ; y<4; y++ )
		{
			float tmp = mat.m[x][y];
			mat.m[x][y] = mat.m[y][x];
			mat.m[y][x] = tmp;
		}
	}
}

void CSteamVRGLHelper::PushViewAndProjection( vr::Hmd_Eye eye, float fNearZ, float fFarZ, const vr::HmdMatrix34_t & matWorldFromHeadPose )
{
	HmdMatrix44_t matProjection = m_pHmd->GetProjectionMatrix( eye, fNearZ, fFarZ, vr::API_OpenGL );

	// Perspective
	glMatrixMode(GL_PROJECTION);
	HmdMatrix_TransposeInPlace( matProjection );
	glLoadMatrixf( &matProjection.m[0][0] );

	// to get the view matrix, we first need to turn the pose into a 4x4
	vr::HmdMatrix44_t matWorldFromHeadPose44;
	for( int y=0; y<4; y++ )
	{
		for( int x=0; x<3; x++ )
			matWorldFromHeadPose44.m[x][y] = matWorldFromHeadPose.m[x][y];
		matWorldFromHeadPose44.m[3][y] = 0;
	}
	matWorldFromHeadPose44.m[3][3] = 1.f;

	// View matrix has changed as well
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf( &matWorldFromHeadPose44.m[0][0] );
	HmdMatrix34_t matEye = m_pHmd->GetHeadFromEyePose( eye );
	glMultMatrixf( &matEye.m[0][0] );

}

bool CSteamVRGLHelper::BInitPredistortRenderTarget()
{
	if( m_PredistortRenderTarget != 0 )
		return false;

	m_pHmd->GetRecommendedRenderTargetSize( &m_unPredistortWidth, &m_unPredistortHeight );

	// create the frame buffer itself
	glGenFramebuffers(1, &m_PredistortRenderTarget);
	glBindFramebuffer(GL_FRAMEBUFFER, m_PredistortRenderTarget);

	// The texture we're going to render to
	glGenTextures(1, &m_PredistortRenderTargetTexture);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, m_PredistortRenderTargetTexture);

	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_unPredistortWidth, m_unPredistortHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

	// The depth buffer
	glGenRenderbuffers(1, &m_PredistortRenderTargetDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, m_PredistortRenderTargetDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_unPredistortWidth, m_unPredistortHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_PredistortRenderTargetDepth);

	// Set "renderedTexture" as our colour attachement #0
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_PredistortRenderTargetTexture, 0 );

	// Set the list of draw buffers.
	GLenum rDrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(ARRAYSIZE( rDrawBuffers ), rDrawBuffers);
}


void CSteamVRGLHelper::PushPredistortRenderTarget()
{
	if( m_PredistortRenderTarget == 0 )
		return;

	glBindFramebuffer(GL_FRAMEBUFFER, m_PredistortRenderTarget);

	glClearColor( 0.1f, 0.1f, 0.1f, (float)1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	glViewport( 0, 0, m_unPredistortWidth, m_unPredistortHeight );
	glScissor( 0, 0, m_unPredistortWidth, m_unPredistortHeight );

	return;
}

static const char* const g_pchVRFragmentShader = "GLOverlay_v0.frag";

static GLuint CreateShader(const char* pchFragmentShader)
{
	char chFragmentShaderInfoLog[1024];
	char chProgramInfoLog[1024];
	chFragmentShaderInfoLog[0] = 0;
	chProgramInfoLog[0] = 0;
	GLuint nProgram = glCreateProgram();
	GLuint nFragmentShader = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( nFragmentShader, 1, &pchFragmentShader, NULL );
	glCompileShader( nFragmentShader );
	GLint nFragmentShaderCompiled;
	glGetShaderiv( nFragmentShader, GL_COMPILE_STATUS, &nFragmentShaderCompiled );
	glGetShaderInfoLog( nFragmentShader, sizeof( chFragmentShaderInfoLog ), NULL, chFragmentShaderInfoLog );
	glAttachShader( nProgram, nFragmentShader );
	glDeleteShader( nFragmentShader );
	glLinkProgram( nProgram );
	GLint nProgramLinked;
	glGetProgramiv( nProgram, GL_LINK_STATUS, &nProgramLinked );
	glGetProgramInfoLog( nProgram, sizeof( chProgramInfoLog ), NULL, chProgramInfoLog );
	if ( chFragmentShaderInfoLog[0] )
	{
		fprintf( stderr, "GLSL Fragment Shader log:\n" );
		fprintf( stderr, "%s\n", chFragmentShaderInfoLog );
	}
	if ( chProgramInfoLog[0] )
	{
		fprintf( stderr, "GLSL Program log:\n" );
		fprintf( stderr, "%s\n", chProgramInfoLog );
	}
	return nProgram;
}

// Quick and dirty wrapper for working with UV coordinates.
template<typename T>
struct TUV
{
	T x, y;
	TUV() {}
	TUV(T _x, T _y)
		: x(_x), y(_y) {}
	TUV(const float a[2])
		: x(a[0]), y(a[1]) {}
	TUV(const TUV& other)
		: x(other.x), y(other.y) {}
	inline TUV operator*(T scale) const
		{ return TUV(x * scale, y * scale); }
	inline TUV operator+(const TUV& other) const
		{ return TUV(x + other.x, y + other.y); }
	inline TUV operator-(const TUV& other) const
		{ return TUV(x - other.x, y - other.y); }
};

template<typename T>
inline TUV<T> operator*(T scale, const TUV<T> v)
{
	return TUV<T>(scale * v.x, scale * v.y);
}

typedef TUV<float> UVf;
typedef TUV<double> UVd;

template<typename T>
void PopulateDistortionMap( vr::IHmd *pHmd, vr::Hmd_Eye eye, int width, int height, T* data, int elem_scale = (T)(-1) )
{
	const int elem_count = 4;

	for ( int yi = 0; yi < height; yi++ )
	{
		const float v = ( yi + 0.5f ) / height;
		for ( int xi = 0; xi < width; xi++ )
		{
			const float u = ( xi + 0.5f ) / width;

			vr::DistortionCoordinates_t coords = pHmd->ComputeDistortion(eye, u, v);
			UVf samp_red(coords.rfRed);
			UVf samp_green(coords.rfGreen);
			UVf samp_blue(coords.rfBlue);

			static const float rg_to_rb_ratio = 0.522f;
			UVf red_to_blue = samp_blue - samp_red;
			UVf tex_samp_red = samp_green - rg_to_rb_ratio * red_to_blue;
			UVf tex_samp_blue = samp_green + (1-rg_to_rb_ratio) * red_to_blue;

			if ( tex_samp_red.x < 0.0 || tex_samp_blue.x < 0.0 )
			{
				tex_samp_red.x = tex_samp_blue.x = 0.0;
			}

			if ( tex_samp_red.x > 1.0 || tex_samp_blue.x > 1.0 )
			{
				tex_samp_red.x = tex_samp_blue.x = 1.0;
			}

			if ( tex_samp_red.y < 0.0 || tex_samp_blue.y < 0.0 )
			{
				tex_samp_red.y = tex_samp_blue.y = 0.0;
			}

			if ( tex_samp_red.y > 1.0 || tex_samp_blue.y > 1.0 )
			{
				tex_samp_red.y = tex_samp_blue.y = 1.0;
			}

			int idx = yi * width + xi;

			data[ idx * elem_count + 0 ] = (T)(tex_samp_red.x * elem_scale + 0.5f);
			data[ idx * elem_count + 1 ] = (T)(tex_samp_red.y * elem_scale + 0.5f);
			data[ idx * elem_count + 2 ] = (T)(tex_samp_blue.x * elem_scale + 0.5f);
			data[ idx * elem_count + 3 ] = (T)(tex_samp_blue.y * elem_scale + 0.5f);
		}
	}
}


void CSteamVRGLHelper::InitDistortion()
{
	if( m_bDistortionInitialized )
		return;

	// init the quad
	uint32_t x, y, w, h;
	m_pHmd->GetEyeOutputViewport( vr::Eye_Left, &x, &y, &w, &h );

	m_rgflQuadData = new GLfloat[ 4 * 3];
	m_rgflQuadColorData = new GLubyte[ 4 * 4 ];
	m_rgflQuadTextureData = new GLfloat[ 4 * 2 ];

	m_rgflQuadData[0 + 0] = 0;
	m_rgflQuadData[0 + 1] = h;
	m_rgflQuadData[0 + 2] = 1;

	m_rgflQuadTextureData[0 + 0] = 0;
	m_rgflQuadTextureData[0 + 1] = 0;

	m_rgflQuadData[3 + 0] = w;
	m_rgflQuadData[3 + 1] = h;
	m_rgflQuadData[3 + 2] = 1;

	m_rgflQuadTextureData[2 + 0] = 1.f;
	m_rgflQuadTextureData[2 + 1] = 0;

	m_rgflQuadData[6 + 0] = w;
	m_rgflQuadData[6 + 1] = 0;
	m_rgflQuadData[6 + 2] = 1;

	m_rgflQuadTextureData[4 + 0] = 1.f;
	m_rgflQuadTextureData[4 + 1] = 1.f;

	m_rgflQuadData[9 + 0] = 0;
	m_rgflQuadData[9 + 1] = 0;
	m_rgflQuadData[9 + 2] = 1;

	m_rgflQuadTextureData[6 + 0] = 0;
	m_rgflQuadTextureData[6 + 1] = 1.f;

	memset( m_rgflQuadColorData, 0xFFFFFFFF, sizeof( GLubyte ) * 4 * 4 );

	// init the distortion textures
	const uint32 unDistortMapSize = 128;
	uint16* pData = new uint16[unDistortMapSize * unDistortMapSize * 4];
	for( int i=0; i<2; i++ )
	{
		vr::Hmd_Eye eye = (vr::Hmd_Eye)i;

		PopulateDistortionMap( m_pHmd, eye, unDistortMapSize, unDistortMapSize, pData);

		glGenTextures( 1, &m_DistortTexture[ i ] );
		glBindTexture( GL_TEXTURE_2D, m_DistortTexture[ i ] );

		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0 );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS_EXT, 0.0 );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA16, unDistortMapSize, unDistortMapSize, 0, GL_RGBA, GL_UNSIGNED_SHORT, (void *)pData );
	}
	delete [] pData;

	// init the distortion shader
	FILE *fileShader = fopen( "shaders/GLVRDistort.glsl", "r" );
	if( !fileShader )
	{
		fprintf( stderr, "Unable to load saders/GLVRDistort.glsl for VR distortion correction\n" );
	}
	else
	{
		fseek( fileShader, 0, SEEK_END );
		long size = ftell( fileShader );
		fseek( fileShader, 0, SEEK_SET );

		char *pchFile = new char[size+1];
		fread( pchFile, size, 1, fileShader );
		pchFile[ size ] = 0;


		m_DistortShader = CreateShader( pchFile );

		m_DistortColorSampler = glGetUniformLocation( m_DistortShader, "BaseTextureSampler" );
		m_DistortMapSampler = glGetUniformLocation( m_DistortShader, "DistortMapTextureSampler" );

		delete [] pchFile;
	}

	// don't do this again
	m_bDistortionInitialized = true;
}


void CSteamVRGLHelper::DrawPredistortToFrameBuffer( vr::Hmd_Eye eye )
{
	InitDistortion();

	// set the output to the frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0 );

	// set the viewport and scissor to the viewport provided by Steam VR
	uint32_t x, y, w, h;
	m_pHmd->GetEyeOutputViewport( eye, &x, &y, &w, &h );
	y=0;
	glViewport( x, y, w, h );
	glScissor( x, y, w, h );

	// get the window bounds to use to build the projection
	int32_t xWindow, yWindow;
	uint32_t wWindow, hWindow;
	m_pHmd->GetWindowBounds( &xWindow, &yWindow, &wWindow, &hWindow );

	// nice 2D projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho( 0, w, h, 0, -1.0f, 1.0f );
	glTranslatef( 0, 0, 0 );

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	// bind the predistort and distortion map textures
	glEnable( GL_TEXTURE_2D );

	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, m_DistortTexture[ eye ] );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, m_PredistortRenderTargetTexture );

	glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	GLint nOldProgram = 0;
	glGetIntegerv( GL_CURRENT_PROGRAM, &nOldProgram );

	glUseProgram( m_DistortShader );

	glUniform1i( m_DistortColorSampler, 0 );
	glUniform1i( m_DistortMapSampler, 1 );

	glColorPointer( 4, GL_UNSIGNED_BYTE, 0, m_rgflQuadColorData );
	glVertexPointer( 3, GL_FLOAT, 0, m_rgflQuadData );
	glTexCoordPointer( 2, GL_FLOAT, 0, m_rgflQuadTextureData );
	glDrawArrays( GL_QUADS, 0, 4 );

	glDisable( GL_TEXTURE_2D );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );

	glUseProgram( nOldProgram );
}

