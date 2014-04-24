#ifndef STEAMVRGLHELPER_H
#define STEAMVRGLHELPER_H

#include <stdint.h>

#include "SDL_opengl.h"

#include "steam/steamvr.h"

class CSteamVRGLHelper
{
public:
	CSteamVRGLHelper();
	~CSteamVRGLHelper();

	bool BInit( vr::IHmd *pHmd );
	void Shutdown();
	bool InitUIRenderTarget( uint32_t unWidth, uint32_t unHeight );
	bool BPushUIRenderTarget();
	GLuint GetUITexture() const { return m_UIRenderTargetTexture; }

	void PushViewAndProjection( vr::Hmd_Eye eye, float fNearZ, float fFarZ, const vr::HmdMatrix34_t & matWorldFromheadPose );

	bool BInitPredistortRenderTarget();
	void PushPredistortRenderTarget();
	float GetAspectRatio() const { return (float)m_unPredistortWidth / (float)m_unPredistortHeight; }
	void DrawPredistortToFrameBuffer( vr::Hmd_Eye eye );

private:
	void InitDistortion();

	// for rendering UI
	GLuint m_UIRenderTarget;
	GLuint m_UIRenderTargetTexture;
	uint32_t m_unUIWidth;
	uint32_t m_unUIHeight;

	// for rendering the pre-distortion scene into
	GLuint m_PredistortRenderTarget;
	GLuint m_PredistortRenderTargetDepth;
	GLuint m_PredistortRenderTargetTexture;
	uint32_t m_unPredistortWidth;
	uint32_t m_unPredistortHeight;

	// Used in the distortion pass
	bool m_bDistortionInitialized;
	GLuint m_DistortShader;
	GLuint m_DistortTexture[2];
	GLuint m_DistortColorSampler;
	GLuint m_DistortMapSampler;
	GLfloat *m_rgflQuadData;
	GLubyte *m_rgflQuadColorData;
	GLfloat *m_rgflQuadTextureData;

	vr::IHmd *m_pHmd;
};

#endif // STEAMVRGLHELPER_H
