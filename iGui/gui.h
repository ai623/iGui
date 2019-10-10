#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <string>
#include <initializer_list>
#include <stdint.h>



#include "common.h"
#include "debug.h"
#include "shape.h"

#define _RELEASE_COM_PTR(p) if(p) {p->Release(); p = nullptr;}

namespace iGui {
	struct Painter;
	namespace _internal {
		extern iCommon::Debug debug;
		extern bool isDebug;

		LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		inline ID3D11Device* getDeviceFrom(Painter& pt);
		inline ID3D11DeviceContext* getContextFrom(Painter& pt);
	}


	struct Nothing {};

	struct DefaultConfigure {
		inline DefaultConfigure() {
			enalbe4xMsaa();
		}

		inline void enalbe4xMsaa() {
			sampleDesc.Count = 4;
			sampleDesc.Quality = 0;
		}

		inline void disable4xMsaa() {
			sampleDesc.Count = 1;
			sampleDesc.Quality = 0;
		}

	private:
		DXGI_SAMPLE_DESC sampleDesc;
		friend struct Painter;
	};

	extern DefaultConfigure defaultConfigure;

	struct iGuiInitializer {
		void operator() ();

		void enableMultiThread();
		void enableDebug();
		void enalbe4xMsaa();
		void disable4xMsaa();
	private:
		int sampleCount = 4;
		friend struct Painter;
	};
	extern iGuiInitializer iGuiInit;

	struct System {
		Rect<int> getScreenResolution();
	};
	extern System system;

	struct Exec {
		void setDefualtRun(void (*func) ()) { defaultRun = func; _isEnableDefaultRun = true; }
		bool isEnableDefaultRun() const { return _isEnableDefaultRun; }
		void enableDefaultRun() { _isEnableDefaultRun = true; }
		void disableDefaultRun() { _isEnableDefaultRun = false; }
		void quit() { PostQuitMessage(0); }
		int operator() ();

	private:
		void (*defaultRun)() = nullptr;
		bool _isEnableDefaultRun = false;
		bool _quitWhenNowindows = true;
	};

	extern Exec exec;

	struct Adapter {
		virtual ~Adapter();
		Adapter() {}
		bool init(int index = 0);

	private:
		IDXGIAdapter* madapter = nullptr;
		friend struct Painter;
	};

	struct Window;
	using Viewport = D3D11_VIEWPORT;
	struct VertexBuffer;
	struct DepthStencilBuffer;
	struct InputLayout;
	struct VertexShader;
	struct PixelShader;

	struct Painter {
		virtual ~Painter();
		Painter(Nothing no) {};
		Painter(const Painter& pt);
		Painter(Painter&& pt);
		Painter() {_init(nullptr, nullptr, false, iGuiInit.sampleCount, _internal::isDebug);}
		Painter(bool debugMode) {_init(nullptr, nullptr, false, iGuiInit.sampleCount, debugMode);}
		Painter(Adapter& adapter) {_init(nullptr, adapter.madapter, false, iGuiInit.sampleCount, _internal::isDebug);}
		Painter(Adapter& adapter, bool debugMode) {_init(nullptr, adapter.madapter, false, iGuiInit.sampleCount, debugMode);}
		Painter(Window& wnd, const Adapter& adapter) {_init(&wnd, adapter.madapter, false, iGuiInit.sampleCount, _internal::isDebug);}
		Painter(Window& wnd, const Adapter& adapter, bool debugMode) {_init(&wnd, adapter.madapter, false, iGuiInit.sampleCount, debugMode);}

		Painter& operator = (const Painter& pt) { copyFrom(pt); return *this; }
		Painter& operator = (Painter&& pt) { moveFrom(std::move(pt)); return *this; }

		void init() { _init(nullptr, nullptr, false, iGuiInit.sampleCount, _internal::isDebug); }
		void init(bool debugMode) { _init(nullptr, nullptr, false, iGuiInit.sampleCount, debugMode); }
		void init(Adapter& adapter) { _init(nullptr, adapter.madapter, false, iGuiInit.sampleCount, _internal::isDebug); }
		void init(Adapter& adapter, bool debugMode) { _init(nullptr, adapter.madapter, false, iGuiInit.sampleCount, debugMode); }
		void init(Window& wnd, const Adapter& adapter) { _init(&wnd, adapter.madapter, false, iGuiInit.sampleCount, _internal::isDebug); }
		void init(Window& wnd, const Adapter& adapter, bool debugMode) { _init(&wnd, adapter.madapter, false, iGuiInit.sampleCount, debugMode); }

		void set(const Viewport& vp);
		void setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology);
		void set(const VertexBuffer& buffer);
		void set(InputLayout& layout);
		void setTarget(Window& wnd, DepthStencilBuffer& buff);
		void set(VertexShader& shader);
		void set(PixelShader& shader);

		void draw() { draw(0); }
		void draw(int startLocation) { mcontext->Draw(vertexElementNum, startLocation); }

		void clearTarget(Window& wnd);

		enum CLEAR_FLAG{
			CLEAR_DEPTH = D3D11_CLEAR_DEPTH,
			CLEAR_STENCIL = D3D11_CLEAR_STENCIL
		};
		void clearTarget(DepthStencilBuffer& buff, unsigned int buffType, float depth, uint8_t stencil);

	private:
		void _init(Window* wnd, IDXGIAdapter* adapter, bool fullScreen, int sampleCount, bool debugMode);
		void copyFrom(const Painter& pt);
		void moveFrom(Painter&& pt);

		inline void releaseAll() {
#define R _RELEASE_COM_PTR
			R(mdevice)R(mcontext)
#undef R
		}

		ID3D11Device* mdevice = nullptr;
		ID3D11DeviceContext* mcontext = nullptr;
		UINT vertexElementNum = 0;
		D3D_FEATURE_LEVEL mlevelGet;

		static const D3D_FEATURE_LEVEL mlevelsWant[];

		friend struct VertexBuffer;
		friend struct DepthStencilBuffer;
		friend struct InputLayout;
		friend struct VertexShader;
		friend struct PixelShader;

		friend ID3D11Device* _internal::getDeviceFrom(Painter&);
		friend ID3D11DeviceContext* _internal::getContextFrom(Painter&);
	};

	namespace _internal {
		inline ID3D11Device* getDeviceFrom(Painter& pt){
			return pt.mdevice;
		}
		inline ID3D11DeviceContext* getContextFrom(Painter& pt) {
			return pt.mcontext;
		}
	}


	struct Window {
		Window();

		virtual ~Window();

		Rect<int> getWindowRect() const;
		static int getWindowsNum();
		Painter& getPainter() { return mpainter; }

		virtual void whenCreate() {};
		virtual void whenDestroy() {};
		virtual void whenPaint() {};

		void present();
	private:
		HWND mhWnd = NULL;
		Painter mpainter = Painter(Nothing());
		IDXGISwapChain* mswapChain = nullptr;
		ID3D11Texture2D* mbackBuffer = nullptr;
		ID3D11RenderTargetView* mtargetView = nullptr;

		inline void releaseAll() {
#define R _RELEASE_COM_PTR
			R(mswapChain) R(mbackBuffer)R(mtargetView)
#undef R
		}

		static int wndNum;
		friend LRESULT CALLBACK _internal::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		friend struct Painter;
		friend struct DepthStencilBuffer;
	};

	struct VertexBufferDesc {
		D3D11_USAGE usage;
		UINT cpuAccessFlags;
		UINT miscFlags;
	};
	
	struct VertexBuffer {
		VertexBuffer(Nothing no){}
		VertexBuffer(const VertexBuffer& vbuff) { copyFrom(vbuff); }
		VertexBuffer(VertexBuffer&& vbuff) { moveFrom(std::move(vbuff)); }

		void operator = (const VertexBuffer& vbuff) { copyFrom(vbuff); }
		void operator = (VertexBuffer&& vbuff) { moveFrom(std::move(vbuff)); }

		virtual ~VertexBuffer() { releaseAll(); }
		template<typename T>
		inline VertexBuffer(Painter& painter, T* data, int eleNum, const VertexBufferDesc& desc) {
			if (sizeof(T) > 255)_internal::debug.error("Too Large Structure Type");
			meleSize = sizeof(T);
			initOther(painter, data, eleNum, desc);
		}
	private:
		inline void releaseAll() {
#define R _RELEASE_COM_PTR
			R(mvertexBuffer);
#undef R
		}

		void initOther(Painter& painter, void* data, int eleNum, const VertexBufferDesc& desc);

		void copyFrom(const VertexBuffer& vbuff);
		void moveFrom(VertexBuffer&& vbuff);

		ID3D11Buffer* mvertexBuffer = nullptr;
		UINT meleSize = 0;
		int melementNum = 0;
		
		friend struct Painter;
	};

	struct DepthStencilBuffer {
		virtual ~DepthStencilBuffer();

		DepthStencilBuffer(Nothing no) {}
		DepthStencilBuffer(DepthStencilBuffer&& buff) { moveFrom(std::move(buff)); }
		DepthStencilBuffer(Window& wnd);
		DepthStencilBuffer& operator=(DepthStencilBuffer&& buff) { moveFrom(std::move(buff)); return *this; }

	private:
		ID3D11Texture2D* mdsBuff = nullptr;
		ID3D11DepthStencilView* mdsView = nullptr;

		void moveFrom(DepthStencilBuffer&& buff);
		inline void releaseAll() {
#define R _RELEASE_COM_PTR
			R(mdsBuff) R(mdsView)
#undef R
		}
		friend struct Painter;
	};

	using InputElementDesc = D3D11_INPUT_ELEMENT_DESC;	

	struct InputLayout {
		virtual ~InputLayout() { releaseAll(); }
		InputLayout(Nothing){}
		InputLayout(InputLayout&& ly) { moveFrom(std::move(ly)); }
		InputLayout(Painter& painter, VertexShader& vs, InputElementDesc* arr, int eleNum);

		InputLayout& operator = (InputLayout&& ly) { moveFrom(std::move(ly)); return *this; }

	private:
		inline void releaseAll() {
#define R _RELEASE_COM_PTR
			R(mlayout);
#undef R
		}

		void moveFrom(InputLayout&& ly);

		ID3D11InputLayout* mlayout = nullptr;
		
		friend struct Painter;
	};

	struct VertexShader {
		virtual ~VertexShader() { releaseAll(); }
		VertexShader(Nothing){}
		VertexShader(Painter& painter, std::wstring path);
		VertexShader(const VertexShader& v) { copyFrom(v); }
		VertexShader(VertexShader&& v) { moveFrom(std::move(v)); }

		VertexShader& operator = (const VertexShader& v) { copyFrom(v); return *this; }
		VertexShader& operator = (VertexShader&& v) { moveFrom(std::move(v)); return *this; }

		bool hasLoaded() { return mvertexShader; }
	private:
		ID3DBlob* mvsFile = nullptr;
		ID3D11VertexShader* mvertexShader = nullptr;

		inline void releaseAll() {
#define R _RELEASE_COM_PTR
			R(mvertexShader)R(mvsFile)
#undef R
		}

		void copyFrom(const VertexShader& v);
		void moveFrom(VertexShader&& v);


		friend struct Painter;
		friend struct InputLayout;
	};

	struct PixelShader {
		~PixelShader() { releaseAll(); }
		PixelShader(Nothing){}
		PixelShader(Painter& painter, std::wstring path);
		PixelShader(PixelShader&& p) { moveFrom(std::move(p)); }
		
		PixelShader& operator = (PixelShader&& p) { moveFrom(std::move(p)); return *this; }

		bool hasLoaded() { return mpixelShader; }
	private:
		inline void releaseAll() {
#define R _RELEASE_COM_PTR
			R(mpixelShader)
#undef R
		}
		void moveFrom(PixelShader&& p);

		ID3D11PixelShader* mpixelShader = nullptr;
		friend struct Painter;
	};

	struct PainterBackup;

	struct PainterSetterInfo {

	};

	struct PainterSetter {
		virtual void set(Painter&) = 0;
		virtual void backup(Painter&, PainterBackup&) = 0;
		virtual const PainterSetterInfo& getInfo() = 0;
	};

	struct PainterBackup : PainterSetter {
	};
}


int guiMain();

#undef _RELEASE_COM_PTR