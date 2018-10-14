#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "MathHelper.h"
#include "LightHelper.h"

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
};

class BoxApp : public D3DApp
{
public:
	BoxApp(HINSTANCE hInstance);
	~BoxApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene(); 

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void BuildGeometryBuffers();
	void BuildFX();
	void BuildVertexLayout();

private:
	ID3D11Buffer* mBoxVB;
	ID3D11Buffer* mBoxIB;

	ID3DX11Effect* mFX;
	ID3DX11EffectTechnique* mTech;
	ID3DX11EffectMatrixVariable* mfxWorldViewProj;

	ID3D11InputLayout* mInputLayout;

	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;

	///////////////////////////////
	DirectionalLight mDirLight;
	PointLight mPointLight;
	SpotLight mSpotLight;
	Material mBoxMat;

	ID3DX11EffectMatrixVariable* mfxWorld;
	ID3DX11EffectMatrixVariable* mfxWorldInvTranspose;
	ID3DX11EffectVectorVariable* mfxEyePosW;
	ID3DX11EffectVariable* mfxDirLight;
	ID3DX11EffectVariable* mfxPointLight;
	ID3DX11EffectVariable* mfxSpotLight;
	ID3DX11EffectVariable* mfxMaterial;

	XMFLOAT3 mEyePosW;
	///////////////////////////////
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	BoxApp theApp(hInstance);
	
	if( !theApp.Init() )
		return 0;
	
	return theApp.Run();
}
 

BoxApp::BoxApp(HINSTANCE hInstance)
: D3DApp(hInstance), mBoxVB(0), mBoxIB(0), mFX(0), mTech(0),
  mfxWorldViewProj(0), mInputLayout(0), 
  mTheta(1.5f*MathHelper::Pi), mPhi(0.25f*MathHelper::Pi), mRadius(5.0f),
   mfxWorld(0), mfxWorldInvTranspose(0), mfxEyePosW(0), 
  mfxDirLight(0), mfxPointLight(0), mfxSpotLight(0), mfxMaterial(0),mEyePosW(0.0f, 0.0f, 0.0f)
{
	mMainWndCaption = L"Lighting Box Demo";
	
	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	////////////////////////////////////////////////////
	// Directional light. //red
	mDirLight.Ambient  = XMFLOAT4(0.9952f, 0.0975f, 0.0f, 1.0f);
	mDirLight.Diffuse  = XMFLOAT4(0.9952f, 0.0975f, 0.0f, 1.0f);
	mDirLight.Specular = XMFLOAT4(0.9952f, 0.0975f, 0.0f, 1.0f);
	mDirLight.Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);
 
	// Point light--position is changed every frame to animate in UpdateScene function. // green
	mPointLight.Ambient  = XMFLOAT4(0.0f, 0.9988f, 0.047f, 1.0f);
	mPointLight.Diffuse  = XMFLOAT4(0.0f, 0.9988f, 0.047f, 1.0f);
	mPointLight.Specular = XMFLOAT4(0.0f, 0.9988f, 0.047f, 1.0f);
	mPointLight.Att      = XMFLOAT3(0.0f, 0.1f, 0.0f);
	mPointLight.Range    = 25.0f;

	// Spot light--position and direction changed every frame to animate in UpdateScene function.//blue
	mSpotLight.Ambient  = XMFLOAT4(0.0f, 0.9813f, 0.1924f, 1.0f);
	mSpotLight.Diffuse  = XMFLOAT4(0.0f, 0.9813f, 0.1924f, 1.0f);
	mSpotLight.Specular = XMFLOAT4(0.0f, 0.9813f, 0.1924f, 1.0f);
	mSpotLight.Att      = XMFLOAT3(1.0f, 0.0f, 0.0f);
	mSpotLight.Spot     = 96.0f;
	mSpotLight.Range    = 10000.0f;

	mBoxMat.Ambient  = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mBoxMat.Diffuse  = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mBoxMat.Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
}

BoxApp::~BoxApp()
{
	ReleaseCOM(mBoxVB);
	ReleaseCOM(mBoxIB);
	ReleaseCOM(mFX);
	ReleaseCOM(mInputLayout);
}

bool BoxApp::Init()
{
	if(!D3DApp::Init())
		return false;

	BuildGeometryBuffers();
	BuildFX();
	BuildVertexLayout();

	return true;
}

void BoxApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void BoxApp::UpdateScene(float dt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius*sinf(mPhi)*cosf(mTheta);
	float z = mRadius*sinf(mPhi)*sinf(mTheta);
	float y = mRadius*cosf(mPhi);

	mEyePosW = XMFLOAT3(x, y, z);

	// Build the view matrix.
	XMVECTOR pos    = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up     = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);

	///////////////////////////////////////////////
	mSpotLight.Position = mEyePosW;
	mPointLight.Position = mSpotLight.Position;
	XMStoreFloat3(&mSpotLight.Direction, XMVector3Normalize(target - pos));
}

void BoxApp::DrawScene()
{
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	md3dImmediateContext->IASetInputLayout(mInputLayout);
    md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
    UINT offset = 0;
    md3dImmediateContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);

	// Set constants
	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX view  = XMLoadFloat4x4(&mView);
	XMMATRIX proj  = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world*view*proj;
	XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
	mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
	////////////////////////////////
	// Set per frame constants.
	mfxDirLight->SetRawValue(&mDirLight, 0, sizeof(mDirLight));
	mfxPointLight->SetRawValue(&mPointLight, 0, sizeof(mPointLight));
	mfxSpotLight->SetRawValue(&mSpotLight, 0, sizeof(mSpotLight));
	mfxEyePosW->SetRawValue(&mEyePosW, 0, sizeof(mEyePosW));
	////////////////////////////////
    D3DX11_TECHNIQUE_DESC techDesc;
    mTech->GetDesc( &techDesc );
    for(UINT p = 0; p < techDesc.Passes; ++p)
    {
		mfxWorld->SetMatrix(reinterpret_cast<float*>(&world));
		mfxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
		mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
		mfxMaterial->SetRawValue(&mBoxMat, 0, sizeof(mBoxMat));
        mTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
        
		// 36 indices for the box.
		md3dImmediateContext->DrawIndexed(36, 0, 0);
    }

	HR(mSwapChain->Present(0, 0));
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if( (btnState & MK_LBUTTON) != 0 )
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi   += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi-0.1f);
	}
	else if( (btnState & MK_RBUTTON) != 0 )
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.005f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void BoxApp::BuildGeometryBuffers()
{
	// Create vertex buffer
	//em chua biet tinh @@

    Vertex vertices[] =
    {
		//////// Front Face
		{XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3 (0.0f, 0.0f , -1.0f)},
		{XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3 (0.0f, 0.0f , -1.0f)},
		{XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3 (0.0f, 0.0f , -1.0f)},
		{XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3 (0.0f, 0.0f , -1.0f)},
		//////// Back Face
		{XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3 (0.0f, 0.0f , 1.0f)},
		{XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3 (0.0f, 0.0f , 1.0f)},
		{XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3 (0.0f, 0.0f , 1.0f)},
		{XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3 (0.0f, 0.0f , 1.0f)},
		/////// Top Face
		{XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3 (0.0f, 1.0f , 0.0f)},
		{XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3 (0.0f, 1.0f , 0.0f)},
		{XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3 (0.0f, 1.0f , 0.0f)},
		{XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3 (0.0f, 1.0f , 0.0f)},
		/////// Bottom Face
		{XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3 (0.0f, -1.0f , 0.0f)},
		{XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3 (0.0f, -1.0f , 0.0f)},
		{XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3 (0.0f, -1.0f , 0.0f)},
		{XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3 (0.0f, -1.0f , 0.0f)},
		////// Left Face
		{XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3 (-1.0f, 0.0f , 0.0f)},
		{XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3 (-1.0f, 0.0f , 0.0f)},
		{XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3 (-1.0f, 0.0f , 0.0f)},
		{XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3 (-1.0f, 0.0f , 0.0f)},
		////// Right Face
		{XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3 (1.0f, 0.0f , 0.0f)},
		{XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3 (1.0f, 0.0f , 0.0f)},
		{XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3 (1.0f, 0.0f , 0.0f)},
		{XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3 (1.0f, 0.0f , 0.0f)}

    };

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex) * 24;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = vertices;
    HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));


	// Create the index buffer
	UINT i[36];

	// Fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// Fill in the back face index data
	i[6] = 4; i[7]  = 5; i[8]  = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// Fill in the top face index data
	i[12] = 8; i[13] =  9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// Fill in the left face index data
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// Fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(UINT) * 36;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = i;
    HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
}
 
void BoxApp::BuildFX()
{
	std::ifstream fin("fx/Lighting.fxo", std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compiledShader(size);

	fin.read(&compiledShader[0], size);
	fin.close();
	
	HR(D3DX11CreateEffectFromMemory(&compiledShader[0], size, 
		0, md3dDevice, &mFX));

	mTech                = mFX->GetTechniqueByName("NormalTech");
	//mTech                = mFX->GetTechniqueByName("LightTech");
	mfxWorldViewProj     = mFX->GetVariableByName("gWorldViewProj")->AsMatrix();
	mfxWorld             = mFX->GetVariableByName("gWorld")->AsMatrix();
	mfxWorldInvTranspose = mFX->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	mfxEyePosW           = mFX->GetVariableByName("gEyePosW")->AsVector();
	mfxDirLight          = mFX->GetVariableByName("gDirLight");
	mfxPointLight        = mFX->GetVariableByName("gPointLight");
	mfxSpotLight         = mFX->GetVariableByName("gSpotLight");
	mfxMaterial          = mFX->GetVariableByName("gMaterial");
}

void BoxApp::BuildVertexLayout()
{
	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

    D3DX11_PASS_DESC passDesc;
    mTech->GetPassByIndex(0)->GetDesc(&passDesc);
	HR(md3dDevice->CreateInputLayout(vertexDesc, 2, passDesc.pIAInputSignature, 
		passDesc.IAInputSignatureSize, &mInputLayout));

}
 