//***************************************************************************************
// ParticlesDemo.cpp by Mark M. Miller
//
// Demonstrates the use of GPU based particle systems and stream out.
//
// Controls:
//		Hold the left mouse button down and move the mouse to rotate.
//      Hold the right mouse button down to zoom in and out.
//
//***************************************************************************************

#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "Effects.h"
#include "Vertex.h"
#include "RenderStates.h"
#include "Camera.h"
#include "ParticleSystem.h"
#include <iostream>
#include <fstream>
#include <string>

class ParticlesApp : public D3DApp
{
public:
	ParticlesApp(HINSTANCE hInstance);
	~ParticlesApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene(); 

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	ID3D11ShaderResourceView* mFlareTexSRV;
	ID3D11ShaderResourceView* mRandomTexSRV;

	ParticleSystem mSmoke;

	DirectionalLight mDirLights[3];

	Camera mCam;

	bool mWalkCamMode;

	POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	ParticlesApp theApp(hInstance);
	
	if( !theApp.Init() )
		return 0;
	
	return theApp.Run();
}

ParticlesApp::ParticlesApp(HINSTANCE hInstance)
: D3DApp(hInstance), mRandomTexSRV(0), mFlareTexSRV(0), mWalkCamMode(false)
{
	mMainWndCaption = L"Particles Demo";
	mEnable4xMsaa = false;

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	mCam.SetPosition(0.0f, 2.0f, 75.0f);

	mDirLights[0].Ambient  = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	mDirLights[0].Diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mDirLights[0].Specular = XMFLOAT4(0.8f, 0.8f, 0.7f, 1.0f);
	mDirLights[0].Direction = XMFLOAT3(0.707f, -0.707f, 0.0f);

	mDirLights[1].Ambient  = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[1].Diffuse  = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[1].Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[1].Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	mDirLights[2].Ambient  = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[2].Diffuse  = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[2].Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[2].Direction = XMFLOAT3(-0.57735f, -0.57735f, -0.57735f);
}

ParticlesApp::~ParticlesApp()
{
	md3dImmediateContext->ClearState();
	
	ReleaseCOM(mRandomTexSRV);
	ReleaseCOM(mFlareTexSRV);

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
	RenderStates::DestroyAll();
}

bool ParticlesApp::Init()
{
	if(!D3DApp::Init())
		return false;

	// Must init Effects first since InputLayouts depend on shader signatures.
	Effects::InitAll(md3dDevice);
	InputLayouts::InitAll(md3dDevice);
	RenderStates::InitAll(md3dDevice);

	mRandomTexSRV = d3dHelper::CreateRandomTexture1DSRV(md3dDevice);

	// Add the texture to a list of textures that are to be used by the shader
	std::vector<std::wstring> smoke;
	smoke.push_back(L"Textures\\smoke0.dds");
	mFlareTexSRV = d3dHelper::CreateTexture2DArraySRV(md3dDevice, md3dImmediateContext, smoke);

	
	std::string line;
	UINT noofParticles = 0;
	std::string action = "";
	std::ifstream myotherfile ("sysSpecs.txt");
	if (myotherfile.is_open())
	{
		int check = 0; 
		while ( myotherfile.good() )
		{
			getline (myotherfile,line);
			if(check == 0)
			{
				noofParticles = atoi(line.c_str());
				check = check + 1;
			}
			else if (check == 1)
			{
				action.append(line.c_str());
			}
		}
		myotherfile.close();
	}
	else
	{
		std::ofstream myfile ("sysSpecs.txt");
		if (myfile.is_open())
		{
			myfile << "20000\n";
			myfile << "cube\n";
			myfile.close();
		}
		action = "quad";
		noofParticles = 20000;
	}
	if(action == "quad")
	{
		mSmoke.Init(md3dDevice, Effects::SmokeFX, mFlareTexSRV, mRandomTexSRV, noofParticles); 
	}
	else
	{
		mSmoke.Init(md3dDevice, Effects::RainFX, mFlareTexSRV, mRandomTexSRV, noofParticles); 
	}
	mSmoke.SetEmitPos(XMFLOAT3(0.0f, 1.0f, 120.0f));

	return true;
}

void ParticlesApp::OnResize()
{
	D3DApp::OnResize();

	mCam.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f,10000.0f);
}

void ParticlesApp::UpdateScene(float dt)
{
	// Camera controls
	if( GetAsyncKeyState('W') & 0x8000 )
		mCam.Walk(100.0f*dt);

	if( GetAsyncKeyState('S') & 0x8000 )
		mCam.Walk(-100.0f*dt);

	if( GetAsyncKeyState('A') & 0x8000 )
		mCam.Strafe(-100.0f*dt);

	if( GetAsyncKeyState('D') & 0x8000 )
		mCam.Strafe(100.0f*dt);


	
	// Reset particle systems if R is pressed	
	if(GetAsyncKeyState('R') & 0x8000)
	{
		mSmoke.Reset();
	}
 
	// Update the particle system
	mSmoke.Update(dt, mTimer.TotalTime());

	mCam.UpdateViewMatrix();
}

void ParticlesApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Black));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(InputLayouts::Basic32);
    md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 
	float blendFactor[] = {0.0f, 0.0f, 0.0f, 0.0f};

	if( GetAsyncKeyState('1') & 0x8000 )
		md3dImmediateContext->RSSetState(RenderStates::WireframeRS);


	md3dImmediateContext->RSSetState(0);

	mSmoke.SetEyePos(mCam.GetPosition());
	mSmoke.Draw(md3dImmediateContext, mCam);	

	md3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff); // restore default
	
	// restore default states.
	md3dImmediateContext->RSSetState(0);
	md3dImmediateContext->OMSetDepthStencilState(0, 0);
	md3dImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff); 

	HR(mSwapChain->Present(0, 0));
}

void ParticlesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void ParticlesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void ParticlesApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if( (btnState & MK_LBUTTON) != 0 )
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mCam.Pitch(dy);
		mCam.RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}
