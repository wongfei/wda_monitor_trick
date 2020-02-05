#include <windows.h>
#include <winternl.h>
#include <process.h>
#include <tlhelp32.h>
#include <inttypes.h>

#include <d3d9.h>
#pragma comment (lib, "d3d9.lib")

#include <stdexcept>
#include <vector>
#include <algorithm>
#include <chrono>

#include "win_utils.h"
#include "d3d9_x.h"

//=======================================================================================

#define WND_CLASS TEXT("wda_monitor")
#define WND_SIZEX 600
#define WND_SIZEY 600

static void xCreateWindow();
static void xInitD3d();
static void xMainLoop();
static void xShutdown();
static LRESULT CALLBACK xWindowProc(_In_ HWND   hwnd, _In_ UINT   uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

static HWND Window = NULL;
static LPDIRECT3D9 D3d = NULL;
static LPDIRECT3DDEVICE9 D3dDevice = NULL;
static LPDIRECT3DVERTEXBUFFER9 TriBuf = NULL;

//=======================================================================================

/*
.text:00000001801B90F4                                                 ; int CWindowNode::RenderBlackShape(CWindowNode *__hidden this, struct CDrawingContext *, const struct CShape *)
.text:00000001801B90F4                                                 ?RenderBlackShape@CWindowNode@@AEAAJPEAVCDrawingContext@@AEBVCShape@@@Z proc near
.text:00000001801B90F4                                                                                         ; CODE XREF: CWindowNode::RenderContent(CDrawingContext *,bool *)+5F9A6↑p
.text:00000001801B90F4                                                                                         ; CWindowNode::RenderBlackContent(CDrawingContext *)+AC↑p
.text:00000001801B90F4                                                                                         ; DATA XREF: ...
.text:00000001801B90F4
.text:00000001801B90F4                                                 var_18          = dword ptr -18h
.text:00000001801B90F4                                                 arg_0           = qword ptr  8
.text:00000001801B90F4
.text:00000001801B90F4 48 89 5C 24 08                                                  mov     [rsp+arg_0], rbx
.text:00000001801B90F9 57                                                              push    rdi
.text:00000001801B90FA 48 83 EC 30                                                     sub     rsp, 30h
.text:00000001801B90FE 49 8B C0                                                        mov     rax, r8
.text:00000001801B9101 48 8B FA                                                        mov     rdi, rdx
.text:00000001801B9104 48 8B D0                                                        mov     rdx, rax        ; struct CShape *
.text:00000001801B9107 4C 8D 05 D2 FE 0F 00                                            lea     r8, stru_1802B8FE0 ; struct _D3DCOLORVALUE *
.text:00000001801B910E 48 8B CF                                                        mov     rcx, rdi        ; this
.text:00000001801B9111 E8 8E BF FA FF                                                  call    ?FillRectangularShapeWithColor@CDrawingContext@@QEAAJAEBVCShape@@AEBU_D3DCOLORVALUE@@@Z ; CDrawingContext::FillRectangularShapeWithColor(CShape const &,_D3DCOLORVALUE const &)
.text:00000001801B9116 8B D8                                                           mov     ebx, eax
.text:00000001801B9118 85 C0                                                           test    eax, eax
.text:00000001801B911A 78 09                                                           js      short loc_1801B9125
.text:00000001801B911C C6 87 D3 18 00 00 01                                            mov     byte ptr [rdi+18D3h], 1
.text:00000001801B9123 EB 15                                                           jmp     short loc_1801B913A
.text:00000001801B9125                                                 ; ---------------------------------------------------------------------------
.text:00000001801B9125
.text:00000001801B9125                                                 loc_1801B9125:                          ; CODE XREF: CWindowNode::RenderBlackShape(CDrawingContext *,CShape const &)+26↑j
.text:00000001801B9125 44 8B CB                                                        mov     r9d, ebx        ; int
.text:00000001801B9128 C7 44 24 20 29 0D 00 00                                         mov     [rsp+38h+var_18], 0D29h ; unsigned int
.text:00000001801B9130 45 33 C0                                                        xor     r8d, r8d        ; unsigned int
.text:00000001801B9133 33 D2                                                           xor     edx, edx        ; int *
.text:00000001801B9135 E8 FE 7F EA FF                                                  call    ?MilInstrumentationCheckHR_MaybeFailFast@@YAXKQEBJIJI@Z ; MilInstrumentationCheckHR_MaybeFailFast(ulong,long const * const,uint,long,uint)
.text:00000001801B913A
.text:00000001801B913A                                                 loc_1801B913A:                          ; CODE XREF: CWindowNode::RenderBlackShape(CDrawingContext *,CShape const &)+2F↑j
.text:00000001801B913A 8B C3                                                           mov     eax, ebx
.text:00000001801B913C 48 8B 5C 24 40                                                  mov     rbx, [rsp+38h+arg_0]
.text:00000001801B9141 48 83 C4 30                                                     add     rsp, 30h
.text:00000001801B9145 5F                                                              pop     rdi
.text:00000001801B9146 C3                                                              retn
.text:00000001801B9146                                                 ?RenderBlackShape@CWindowNode@@AEAAJPEAVCDrawingContext@@AEBVCShape@@@Z endp
*/

int main(int argc, const char* argv[])
{
	(void)argc;
	(void)argv;

	// Version 10.0.17763.973
	// MD5 ea6256b1c5bc75242cc92bbcfd842832 dwmcore.dll

	auto c1(xParseByteArray("48 89 5C 24 08 57 48 83 EC 30 49 8B C0 48 8B FA 48 8B D0 4C 8D 05 CC CC CC CC 48 8B CF E8 CC CC CC CC 8B D8 85 C0 78 09"));
	auto c2(xParseByteArray("48 89 5C 24 08 57 48 83 EC 30 49 8B C0 48 8B FA 48 8B D0 4C 8D 05 CC CC CC CC 48 8B CF B8 00 00 00 00 8B D8 85 C0 78 09"));

	xPatchProcess(L"dwm.exe", c1, c2, NULL, 0);

	xCreateWindow();
	xInitD3d();

	// make content invisible!
	XGUARD_WIN(SetWindowDisplayAffinity(Window, WDA_MONITOR));

	xMainLoop();
	xShutdown();

	return 0;
}

void xCreateWindow()
{
	printf("xCreateWindow\n");

	WNDCLASSEXW wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.lpszClassName = WND_CLASS;
	wc.lpfnWndProc = xWindowProc;
	XGUARD_WIN(RegisterClassEx(&wc));

	#if 1
		DWORD ExStyle = WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW;
		DWORD Style = WS_POPUP;
	#else
		DWORD ExStyle = 0;
		DWORD Style = WS_OVERLAPPEDWINDOW;
	#endif

	Window = CreateWindowEx(
		ExStyle, WND_CLASS, TEXT("hello"), Style,
		0, 0, WND_SIZEX, WND_SIZEY,
		0, 0, GetModuleHandle(NULL), 0
	);
	XGUARD_WIN(Window);

	# if 1
	SetWindowLong(Window, GWL_EXSTYLE, ExStyle | WS_EX_LAYERED);
	SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 255, LWA_ALPHA | LWA_COLORKEY);
	#endif

	ShowWindow(Window, SW_SHOW);
	UpdateWindow(Window);
}

#define CUSTOMFVF (D3DFVF_XYZ | D3DFVF_DIFFUSE)
struct CUSTOMVERTEX
{
	FLOAT x, y, z;
	DWORD color;
};

void xInitD3d()
{
	printf("xInitD3d\n");

	D3d = Direct3DCreate9(D3D_SDK_VERSION);
	XGUARD(D3d);

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.BackBufferWidth = WND_SIZEX;
	d3dpp.BackBufferHeight = WND_SIZEY;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = Window;
	d3dpp.Windowed = TRUE;

	XGUARD_HR(D3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, Window, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &D3dDevice));

	D3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	D3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	#if 0
	D3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	D3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	D3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	D3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	#endif

	// tri

	CUSTOMVERTEX Vertices[] =
	{
		{  2.5f, -3.0f, 0.0f, D3DCOLOR_ARGB(0xff, 0, 0, 255), },
		{  0.0f,  3.0f, 0.0f, D3DCOLOR_ARGB(0xff, 0, 255, 0), },
		{ -2.5f, -3.0f, 0.0f, D3DCOLOR_ARGB(0xff, 255, 0, 0), },
	};

	XGUARD_HR(D3dDevice->CreateVertexBuffer(3*sizeof(CUSTOMVERTEX), 0, CUSTOMFVF, D3DPOOL_MANAGED, &TriBuf, NULL));

	VOID* TriMem = NULL;
	XGUARD_HR(TriBuf->Lock(0, 0, (void**)&TriMem, 0));
	memcpy(TriMem, Vertices, sizeof(Vertices));
	TriBuf->Unlock();

	XGUARD_HR(D3dDevice->SetFVF(CUSTOMFVF));
	XGUARD_HR(D3dDevice->SetStreamSource(0, TriBuf, 0, sizeof(CUSTOMVERTEX)));

	// cam

	D3DVECTOR CamPos = {0.0f, 0.0f, 10.0f};
	D3DVECTOR CamTarg = {0.0f, 0.0f, 0.0f};
	D3DVECTOR CamUp = {0.0f, 1.0f, 0.0f};

	D3DMATRIX View;
	D3DXMatrixLookAtLH(&View, &CamPos, &CamTarg, &CamUp);
	XGUARD_HR(D3dDevice->SetTransform(D3DTS_VIEW, &View));

	D3DMATRIX Proj;
	D3DXMatrixPerspectiveFovLH(&Proj, 45 * 0.01745f, (FLOAT)WND_SIZEX / (FLOAT)WND_SIZEY, 1.0f, 1000.0f);
	XGUARD_HR(D3dDevice->SetTransform(D3DTS_PROJECTION, &Proj));
}

void xMainLoop()
{
	printf("xMainLoop\n");

	UINT FrameId = 0;
	FLOAT Angle = 0;

	using clock = std::chrono::high_resolution_clock;
	clock::time_point t0, t1, t2;

	t0 = clock::now();

	for (;;)
	{
		t1 = clock::now();
		const double dt = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() * 1e-6; // micros -> sec
		t0 = t1;

		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				return;
			}
		}

		D3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xff, 0, 0, 0), 1.0f, 0);
		D3dDevice->BeginScene();

		D3DMATRIX World;
		D3DXMatrixRotationY(&World, Angle);
		XGUARD_HR(D3dDevice->SetTransform(D3DTS_WORLD, &World));
		Angle += 5.0f * dt;

		XGUARD_HR(D3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1));

		D3dDevice->EndScene();
		XGUARD_HR(D3dDevice->Present(NULL, NULL, NULL, NULL));

		FrameId++;

		t2 = clock::now();
		const double elapsed = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() * 0.001; // micros -> millis
		const double rate = 1000 / 60.0;

		if (elapsed < rate)
		{
			Sleep((DWORD)(rate - elapsed));
		}
	}
}

LRESULT CALLBACK xWindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_NCPAINT:
		case WM_ERASEBKGND:
		case WM_PRINT:
		case WM_PRINTCLIENT:
			return 0;

		case WM_NCHITTEST:
			return HTTRANSPARENT;

		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE)
			{
				PostQuitMessage(0);
				return 0;
			}
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void xShutdown()
{
	printf("xShutdown\n");

	TriBuf->Release();
	D3dDevice->Release();
	D3d->Release();

	DestroyWindow(Window);
	UnregisterClass(WND_CLASS, NULL);
}
