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

namespace iGui {
	struct Painter;
	namespace _internal {
		LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		inline ID3D11Device* getDeviceFrom(Painter& pt);
		inline ID3D11DeviceContext* getContextFrom(Painter& pt);
	}
	extern iCommon::Debug debug;

	struct Nothing {

	};

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
		IDXGIAdapter* adapter = nullptr;
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
		Painter(const Painter& pt);
		Painter(Painter&& pt);
		Painter(Nothing no) {};
		Painter();
		Painter(bool debugMode);
		Painter(Adapter& adapter);
		Painter(Adapter& adapter, bool debugMode);
		Painter(Window& wnd, const Adapter& adapter);
		Painter(Window& wnd, const Adapter& adapter, bool debugMode);

		Painter& operator = (const Painter& pt);
		Painter& operator = (Painter&& pt);

		

		void set(const Viewport& vp);
		void setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology);
		void set(const VertexBuffer& buffer);
		void set(InputLayout& layout);
		void setTarget(Window& wnd, DepthStencilBuffer& buff);
		void set(VertexShader& shader);
		void set(PixelShader& shader);

		void draw();

		void clearTarget(Window& wnd);

		enum {
			CLEAR_DEPTH = D3D11_CLEAR_DEPTH,
			CLEAR_STENCIL = D3D11_CLEAR_STENCIL
		};
		void clearTarget(DepthStencilBuffer& buff, unsigned int buffType, float depth, uint8_t stencil);

	private:
		void init(Window* wnd, IDXGIAdapter* adapter, bool fullScreen, int sampleCount, bool debugMode);

		inline void releaseAll() {
#define R(x) if(x) {x->Release(); x = nullptr;}
			R(device)R(context)
#undef R
		}

		ID3D11Device* device = nullptr;
		ID3D11DeviceContext* context = nullptr;
		UINT vertexElementNum = 0;
		D3D_FEATURE_LEVEL levelGet;

		static const D3D_FEATURE_LEVEL levelsWant[];

		friend struct VertexBuffer;
		friend struct DepthStencilBuffer;
		friend struct InputLayout;
		friend struct VertexShader;
		friend struct PixelShader;

		friend ID3D11Device* _internal::getDeviceFrom(Painter&);
		friend ID3D11DeviceContext* _internal::getContextFrom(Painter&);
	};

	namespace _internal {
		inline ID3D11Device* getDeviceFrom(Painter& pt) {
			return pt.device;
		}

		inline ID3D11DeviceContext* getContextFrom(Painter& pt) {
			return pt.context;
		}
	}


	struct Window {
		Window();

		virtual ~Window();

		Rect<int> getWindowRect() const;
		static int getWindowsNum();
		Painter& getPainter() { return painter; }

		virtual void whenCreate() {};
		virtual void whenDestroy() {};
		virtual void whenPaint() {};

		void present();
	private:
		HWND hWnd = NULL;
		Painter painter = Painter(Nothing());
		IDXGISwapChain* swapChain = nullptr;
		ID3D11Texture2D* backBuffer = nullptr;
		ID3D11RenderTargetView* targetView = nullptr;

		inline void releaseAll() {
#define R(x) if(x) {x->Release(); x = nullptr;}
			R(swapChain) R(backBuffer)R(targetView)
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
			if (sizeof(T) > 255)debug.error("Too Large Structure Type");
			eleSize = sizeof(T);
			initOther(painter, data, eleNum, desc);
		}
	private:
		inline void releaseAll() {
#define R(x) if(x) {x->Release(); x = nullptr;}
			R(vb);
#undef R
		}

		void initOther(Painter& painter, void* data, int eleNum, const VertexBufferDesc& desc);

		void copyFrom(const VertexBuffer& vbuff);
		void moveFrom(VertexBuffer&& vbuff);

		ID3D11Buffer* vb = nullptr;
		UINT eleSize = 0;
		int elementNum = 0;
		
		friend struct Painter;
	};

	struct DepthStencilBuffer {
		virtual ~DepthStencilBuffer();

		DepthStencilBuffer(Nothing no) {}
		DepthStencilBuffer(DepthStencilBuffer&& buff) { moveFrom(std::move(buff)); }
		DepthStencilBuffer(Window& wnd);
		DepthStencilBuffer& operator=(DepthStencilBuffer&& buff) { moveFrom(std::move(buff)); return *this; }

	private:
		ID3D11Texture2D* dsBuff = nullptr;
		ID3D11DepthStencilView* dsView = nullptr;

		void moveFrom(DepthStencilBuffer&& buff);
		inline void releaseAll() {
#define R(x) if(x) {x->Release(); x = nullptr;}
			R(dsBuff) R(dsView)
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
#define R(x) if(x) {x->Release(); x = nullptr;}
			R(layout);
#undef R
		}

		void moveFrom(InputLayout&& ly);

		ID3D11InputLayout* layout = nullptr;
		
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

		bool hasLoaded() { return vs; }
	private:
		ID3DBlob* vsFile = nullptr;
		ID3D11VertexShader* vs = nullptr;

		inline void releaseAll() {
#define R(x) if(x) {x->Release(); x = nullptr;}
			R(vs)R(vsFile)
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

		bool hasLoaded() { return ps; }
	private:
		inline void releaseAll() {
#define R(x) if(x) {x->Release(); x = nullptr;}
			R(ps)
#undef R
		}
		void moveFrom(PixelShader&& p);

		ID3D11PixelShader* ps = nullptr;
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