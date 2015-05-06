//=== === Copyright 1996-2014, Valve Corporation, All rights reserved. =======
//
// Purpose: This file contains C#/managed code bindings for the SteamAPI interfaces
// This file is auto-generated, do not edit it.
//
//=============================================================================

using System;
using System.Runtime.InteropServices;
using Valve.Steamworks;
using Valve.Interop;



namespace Valve.VR
{

	public abstract class IVRSystem
	{
		public abstract IntPtr GetIntPtr();
		public abstract void GetWindowBounds(ref int pnX,ref int pnY,ref uint pnWidth,ref uint pnHeight);
		public abstract void GetRecommendedRenderTargetSize(ref uint pnWidth,ref uint pnHeight);
		public abstract void GetEyeOutputViewport(Hmd_Eye eEye,ref uint pnX,ref uint pnY,ref uint pnWidth,ref uint pnHeight);
		public abstract HmdMatrix44_t GetProjectionMatrix(Hmd_Eye eEye,float fNearZ,float fFarZ,GraphicsAPIConvention eProjType);
		public abstract void GetProjectionRaw(Hmd_Eye eEye,ref float pfLeft,ref float pfRight,ref float pfTop,ref float pfBottom);
		public abstract DistortionCoordinates_t ComputeDistortion(Hmd_Eye eEye,float fU,float fV);
		public abstract HmdMatrix34_t GetEyeToHeadTransform(Hmd_Eye eEye);
		public abstract bool GetTimeSinceLastVsync(ref float pfSecondsSinceLastVsync,ref ulong pulFrameCounter);
		public abstract int GetD3D9AdapterIndex();
		public abstract void GetDXGIOutputInfo(ref int pnAdapterIndex,ref int pnAdapterOutputIndex);
		public abstract void AttachToWindow(IntPtr hWnd);
		public abstract void GetDeviceToAbsoluteTrackingPose(TrackingUniverseOrigin eOrigin,float fPredictedSecondsToPhotonsFromNow,TrackedDevicePose_t [] pTrackedDevicePoseArray);
		public abstract void ResetSeatedZeroPose();
		public abstract HmdMatrix34_t GetSeatedZeroPoseToStandingAbsoluteTrackingPose();
		public abstract bool LoadRenderModel(string pchRenderModelName,ref RenderModel_t pRenderModel);
		public abstract void FreeRenderModel(ref RenderModel_t pRenderModel);
		public abstract TrackedDeviceClass GetTrackedDeviceClass(TrackedDeviceIndex_t unDeviceIndex);
		public abstract bool IsTrackedDeviceConnected(TrackedDeviceIndex_t unDeviceIndex);
		public abstract bool GetBoolTrackedDeviceProperty(TrackedDeviceIndex_t unDeviceIndex,TrackedDeviceProperty prop,ref TrackedPropertyError pError);
		public abstract float GetFloatTrackedDeviceProperty(TrackedDeviceIndex_t unDeviceIndex,TrackedDeviceProperty prop,ref TrackedPropertyError pError);
		public abstract int GetInt32TrackedDeviceProperty(TrackedDeviceIndex_t unDeviceIndex,TrackedDeviceProperty prop,ref TrackedPropertyError pError);
		public abstract ulong GetUint64TrackedDeviceProperty(TrackedDeviceIndex_t unDeviceIndex,TrackedDeviceProperty prop,ref TrackedPropertyError pError);
		public abstract HmdMatrix34_t GetMatrix34TrackedDeviceProperty(TrackedDeviceIndex_t unDeviceIndex,TrackedDeviceProperty prop,ref TrackedPropertyError pError);
		public abstract uint GetStringTrackedDeviceProperty(TrackedDeviceIndex_t unDeviceIndex,TrackedDeviceProperty prop,System.Text.StringBuilder pchValue,uint unBufferSize,ref TrackedPropertyError pError);
		public abstract string GetPropErrorNameFromEnum(TrackedPropertyError error);
		public abstract bool PollNextEvent(ref VREvent_t pEvent);
		public abstract string GetEventTypeNameFromEnum(uint eType);
		public abstract uint GetHiddenAreaMesh(Hmd_Eye eEye);
	}


	public abstract class IVRChaperone
	{
		public abstract IntPtr GetIntPtr();
		public abstract ChaperoneCalibrationState GetCalibrationState();
		public abstract bool GetSoftBoundsInfo(ref ChaperoneSoftBoundsInfo_t pInfo);
		public abstract bool GetHardBoundsInfo(out HmdQuad_t [] pQuadsBuffer);
		public abstract bool GetSeatedBoundsInfo(ref ChaperoneSeatedBoundsInfo_t pInfo);
	}


	public abstract class IVRCompositor
	{
		public abstract IntPtr GetIntPtr();
		public abstract uint GetLastError(System.Text.StringBuilder pchBuffer,uint unBufferSize);
		public abstract void SetVSync(bool bVSync);
		public abstract bool GetVSync();
		public abstract void SetGamma(float fGamma);
		public abstract float GetGamma();
		public abstract void SetGraphicsDevice(Compositor_DeviceType eType,IntPtr pDevice);
		public abstract void WaitGetPoses(TrackedDevicePose_t [] pPoseArray);
		public abstract void Submit(Hmd_Eye eEye,IntPtr pTexture,ref Compositor_TextureBounds pBounds);
		public abstract void ClearLastSubmittedFrame();
		public abstract void GetOverlayDefaults(ref Compositor_OverlaySettings pSettings);
		public abstract void SetOverlay(IntPtr pTexture,ref Compositor_OverlaySettings pSettings);
		public abstract void SetOverlayRaw(IntPtr buffer,uint width,uint height,uint depth,ref Compositor_OverlaySettings pSettings);
		public abstract void SetOverlayFromFile(string pchFilePath,ref Compositor_OverlaySettings pSettings);
		public abstract void ClearOverlay();
		public abstract bool GetFrameTiming(ref Compositor_FrameTiming pTiming,uint unFramesAgo);
		public abstract void FadeToColor(float fSeconds,float fRed,float fGreen,float fBlue,float fAlpha,bool bBackground);
		public abstract void FadeGrid(float fSeconds,bool bFadeIn);
		public abstract void CompositorBringToFront();
		public abstract void CompositorGoToBack();
		public abstract void CompositorQuit();
		public abstract bool IsFullscreen();
	}


	public abstract class IVRControlPanel
	{
		public abstract IntPtr GetIntPtr();
		public abstract uint GetDriverCount();
		public abstract uint GetDriverId(uint unDriverIndex,string pchBuffer,uint unBufferLen);
		public abstract uint GetDriverDisplayCount(string pchDriverId);
		public abstract uint GetDriverDisplayId(string pchDriverId,uint unDisplayIndex,string pchBuffer,uint unBufferLen);
		public abstract uint GetDriverDisplayModelNumber(string pchDriverId,string pchDisplayId,string pchBuffer,uint unBufferLen);
		public abstract uint GetDriverDisplaySerialNumber(string pchDriverId,string pchDisplayId,string pchBuffer,uint unBufferLen);
		public abstract uint LoadSharedResource(string pchResourceName,string pchBuffer,uint unBufferLen);
		public abstract float GetIPD();
		public abstract void SetIPD(float fIPD);
	}


public class CVRSystem : IVRSystem
{
public CVRSystem(IntPtr VRSystem)
{
	m_pVRSystem = VRSystem;
}
IntPtr m_pVRSystem;

public override IntPtr GetIntPtr() { return m_pVRSystem; }

private void CheckIfUsable()
{
	if (m_pVRSystem == IntPtr.Zero)
	{
		throw new Exception("Steam Pointer not configured");
	}
}
public override void GetWindowBounds(ref int pnX,ref int pnY,ref uint pnWidth,ref uint pnHeight)
{
	CheckIfUsable();
	pnX = 0;
	pnY = 0;
	pnWidth = 0;
	pnHeight = 0;
	NativeEntrypoints.SteamAPI_vr_IVRSystem_GetWindowBounds(m_pVRSystem,ref pnX,ref pnY,ref pnWidth,ref pnHeight);
}
public override void GetRecommendedRenderTargetSize(ref uint pnWidth,ref uint pnHeight)
{
	CheckIfUsable();
	pnWidth = 0;
	pnHeight = 0;
	NativeEntrypoints.SteamAPI_vr_IVRSystem_GetRecommendedRenderTargetSize(m_pVRSystem,ref pnWidth,ref pnHeight);
}
public override void GetEyeOutputViewport(Hmd_Eye eEye,ref uint pnX,ref uint pnY,ref uint pnWidth,ref uint pnHeight)
{
	CheckIfUsable();
	pnX = 0;
	pnY = 0;
	pnWidth = 0;
	pnHeight = 0;
	NativeEntrypoints.SteamAPI_vr_IVRSystem_GetEyeOutputViewport(m_pVRSystem,eEye,ref pnX,ref pnY,ref pnWidth,ref pnHeight);
}
public override HmdMatrix44_t GetProjectionMatrix(Hmd_Eye eEye,float fNearZ,float fFarZ,GraphicsAPIConvention eProjType)
{
	CheckIfUsable();
	HmdMatrix44_t result = NativeEntrypoints.SteamAPI_vr_IVRSystem_GetProjectionMatrix(m_pVRSystem,eEye,fNearZ,fFarZ,eProjType);
	return result;
}
public override void GetProjectionRaw(Hmd_Eye eEye,ref float pfLeft,ref float pfRight,ref float pfTop,ref float pfBottom)
{
	CheckIfUsable();
	pfLeft = 0;
	pfRight = 0;
	pfTop = 0;
	pfBottom = 0;
	NativeEntrypoints.SteamAPI_vr_IVRSystem_GetProjectionRaw(m_pVRSystem,eEye,ref pfLeft,ref pfRight,ref pfTop,ref pfBottom);
}
public override DistortionCoordinates_t ComputeDistortion(Hmd_Eye eEye,float fU,float fV)
{
	CheckIfUsable();
	DistortionCoordinates_t result = NativeEntrypoints.SteamAPI_vr_IVRSystem_ComputeDistortion(m_pVRSystem,eEye,fU,fV);
	return result;
}
public override HmdMatrix34_t GetEyeToHeadTransform(Hmd_Eye eEye)
{
	CheckIfUsable();
	HmdMatrix34_t result = NativeEntrypoints.SteamAPI_vr_IVRSystem_GetEyeToHeadTransform(m_pVRSystem,eEye);
	return result;
}
public override bool GetTimeSinceLastVsync(ref float pfSecondsSinceLastVsync,ref ulong pulFrameCounter)
{
	CheckIfUsable();
	pfSecondsSinceLastVsync = 0;
	pulFrameCounter = 0;
	bool result = NativeEntrypoints.SteamAPI_vr_IVRSystem_GetTimeSinceLastVsync(m_pVRSystem,ref pfSecondsSinceLastVsync,ref pulFrameCounter);
	return result;
}
public override int GetD3D9AdapterIndex()
{
	CheckIfUsable();
	int result = NativeEntrypoints.SteamAPI_vr_IVRSystem_GetD3D9AdapterIndex(m_pVRSystem);
	return result;
}
public override void GetDXGIOutputInfo(ref int pnAdapterIndex,ref int pnAdapterOutputIndex)
{
	CheckIfUsable();
	pnAdapterIndex = 0;
	pnAdapterOutputIndex = 0;
	NativeEntrypoints.SteamAPI_vr_IVRSystem_GetDXGIOutputInfo(m_pVRSystem,ref pnAdapterIndex,ref pnAdapterOutputIndex);
}
public override void AttachToWindow(IntPtr hWnd)
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRSystem_AttachToWindow(m_pVRSystem,hWnd);
}
public override void GetDeviceToAbsoluteTrackingPose(TrackingUniverseOrigin eOrigin,float fPredictedSecondsToPhotonsFromNow,TrackedDevicePose_t [] pTrackedDevicePoseArray)
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRSystem_GetDeviceToAbsoluteTrackingPose(m_pVRSystem,eOrigin,fPredictedSecondsToPhotonsFromNow,pTrackedDevicePoseArray,(uint) pTrackedDevicePoseArray.Length);
}
public override void ResetSeatedZeroPose()
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRSystem_ResetSeatedZeroPose(m_pVRSystem);
}
public override HmdMatrix34_t GetSeatedZeroPoseToStandingAbsoluteTrackingPose()
{
	CheckIfUsable();
	HmdMatrix34_t result = NativeEntrypoints.SteamAPI_vr_IVRSystem_GetSeatedZeroPoseToStandingAbsoluteTrackingPose(m_pVRSystem);
	return result;
}
public override bool LoadRenderModel(string pchRenderModelName,ref RenderModel_t pRenderModel)
{
	CheckIfUsable();
	bool result = NativeEntrypoints.SteamAPI_vr_IVRSystem_LoadRenderModel(m_pVRSystem,pchRenderModelName,ref pRenderModel);
	return result;
}
public override void FreeRenderModel(ref RenderModel_t pRenderModel)
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRSystem_FreeRenderModel(m_pVRSystem,ref pRenderModel);
}
public override TrackedDeviceClass GetTrackedDeviceClass(TrackedDeviceIndex_t unDeviceIndex)
{
	CheckIfUsable();
	TrackedDeviceClass result = NativeEntrypoints.SteamAPI_vr_IVRSystem_GetTrackedDeviceClass(m_pVRSystem,unDeviceIndex);
	return result;
}
public override bool IsTrackedDeviceConnected(TrackedDeviceIndex_t unDeviceIndex)
{
	CheckIfUsable();
	bool result = NativeEntrypoints.SteamAPI_vr_IVRSystem_IsTrackedDeviceConnected(m_pVRSystem,unDeviceIndex);
	return result;
}
public override bool GetBoolTrackedDeviceProperty(TrackedDeviceIndex_t unDeviceIndex,TrackedDeviceProperty prop,ref TrackedPropertyError pError)
{
	CheckIfUsable();
	bool result = NativeEntrypoints.SteamAPI_vr_IVRSystem_GetBoolTrackedDeviceProperty(m_pVRSystem,unDeviceIndex,prop,ref pError);
	return result;
}
public override float GetFloatTrackedDeviceProperty(TrackedDeviceIndex_t unDeviceIndex,TrackedDeviceProperty prop,ref TrackedPropertyError pError)
{
	CheckIfUsable();
	float result = NativeEntrypoints.SteamAPI_vr_IVRSystem_GetFloatTrackedDeviceProperty(m_pVRSystem,unDeviceIndex,prop,ref pError);
	return result;
}
public override int GetInt32TrackedDeviceProperty(TrackedDeviceIndex_t unDeviceIndex,TrackedDeviceProperty prop,ref TrackedPropertyError pError)
{
	CheckIfUsable();
	int result = NativeEntrypoints.SteamAPI_vr_IVRSystem_GetInt32TrackedDeviceProperty(m_pVRSystem,unDeviceIndex,prop,ref pError);
	return result;
}
public override ulong GetUint64TrackedDeviceProperty(TrackedDeviceIndex_t unDeviceIndex,TrackedDeviceProperty prop,ref TrackedPropertyError pError)
{
	CheckIfUsable();
	ulong result = NativeEntrypoints.SteamAPI_vr_IVRSystem_GetUint64TrackedDeviceProperty(m_pVRSystem,unDeviceIndex,prop,ref pError);
	return result;
}
public override HmdMatrix34_t GetMatrix34TrackedDeviceProperty(TrackedDeviceIndex_t unDeviceIndex,TrackedDeviceProperty prop,ref TrackedPropertyError pError)
{
	CheckIfUsable();
	HmdMatrix34_t result = NativeEntrypoints.SteamAPI_vr_IVRSystem_GetMatrix34TrackedDeviceProperty(m_pVRSystem,unDeviceIndex,prop,ref pError);
	return result;
}
public override uint GetStringTrackedDeviceProperty(TrackedDeviceIndex_t unDeviceIndex,TrackedDeviceProperty prop,System.Text.StringBuffer pchValue,uint unBufferSize,ref TrackedPropertyError pError)
{
	CheckIfUsable();
	uint result = NativeEntrypoints.SteamAPI_vr_IVRSystem_GetStringTrackedDeviceProperty(m_pVRSystem,unDeviceIndex,prop,pchValue,unBufferSize,ref pError);
	return result;
}
public override string GetPropErrorNameFromEnum(TrackedPropertyError error)
{
	CheckIfUsable();
	IntPtr result = NativeEntrypoints.SteamAPI_vr_IVRSystem_GetPropErrorNameFromEnum(m_pVRSystem,error);
	return (string) Marshal.PtrToStructure(result, typeof(string));
}
public override bool PollNextEvent(ref VREvent_t pEvent)
{
	CheckIfUsable();
	bool result = NativeEntrypoints.SteamAPI_vr_IVRSystem_PollNextEvent(m_pVRSystem,ref pEvent);
	return result;
}
public override string GetEventTypeNameFromEnum(uint eType)
{
	CheckIfUsable();
	IntPtr result = NativeEntrypoints.SteamAPI_vr_IVRSystem_GetEventTypeNameFromEnum(m_pVRSystem,eType);
	return (string) Marshal.PtrToStructure(result, typeof(string));
}
public override uint GetHiddenAreaMesh(Hmd_Eye eEye)
{
	CheckIfUsable();
	uint result = NativeEntrypoints.SteamAPI_vr_IVRSystem_GetHiddenAreaMesh(m_pVRSystem,eEye);
	return result;
}
}


public class CVRChaperone : IVRChaperone
{
public CVRChaperone(IntPtr VRChaperone)
{
	m_pVRChaperone = VRChaperone;
}
IntPtr m_pVRChaperone;

public override IntPtr GetIntPtr() { return m_pVRChaperone; }

private void CheckIfUsable()
{
	if (m_pVRChaperone == IntPtr.Zero)
	{
		throw new Exception("Steam Pointer not configured");
	}
}
public override ChaperoneCalibrationState GetCalibrationState()
{
	CheckIfUsable();
	ChaperoneCalibrationState result = NativeEntrypoints.SteamAPI_vr_IVRChaperone_GetCalibrationState(m_pVRChaperone);
	return result;
}
public override bool GetSoftBoundsInfo(ref ChaperoneSoftBoundsInfo_t pInfo)
{
	CheckIfUsable();
	bool result = NativeEntrypoints.SteamAPI_vr_IVRChaperone_GetSoftBoundsInfo(m_pVRChaperone,ref pInfo);
	return result;
}
public override bool GetHardBoundsInfo(out HmdQuad_t [] pQuadsBuffer)
{
	CheckIfUsable();
	uint punQuadsCount = 0;
	bool result = NativeEntrypoints.SteamAPI_vr_IVRChaperone_GetHardBoundsInfo(m_pVRChaperone,null,ref punQuadsCount);
	pQuadsBuffer= new HmdQuad_t[punQuadsCount];
	 result = NativeEntrypoints.SteamAPI_vr_IVRChaperone_GetHardBoundsInfo(m_pVRChaperone,pQuadsBuffer,ref punQuadsCount);
	return result;
}
public override bool GetSeatedBoundsInfo(ref ChaperoneSeatedBoundsInfo_t pInfo)
{
	CheckIfUsable();
	bool result = NativeEntrypoints.SteamAPI_vr_IVRChaperone_GetSeatedBoundsInfo(m_pVRChaperone,ref pInfo);
	return result;
}
}


public class CVRCompositor : IVRCompositor
{
public CVRCompositor(IntPtr VRCompositor)
{
	m_pVRCompositor = VRCompositor;
}
IntPtr m_pVRCompositor;

public override IntPtr GetIntPtr() { return m_pVRCompositor; }

private void CheckIfUsable()
{
	if (m_pVRCompositor == IntPtr.Zero)
	{
		throw new Exception("Steam Pointer not configured");
	}
}
public override uint GetLastError(System.Text.StringBuffer pchBuffer,uint unBufferSize)
{
	CheckIfUsable();
	uint result = NativeEntrypoints.SteamAPI_vr_IVRCompositor_GetLastError(m_pVRCompositor,pchBuffer,unBufferSize);
	return result;
}
public override void SetVSync(bool bVSync)
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_SetVSync(m_pVRCompositor,bVSync);
}
public override bool GetVSync()
{
	CheckIfUsable();
	bool result = NativeEntrypoints.SteamAPI_vr_IVRCompositor_GetVSync(m_pVRCompositor);
	return result;
}
public override void SetGamma(float fGamma)
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_SetGamma(m_pVRCompositor,fGamma);
}
public override float GetGamma()
{
	CheckIfUsable();
	float result = NativeEntrypoints.SteamAPI_vr_IVRCompositor_GetGamma(m_pVRCompositor);
	return result;
}
public override void SetGraphicsDevice(Compositor_DeviceType eType,IntPtr pDevice)
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_SetGraphicsDevice(m_pVRCompositor,eType,pDevice);
}
public override void WaitGetPoses(TrackedDevicePose_t [] pPoseArray)
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_WaitGetPoses(m_pVRCompositor,pPoseArray,(uint) pPoseArray.Length);
}
public override void Submit(Hmd_Eye eEye,IntPtr pTexture,ref Compositor_TextureBounds pBounds)
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_Submit(m_pVRCompositor,eEye,pTexture,ref pBounds);
}
public override void ClearLastSubmittedFrame()
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_ClearLastSubmittedFrame(m_pVRCompositor);
}
public override void GetOverlayDefaults(ref Compositor_OverlaySettings pSettings)
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_GetOverlayDefaults(m_pVRCompositor,ref pSettings);
}
public override void SetOverlay(IntPtr pTexture,ref Compositor_OverlaySettings pSettings)
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_SetOverlay(m_pVRCompositor,pTexture,ref pSettings);
}
public override void SetOverlayRaw(IntPtr buffer,uint width,uint height,uint depth,ref Compositor_OverlaySettings pSettings)
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_SetOverlayRaw(m_pVRCompositor,buffer,width,height,depth,ref pSettings);
}
public override void SetOverlayFromFile(string pchFilePath,ref Compositor_OverlaySettings pSettings)
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_SetOverlayFromFile(m_pVRCompositor,pchFilePath,ref pSettings);
}
public override void ClearOverlay()
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_ClearOverlay(m_pVRCompositor);
}
public override bool GetFrameTiming(ref Compositor_FrameTiming pTiming,uint unFramesAgo)
{
	CheckIfUsable();
	bool result = NativeEntrypoints.SteamAPI_vr_IVRCompositor_GetFrameTiming(m_pVRCompositor,ref pTiming,unFramesAgo);
	return result;
}
public override void FadeToColor(float fSeconds,float fRed,float fGreen,float fBlue,float fAlpha,bool bBackground)
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_FadeToColor(m_pVRCompositor,fSeconds,fRed,fGreen,fBlue,fAlpha,bBackground);
}
public override void FadeGrid(float fSeconds,bool bFadeIn)
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_FadeGrid(m_pVRCompositor,fSeconds,bFadeIn);
}
public override void CompositorBringToFront()
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_CompositorBringToFront(m_pVRCompositor);
}
public override void CompositorGoToBack()
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_CompositorGoToBack(m_pVRCompositor);
}
public override void CompositorQuit()
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRCompositor_CompositorQuit(m_pVRCompositor);
}
public override bool IsFullscreen()
{
	CheckIfUsable();
	bool result = NativeEntrypoints.SteamAPI_vr_IVRCompositor_IsFullscreen(m_pVRCompositor);
	return result;
}
}


public class CVRControlPanel : IVRControlPanel
{
public CVRControlPanel(IntPtr VRControlPanel)
{
	m_pVRControlPanel = VRControlPanel;
}
IntPtr m_pVRControlPanel;

public override IntPtr GetIntPtr() { return m_pVRControlPanel; }

private void CheckIfUsable()
{
	if (m_pVRControlPanel == IntPtr.Zero)
	{
		throw new Exception("Steam Pointer not configured");
	}
}
public override uint GetDriverCount()
{
	CheckIfUsable();
	uint result = NativeEntrypoints.SteamAPI_vr_IVRControlPanel_GetDriverCount(m_pVRControlPanel);
	return result;
}
public override uint GetDriverId(uint unDriverIndex,string pchBuffer,uint unBufferLen)
{
	CheckIfUsable();
	uint result = NativeEntrypoints.SteamAPI_vr_IVRControlPanel_GetDriverId(m_pVRControlPanel,unDriverIndex,pchBuffer,unBufferLen);
	return result;
}
public override uint GetDriverDisplayCount(string pchDriverId)
{
	CheckIfUsable();
	uint result = NativeEntrypoints.SteamAPI_vr_IVRControlPanel_GetDriverDisplayCount(m_pVRControlPanel,pchDriverId);
	return result;
}
public override uint GetDriverDisplayId(string pchDriverId,uint unDisplayIndex,string pchBuffer,uint unBufferLen)
{
	CheckIfUsable();
	uint result = NativeEntrypoints.SteamAPI_vr_IVRControlPanel_GetDriverDisplayId(m_pVRControlPanel,pchDriverId,unDisplayIndex,pchBuffer,unBufferLen);
	return result;
}
public override uint GetDriverDisplayModelNumber(string pchDriverId,string pchDisplayId,string pchBuffer,uint unBufferLen)
{
	CheckIfUsable();
	uint result = NativeEntrypoints.SteamAPI_vr_IVRControlPanel_GetDriverDisplayModelNumber(m_pVRControlPanel,pchDriverId,pchDisplayId,pchBuffer,unBufferLen);
	return result;
}
public override uint GetDriverDisplaySerialNumber(string pchDriverId,string pchDisplayId,string pchBuffer,uint unBufferLen)
{
	CheckIfUsable();
	uint result = NativeEntrypoints.SteamAPI_vr_IVRControlPanel_GetDriverDisplaySerialNumber(m_pVRControlPanel,pchDriverId,pchDisplayId,pchBuffer,unBufferLen);
	return result;
}
public override uint LoadSharedResource(string pchResourceName,string pchBuffer,uint unBufferLen)
{
	CheckIfUsable();
	uint result = NativeEntrypoints.SteamAPI_vr_IVRControlPanel_LoadSharedResource(m_pVRControlPanel,pchResourceName,pchBuffer,unBufferLen);
	return result;
}
public override float GetIPD()
{
	CheckIfUsable();
	float result = NativeEntrypoints.SteamAPI_vr_IVRControlPanel_GetIPD(m_pVRControlPanel);
	return result;
}
public override void SetIPD(float fIPD)
{
	CheckIfUsable();
	NativeEntrypoints.SteamAPI_vr_IVRControlPanel_SetIPD(m_pVRControlPanel,fIPD);
}
}


public class SteamAPIInterop
{
[DllImportAttribute("Steam_api", EntryPoint = "SteamAPI_RestartAppIfNecessary")]
internal static extern void SteamAPI_RestartAppIfNecessary(uint unOwnAppID );
[DllImportAttribute("Steam_api", EntryPoint = "SteamAPI_Init")]
internal static extern void SteamAPI_Init();
[DllImportAttribute("Steam_api", EntryPoint = "SteamAPI_RunCallbacks")]
internal static extern void SteamAPI_RunCallbacks();
[DllImportAttribute("Steam_api", EntryPoint = "SteamAPI_RegisterCallback")]
internal static extern void SteamAPI_RegisterCallback(IntPtr pCallback, int iCallback);
[DllImportAttribute("Steam_api", EntryPoint = "SteamAPI_UnregisterCallback")]
internal static extern void SteamAPI_UnregisterCallback(IntPtr pCallback);
[DllImportAttribute("Steam_api", EntryPoint = "SteamClient")]
internal static extern IntPtr SteamClient();
[DllImportAttribute("Steam_api", EntryPoint = "SteamUser")]
internal static extern IntPtr SteamUser();
[DllImportAttribute("Steam_api", EntryPoint = "SteamFriends")]
internal static extern IntPtr SteamFriends();
[DllImportAttribute("Steam_api", EntryPoint = "SteamUtils")]
internal static extern IntPtr SteamUtils();
[DllImportAttribute("Steam_api", EntryPoint = "SteamMatchmaking")]
internal static extern IntPtr SteamMatchmaking();
[DllImportAttribute("Steam_api", EntryPoint = "SteamMatchmakingServerListResponse")]
internal static extern IntPtr SteamMatchmakingServerListResponse();
[DllImportAttribute("Steam_api", EntryPoint = "SteamMatchmakingPingResponse")]
internal static extern IntPtr SteamMatchmakingPingResponse();
[DllImportAttribute("Steam_api", EntryPoint = "SteamMatchmakingPlayersResponse")]
internal static extern IntPtr SteamMatchmakingPlayersResponse();
[DllImportAttribute("Steam_api", EntryPoint = "SteamMatchmakingRulesResponse")]
internal static extern IntPtr SteamMatchmakingRulesResponse();
[DllImportAttribute("Steam_api", EntryPoint = "SteamMatchmakingServers")]
internal static extern IntPtr SteamMatchmakingServers();
[DllImportAttribute("Steam_api", EntryPoint = "SteamRemoteStorage")]
internal static extern IntPtr SteamRemoteStorage();
[DllImportAttribute("Steam_api", EntryPoint = "SteamUserStats")]
internal static extern IntPtr SteamUserStats();
[DllImportAttribute("Steam_api", EntryPoint = "SteamApps")]
internal static extern IntPtr SteamApps();
[DllImportAttribute("Steam_api", EntryPoint = "SteamNetworking")]
internal static extern IntPtr SteamNetworking();
[DllImportAttribute("Steam_api", EntryPoint = "SteamScreenshots")]
internal static extern IntPtr SteamScreenshots();
[DllImportAttribute("Steam_api", EntryPoint = "SteamMusic")]
internal static extern IntPtr SteamMusic();
[DllImportAttribute("Steam_api", EntryPoint = "SteamMusicRemote")]
internal static extern IntPtr SteamMusicRemote();
[DllImportAttribute("Steam_api", EntryPoint = "SteamHTTP")]
internal static extern IntPtr SteamHTTP();
[DllImportAttribute("Steam_api", EntryPoint = "SteamUnifiedMessages")]
internal static extern IntPtr SteamUnifiedMessages();
[DllImportAttribute("Steam_api", EntryPoint = "SteamController")]
internal static extern IntPtr SteamController();
[DllImportAttribute("Steam_api", EntryPoint = "SteamUGC")]
internal static extern IntPtr SteamUGC();
[DllImportAttribute("Steam_api", EntryPoint = "SteamAppList")]
internal static extern IntPtr SteamAppList();
[DllImportAttribute("Steam_api", EntryPoint = "SteamHTMLSurface")]
internal static extern IntPtr SteamHTMLSurface();
[DllImportAttribute("Steam_api", EntryPoint = "SteamInventory")]
internal static extern IntPtr SteamInventory();
[DllImportAttribute("Steam_api", EntryPoint = "SteamVideo")]
internal static extern IntPtr SteamVideo();
[DllImportAttribute("Steam_api", EntryPoint = "CV")]
internal static extern IntPtr CV();
[DllImportAttribute("Steam_api", EntryPoint = "CV")]
internal static extern IntPtr CV();
[DllImportAttribute("Steam_api", EntryPoint = "CV")]
internal static extern IntPtr CV();
[DllImportAttribute("Steam_api", EntryPoint = "CV")]
internal static extern IntPtr CV();
[DllImportAttribute("Steam_api", EntryPoint = "SteamGameServer")]
internal static extern IntPtr SteamGameServer();
[DllImportAttribute("Steam_api", EntryPoint = "SteamGameServerStats")]
internal static extern IntPtr SteamGameServerStats();
}


public enum Hmd_Eye
{
	Eye_Left = 0,
	Eye_Right = 1,
}
public enum GraphicsAPIConvention
{
	API_DirectX = 0,
	API_OpenGL = 1,
}
public enum HmdTrackingResult
{
	TrackingResult_Uninitialized = 1,
	TrackingResult_Calibrating_InProgress = 100,
	TrackingResult_Calibrating_OutOfRange = 101,
	TrackingResult_Running_OK = 200,
	TrackingResult_Running_OutOfRange = 201,
}
public enum TrackedDeviceClass
{
	TrackedDeviceClass_Invalid = 0,
	TrackedDeviceClass_HMD = 1,
	TrackedDeviceClass_Controller = 2,
	TrackedDeviceClass_TrackingReference = 4,
	TrackedDeviceClass_Other = 1000,
}
public enum TrackingUniverseOrigin
{
	TrackingUniverseSeated = 0,
	TrackingUniverseStanding = 1,
	TrackingUniverseRawAndUncalibrated = 2,
}
public enum TrackedDeviceProperty
{
	Prop_TrackingSystemName_String = 1000,
	Prop_ModelNumber_String = 1001,
	Prop_SerialNumber_String = 1002,
	Prop_RenderModelName_String = 1003,
	Prop_WillDriftInYaw_Bool = 1004,
	Prop_ManufacturerName_String = 1005,
	Prop_TrackingFirmwareVersion_String = 1006,
	Prop_HardwareRevision_String = 1007,
	Prop_ReportsTimeSinceVSync_Bool = 2000,
	Prop_SecondsFromVsyncToPhotons_Float = 2001,
	Prop_DisplayFrequency_Float = 2002,
	Prop_UserIpdMeters_Float = 2003,
	Prop_CurrentUniverseId_Uint64 = 2004,
	Prop_PreviousUniverseId_Uint64 = 2005,
	Prop_DisplayFirmwareVersion_String = 2006,
	Prop_AttachedDeviceId_String = 3000,
	Prop_FieldOfViewLeftDegrees_Float = 4000,
	Prop_FieldOfViewRightDegrees_Float = 4001,
	Prop_FieldOfViewTopDegrees_Float = 4002,
	Prop_FieldOfViewBottomDegrees_Float = 4003,
	Prop_TrackingRangeMinimumMeters_Float = 4004,
	Prop_TrackingRangeMaximumMeters_Float = 4005,
}
public enum TrackedPropertyError
{
	TrackedProp_Success = 0,
	TrackedProp_WrongDataType = 1,
	TrackedProp_WrongDeviceClass = 2,
	TrackedProp_BufferTooSmall = 3,
	TrackedProp_UnknownProperty = 4,
	TrackedProp_InvalidDevice = 5,
	TrackedProp_CouldNotContactServer = 6,
	TrackedProp_ValueNotProvidedByDevice = 7,
	TrackedProp_StringExceedsMaximumLength = 8,
}
public enum EVREventType
{
	VREvent_None = 0,
	VREvent_TrackedDeviceActivated = 100,
	VREvent_TrackedDeviceDeactivated = 101,
	VREvent_TrackedDeviceUpdated = 102,
}
public enum HmdError
{
	HmdError_None = 0,
	HmdError_Init_InstallationNotFound = 100,
	HmdError_Init_InstallationCorrupt = 101,
	HmdError_Init_VRClientDLLNotFound = 102,
	HmdError_Init_FileNotFound = 103,
	HmdError_Init_FactoryNotFound = 104,
	HmdError_Init_InterfaceNotFound = 105,
	HmdError_Init_InvalidInterface = 106,
	HmdError_Init_UserConfigDirectoryInvalid = 107,
	HmdError_Init_HmdNotFound = 108,
	HmdError_Init_NotInitialized = 109,
	HmdError_Init_PathRegistryNotFound = 110,
	HmdError_Init_NoConfigPath = 111,
	HmdError_Init_NoLogPath = 112,
	HmdError_Init_PathRegistryNotWritable = 113,
	HmdError_Driver_Failed = 200,
	HmdError_Driver_Unknown = 201,
	HmdError_Driver_HmdUnknown = 202,
	HmdError_Driver_NotLoaded = 203,
	HmdError_Driver_RuntimeOutOfDate = 204,
	HmdError_Driver_HmdInUse = 205,
	HmdError_IPC_ServerInitFailed = 300,
	HmdError_IPC_ConnectFailed = 301,
	HmdError_IPC_SharedStateInitFailed = 302,
	HmdError_IPC_CompositorInitFailed = 303,
	HmdError_IPC_MutexInitFailed = 304,
	HmdError_VendorSpecific_UnableToConnectToOculusRuntime = 1000,
	HmdError_Steam_SteamInstallationNotFound = 2000,
}
public enum ChaperoneCalibrationState
{
	ChaperoneCalibrationState_OK = 1,
	ChaperoneCalibrationState_Warning = 100,
	ChaperoneCalibrationState_Warning_BaseStationMayHaveMoved = 101,
	ChaperoneCalibrationState_Warning_BaseStationRemoved = 102,
	ChaperoneCalibrationState_Warning_SeatedBoundsInvalid = 103,
	ChaperoneCalibrationState_Error = 200,
	ChaperoneCalibrationState_Error_BaseStationUninitalized = 201,
	ChaperoneCalibrationState_Error_BaseStationConflict = 202,
	ChaperoneCalibrationState_Error_SoftBoundsInvalid = 203,
	ChaperoneCalibrationState_Error_HardBoundsInvalid = 204,
}
public enum Compositor_DeviceType
{
	Compositor_DeviceType_None = 0,
	Compositor_DeviceType_D3D9 = 1,
	Compositor_DeviceType_D3D9Ex = 2,
	Compositor_DeviceType_D3D10 = 3,
	Compositor_DeviceType_D3D11 = 4,
	Compositor_DeviceType_OpenGL = 5,
}
[StructLayout(LayoutKind.Sequential)] public struct HmdMatrix34_t
{
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 12, ArraySubType = UnmanagedType.R4)]
	public float[] m; //float[3][4]
}
[StructLayout(LayoutKind.Sequential)] public struct HmdMatrix44_t
{
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 16, ArraySubType = UnmanagedType.R4)]
	public float[] m; //float[4][4]
}
[StructLayout(LayoutKind.Sequential)] public struct HmdVector3_t
{
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 3, ArraySubType = UnmanagedType.R4)]
	public float[] v; //float[3]
}
[StructLayout(LayoutKind.Sequential)] public struct HmdVector3d_t
{
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 3, ArraySubType = UnmanagedType.R8)]
	public double[] v; //double[3]
}
[StructLayout(LayoutKind.Sequential)] public struct HmdVector2_t
{
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2, ArraySubType = UnmanagedType.R4)]
	public float[] v; //float[2]
}
[StructLayout(LayoutKind.Sequential)] public struct HmdQuaternion_t
{
	public double w;
	public double x;
	public double y;
	public double z;
}
[StructLayout(LayoutKind.Sequential)] public struct HmdQuad_t
{
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 4, ArraySubType = UnmanagedType.HmdVector3_t)]
	public HmdVector3_t[] vCorners; //HmdVector3_t[4]
}
[StructLayout(LayoutKind.Sequential)] public struct DistortionCoordinates_t
{
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2, ArraySubType = UnmanagedType.R4)]
	public float[] rfRed; //float[2]
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2, ArraySubType = UnmanagedType.R4)]
	public float[] rfGreen; //float[2]
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2, ArraySubType = UnmanagedType.R4)]
	public float[] rfBlue; //float[2]
}
[StructLayout(LayoutKind.Sequential)] public struct TrackedDevicePose_t
{
	public HmdMatrix34_t mDeviceToAbsoluteTracking;
	public HmdVector3_t vVelocity;
	public HmdVector3_t vAngularVelocity;
	public HmdTrackingResult eTrackingResult;
	public bool bPoseIsValid;
	public bool bDeviceIsValid;
}
[StructLayout(LayoutKind.Sequential)] public struct RenderModel_Vertex_t
{
	public HmdVector3_t vPosition;
	public HmdVector3_t vNormal;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2, ArraySubType = UnmanagedType.R4)]
	public float[] rfTextureCoord; //float[2]
}
[StructLayout(LayoutKind.Sequential)] public struct RenderModel_TextureMap_t
{
	public char unWidth;
	public char unHeight;
	public byte rubTextureMapData;
}
[StructLayout(LayoutKind.Sequential)] public struct RenderModel_t
{
	public ulong ulInternalHandle;
	public RenderModel_Vertex_t rVertexData;
	public uint unVertexCount;
	public char rIndexData;
	public uint unTriangleCount;
	public RenderModel_TextureMap_t diffuseTexture;
}
[StructLayout(LayoutKind.Sequential)] public struct VREvent_t
{
	public uint eventType;
	public TrackedDeviceIndex_t trackedDeviceIndex;
	public uint param1;
	public float eventAgeSeconds;
}
[StructLayout(LayoutKind.Sequential)] public struct HiddenAreaMesh_t
{
	public HmdVector2_t pVertexData;
	public uint unTriangleCount;
}
[StructLayout(LayoutKind.Sequential)] public struct ChaperoneSoftBoundsInfo_t
{
	public HmdQuad_t quadCorners;
}
[StructLayout(LayoutKind.Sequential)] public struct ChaperoneSeatedBoundsInfo_t
{
	public HmdVector3_t vSeatedHeadPosition;
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 2, ArraySubType = UnmanagedType.HmdVector3_t)]
	public HmdVector3_t[] vDeskEdgePositions; //HmdVector3_t[2]
}
[StructLayout(LayoutKind.Sequential)] public struct Compositor_FrameTiming
{
	public uint size;
	public double frameStart;
	public float frameVSync;
	public uint droppedFrames;
	public uint frameIndex;
	public TrackedDevicePose_t pose;
}
[StructLayout(LayoutKind.Sequential)] public struct Compositor_OverlaySettings
{
	public uint size;
	public bool curved;
	public bool antialias;
	public float scale;
	public float distance;
	public float alpha;
	public float uOffset;
	public float vOffset;
	public float uScale;
	public float vScale;
	public float gridDivs;
	public float gridWidth;
	public float gridScale;
	public HmdMatrix44_t transform;
}
[StructLayout(LayoutKind.Sequential)] public struct Compositor_TextureBounds
{
	public float uMin;
	public float vMin;
	public float uMax;
	public float vMax;
}

public class SteamVR
{
public static void Init(uint appId)
{
SteamAPIInterop.SteamAPI_RestartAppIfNecessary (appId);
SteamAPIInterop.SteamAPI_Init ();
}

public static void RunCallbacks()
{
SteamAPIInterop.SteamAPI_RunCallbacks ();
}

public static void RegisterCallback(IntPtr pCallback, int iCallback)
{
SteamAPIInterop.SteamAPI_RegisterCallback (pCallback, iCallback);
}

public static void UnregisterCallback(IntPtr pCallback)
{
SteamAPIInterop.SteamAPI_UnregisterCallback (pCallback);
}

public const int k_iSteamUserCallbacks = 100;
public const int k_iSteamGameServerCallbacks = 200;
public const int k_iSteamFriendsCallbacks = 300;
public const int k_iSteamBillingCallbacks = 400;
public const int k_iSteamMatchmakingCallbacks = 500;
public const int k_iSteamContentServerCallbacks = 600;
public const int k_iSteamUtilsCallbacks = 700;
public const int k_iClientFriendsCallbacks = 800;
public const int k_iClientUserCallbacks = 900;
public const int k_iSteamAppsCallbacks = 1000;
public const int k_iSteamUserStatsCallbacks = 1100;
public const int k_iSteamNetworkingCallbacks = 1200;
public const int k_iClientRemoteStorageCallbacks = 1300;
public const int k_iClientDepotBuilderCallbacks = 1400;
public const int k_iSteamGameServerItemsCallbacks = 1500;
public const int k_iClientUtilsCallbacks = 1600;
public const int k_iSteamGameCoordinatorCallbacks = 1700;
public const int k_iSteamGameServerStatsCallbacks = 1800;
public const int k_iSteam2AsyncCallbacks = 1900;
public const int k_iSteamGameStatsCallbacks = 2000;
public const int k_iClientHTTPCallbacks = 2100;
public const int k_iClientScreenshotsCallbacks = 2200;
public const int k_iSteamScreenshotsCallbacks = 2300;
public const int k_iClientAudioCallbacks = 2400;
public const int k_iClientUnifiedMessagesCallbacks = 2500;
public const int k_iSteamStreamLauncherCallbacks = 2600;
public const int k_iClientControllerCallbacks = 2700;
public const int k_iSteamControllerCallbacks = 2800;
public const int k_iClientParentalSettingsCallbacks = 2900;
public const int k_iClientDeviceAuthCallbacks = 3000;
public const int k_iClientNetworkDeviceManagerCallbacks = 3100;
public const int k_iClientMusicCallbacks = 3200;
public const int k_iClientRemoteClientManagerCallbacks = 3300;
public const int k_iClientUGCCallbacks = 3400;
public const int k_iSteamStreamClientCallbacks = 3500;
public const int k_IClientProductBuilderCallbacks = 3600;
public const int k_iClientShortcutsCallbacks = 3700;
public const int k_iClientRemoteControlManagerCallbacks = 3800;
public const int k_iSteamAppListCallbacks = 3900;
public const int k_iSteamMusicCallbacks = 4000;
public const int k_iSteamMusicRemoteCallbacks = 4100;
public const int k_iClientVRCallbacks = 4200;
public const int k_iClientReservedCallbacks = 4300;
public const int k_iSteamReservedCallbacks = 4400;
public const int k_iSteamHTMLSurfaceCallbacks = 4500;
public const int k_iClientVideoCallbacks = 4600;
public const int k_iClientInventoryCallbacks = 4700;
public const int k_cchPersonaNameMax = 128;
public const int k_cwchPersonaNameMax = 32;
public const int k_cchMaxRichPresenceKeys = 20;
public const int k_cchMaxRichPresenceKeyLength = 64;
public const int k_cchMaxRichPresenceValueLength = 256;
public const int k_cchStatNameMax = 128;
public const int k_cchLeaderboardNameMax = 128;
public const int k_cLeaderboardDetailsMax = 64;
}



}

