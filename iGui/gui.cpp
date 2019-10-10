#include "gui.h"

namespace iGui {
	DefaultConfigure defaultConfigure;
	iGuiInitializer iGuiInit;
	System system;
	Exec exec;

	int Window::wndNum = 0;
	const D3D_FEATURE_LEVEL Painter::mlevelsWant[] = { D3D_FEATURE_LEVEL_11_0};

	namespace _internal {
		iCommon::Debug debug;
		HINSTANCE hInstance;
		PWSTR pCmdLine;
		int nCmdShow;
		const wchar_t wcName[] = L"BaseClass";

		bool isEnableMultiThread = false;
		bool isDebug = false;

		IDXGIFactory* dxgiFactory = nullptr;

		LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			Window* wnd = (Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			switch (msg)
			{
			case WM_PAINT: 
			{
				wnd->whenPaint();
				break;
			}
			case WM_CREATE:
			{
				wnd = (Window*)(((CREATESTRUCT*)lParam)->lpCreateParams);
				SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)wnd);
				wnd->mhWnd = hWnd;
				Window::wndNum++;
				Painter(*wnd, Adapter());

				//wnd->whenCreate();
				return 0;
			}
			case WM_DESTROY:
			{
				
				wnd->whenDestroy();
				wnd->releaseAll();
				wnd->mhWnd = NULL;
				Window::wndNum--;
				return 0;
			}
			default:
				break;
			}

			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
	}
	using _internal::debug;


	void iGuiInitializer::enableMultiThread()
	{
		_internal::isEnableMultiThread = true;
	}

	void iGuiInitializer::enableDebug()
	{
		_internal::isDebug = true;
	}

	void iGuiInitializer::operator()()
	{
	}

	void iGuiInitializer::enalbe4xMsaa()
	{
		sampleCount = 4;
	}

	void iGuiInitializer::disable4xMsaa()
	{
		sampleCount = 1;
	}

	int Exec::operator()()
	{
		MSG msg = { };
		while (msg.message != WM_QUIT)
		{
			if (_isEnableDefaultRun) {
				// If there are Window messages then process them.
				if (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessageW(&msg);
				}
				else
				{
					if (defaultRun) {
						defaultRun();
					}
				}
			}
			else {
				GetMessageW(&msg, NULL, 0, 0);
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
		}
		return msg.wParam;
	}

	Window::Window()
	{
		mhWnd = CreateWindowExW(
			0,                              // Optional window styles.
			_internal::wcName,                     // Window class
			L"",    // Window text
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,            // Window style

			// Size and position
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

			NULL,       // Parent window    
			NULL,       // Menu
			_internal::hInstance,  // Instance handle
			this        // Additional application data
		);
		if (mhWnd == NULL) {
			debug.error("Fail to CreateWindowExW");
		}
		InvalidateRect(mhWnd, NULL, FALSE);
		
	}

	Window::~Window()
	{
		if (mhWnd) {
			DestroyWindow(mhWnd);
			return;
		}
		releaseAll();
	}

	Rect<int> Window::getWindowRect() const
	{
		Rect<int> re{};
		if (mhWnd) {
			RECT rect;
			if (GetClientRect(mhWnd, &rect)) {
				re.width = rect.right - rect.left;
				re.height = rect.bottom - rect.top;
				return re;
			}
		}
		return re;
	}

	int Window::getWindowsNum()
	{
		return wndNum;
	}

	void Window::present()
	{
		auto re = mswapChain->Present(0, 0);
	}

	Painter::~Painter()
	{
#define R(p) if(p) p->Release()
		R(mdevice);
		R(mcontext);
#undef R
	}

	Painter::Painter(const Painter& pt)
	{
		(*this) = pt;
	}

	Painter::Painter(Painter&& pt)
	{
		*this = std::move(pt);
	}

	void Painter::set(const Viewport& vp)
	{
		mcontext->RSSetViewports(1, &vp);
	}

	void Painter::setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology)
	{
		mcontext->IASetPrimitiveTopology(topology);
	}

	void Painter::set(const VertexBuffer& buffer)
	{
		vertexElementNum = buffer.melementNum;
		ID3D11Buffer* buffs[]{ buffer.mvertexBuffer };
		UINT strides[]{ buffer.meleSize };
		UINT offsets[]{ 0 };
		mcontext->IASetVertexBuffers(0, 1, buffs, strides, offsets);
	}

	void Painter::set(InputLayout& layout)
	{
		mcontext->IASetInputLayout(layout.mlayout);
	}

	void Painter::setTarget(Window& wnd, DepthStencilBuffer& buff)
	{
		mcontext->OMSetRenderTargets(1, &wnd.mtargetView, buff.mdsView);
	}



	void Painter::set(VertexShader& shader)
	{
		mcontext->VSSetShader(shader.mvertexShader, nullptr, 0);
	}

	void Painter::set(PixelShader& shader)
	{
		mcontext->PSSetShader(shader.mpixelShader,nullptr,0);
	}


	void Painter::clearTarget(Window& wnd)
	{
		FLOAT color[] = { 0,0,0,0 };
		mcontext->ClearRenderTargetView(wnd.mtargetView, color);
	}

	void Painter::clearTarget(DepthStencilBuffer& buff, unsigned int buffType, float depth, uint8_t stencil)
	{
		mcontext->ClearDepthStencilView(buff.mdsView, buffType, depth, stencil);
	}

	void Painter::_init(Window* wnd, IDXGIAdapter* adapter, bool fullScreen, int sampleCount, bool debugMode)
	{
		HRESULT hr;
		releaseAll();

		UINT flags{};
		if (!_internal::isEnableMultiThread) {
			flags |= D3D11_CREATE_DEVICE_SINGLETHREADED;
		}

		if (debugMode) {
			flags |= D3D11_CREATE_DEVICE_DEBUG;
		}

		D3D_DRIVER_TYPE driverType;
		if (adapter == nullptr) {
			driverType = D3D_DRIVER_TYPE_HARDWARE;
		}
		else {
			driverType = D3D_DRIVER_TYPE_UNKNOWN;
		}

		if (wnd) {
			DXGI_SWAP_CHAIN_DESC desc;
			{
				DXGI_MODE_DESC& bufferDesc = desc.BufferDesc;		//指定显示buffer的属性，通常在全屏时才生效

				if (fullScreen) {
					auto rect = system.getScreenResolution();
					bufferDesc.Width = rect.width;
					bufferDesc.Height = rect.height;
					desc.Windowed = FALSE;
				}
				else {
					bufferDesc.Width = 0;												//0表示自动获取目标窗口的宽度
					bufferDesc.Height = 0;
					desc.Windowed = TRUE;
				}

				bufferDesc.RefreshRate = DXGI_RATIONAL{ 144,1 };					//刷新率(Hz)，是分数
				bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;						//buffer中元素类型
				bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;	//光栅顺序	
				bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;					//指定缩放模式，未指定表示使用原始分辨率

				desc.SampleDesc.Count= sampleCount;	//指定多重采样属性
				desc.SampleDesc.Quality = 0;

				desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;		//指定用途
				desc.BufferCount = 1;									//指定front buffer的数量
				desc.OutputWindow = wnd->mhWnd;								//指定输出窗口
				desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;				//指定将画面传递到DWM的方式，DISCARD表示让系统自动选择

				desc.Flags = 0;			//包含一些显示模式，兼容性，隐私安全的设置
			}


			if (wnd->mswapChain) {
				wnd->mswapChain->Release();
			}
			
			hr = D3D11CreateDeviceAndSwapChain(
				adapter,														//显卡,nullptr使用默认显卡
				driverType,										//硬渲染
				NULL,															//软件模块，如果使用软件实现的话
				flags,	//是否debug和单线程
				mlevelsWant,														//想要的d3d11版本，是个数组，将会依次遍历，如果支持则立即返回
				ARRAYSIZE(mlevelsWant),											//想要的d3d11版本数量
				D3D11_SDK_VERSION,												//必须是该值
				&desc,
				&wnd->mswapChain,
				&mdevice,														//
				&mlevelGet,														//得到的d3d11版本
				&mcontext
			);
			if (FAILED(hr)) debug.error("Fail to D3D11CreateDeviceAndSwapChain", hr);

			hr = wnd->mswapChain->GetBuffer(0, IID_PPV_ARGS(&wnd->mbackBuffer));
			if (FAILED(hr))debug.error("backBuffer创建失败", hr);
			hr = mdevice->CreateRenderTargetView(wnd->mbackBuffer, nullptr, &wnd->mtargetView);
			if (FAILED(hr)) debug.error("targetView创建失败", hr);
			wnd->mpainter = *this;
		}
		else {
			hr = D3D11CreateDevice(
				adapter,														//显卡,nullptr使用默认显卡
				driverType,										//硬渲染
				NULL,															//软件模块，如果使用软件实现的话
				flags,	//是否debug和单线程
				mlevelsWant,														//想要的d3d11版本，是个数组，将会依次遍历，如果支持则立即返回
				ARRAYSIZE(mlevelsWant),											//想要的d3d11版本数量
				D3D11_SDK_VERSION,												//必须是该值
				&mdevice,														//
				&mlevelGet,														//得到的d3d11版本
				&mcontext														//
			);
			if (FAILED(hr)) debug.error("Fail to D3D11CreateDevice", hr);
		}
		
	}

	void Painter::copyFrom(const Painter& pt)
	{
		releaseAll();
		mdevice = pt.mdevice;
		mcontext = pt.mcontext;
		if (mcontext)mcontext->AddRef();
		if (mdevice)mdevice->AddRef();
		mlevelGet = pt.mlevelGet;
		vertexElementNum = pt.vertexElementNum;
	}

	void Painter::moveFrom(Painter&& pt)
	{
		releaseAll();
		mdevice = pt.mdevice;
		mcontext = pt.mcontext;
		mlevelGet = pt.mlevelGet;
		pt.mdevice = nullptr;
		pt.mcontext = nullptr;
		mlevelGet = pt.mlevelGet;
		vertexElementNum = pt.vertexElementNum;
	}





	Adapter::~Adapter()
	{
		if (madapter)madapter->Release();
	}

	bool Adapter::init(int index)
	{
		using _internal::dxgiFactory;
		HRESULT hr;
		hr = dxgiFactory->EnumAdapters(index, &madapter);
		if (FAILED(hr)) return false;
		return true;
	}



	DepthStencilBuffer::~DepthStencilBuffer()
	{
		releaseAll();
	}

	DepthStencilBuffer::DepthStencilBuffer(Window& wnd)
	{
		HRESULT hr;
		D3D11_TEXTURE2D_DESC desc;
		wnd.mbackBuffer->GetDesc(&desc);
		auto device = wnd.mpainter.mdevice;

		//uncertain
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		//

		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;	

		hr = device->CreateTexture2D(&desc, nullptr, &mdsBuff);
		if (FAILED(hr))debug.error("Fail to CreateTexture2D", hr);
		device->CreateDepthStencilView(mdsBuff, nullptr, &mdsView);
		if (FAILED(hr))debug.error("Fail to CreateDepthStencilView", hr);
	}

	void DepthStencilBuffer::moveFrom(DepthStencilBuffer&& buff)
	{
		releaseAll();
		mdsBuff = buff.mdsBuff;
		mdsView = buff.mdsView;
		buff.mdsBuff = nullptr;
		buff.mdsView = nullptr;
	}

	VertexShader::VertexShader(Painter& painter, std::wstring path)
	{
		HRESULT hr = D3DReadFileToBlob(path.c_str(), &mvsFile);
		if (FAILED(hr)) { return; }
		
		hr = painter.mdevice->CreateVertexShader(mvsFile->GetBufferPointer(), mvsFile->GetBufferSize(), nullptr, &mvertexShader);
		if (FAILED(hr)) { mvsFile->Release(); mvsFile = nullptr; return; }
	}

	void VertexShader::copyFrom(const VertexShader& v)
	{
		releaseAll();
		mvsFile = v.mvsFile;
		mvertexShader = v.mvertexShader;
		if(mvsFile) mvsFile->AddRef();
		if(mvertexShader) mvertexShader->AddRef();
	}

	void VertexShader::moveFrom(VertexShader&& v)
	{
		releaseAll();
		mvertexShader = v.mvertexShader;
		mvsFile = v.mvsFile;
		v.mvertexShader = nullptr;
		v.mvsFile = nullptr;
	}

	PixelShader::PixelShader(Painter& painter, std::wstring path)
	{
		HRESULT hr;
		ID3DBlob* psFile;
		hr = D3DReadFileToBlob(path.c_str(), &psFile);
		if (FAILED(hr)) return;

		hr = painter.mdevice->CreatePixelShader(psFile->GetBufferPointer(), psFile->GetBufferSize(), NULL, &mpixelShader);
		if (FAILED(hr)) return;
		psFile->Release();
	}

	void PixelShader::moveFrom(PixelShader&& p)
	{
		releaseAll();
		mpixelShader = p.mpixelShader;
		p.mpixelShader = nullptr;
	}

	InputLayout::InputLayout(Painter& painter, VertexShader& vs, InputElementDesc* arr, int eleNum)
	{
		HRESULT hr;
		hr = painter.mdevice->CreateInputLayout(arr, eleNum, vs.mvsFile->GetBufferPointer(), vs.mvsFile->GetBufferSize(), &mlayout);
		if (FAILED(hr))debug.error("Fail to CreateInputLayout");
	}

	void InputLayout::moveFrom(InputLayout&& ly)
	{
		releaseAll();
		mlayout = ly.mlayout;
		ly.mlayout = nullptr;
	}

	void VertexBuffer::initOther(Painter& painter, void* data, int eleNum, const VertexBufferDesc& desc)
	{
		melementNum = eleNum;
		D3D11_BUFFER_DESC d3dDesc{};
		d3dDesc.ByteWidth = melementNum*meleSize;
		d3dDesc.Usage = desc.usage;
		d3dDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		d3dDesc.CPUAccessFlags = desc.cpuAccessFlags;
		d3dDesc.MiscFlags = desc.miscFlags;
		//d3dDesc.StructureByteStride = 0;
		//d3dDesc.StructureByteStride = eleSize;

		D3D11_SUBRESOURCE_DATA srData{};
		srData.pSysMem = data;

		HRESULT hr;
		hr = painter.mdevice->CreateBuffer(&d3dDesc, &srData, &mvertexBuffer);
		if (FAILED(hr)) debug.error("Fail to CreateBuffer", hr);
	}

	void VertexBuffer::copyFrom(const VertexBuffer& vbuff)
	{
		releaseAll();
		mvertexBuffer = vbuff.mvertexBuffer;
		if(mvertexBuffer)mvertexBuffer->AddRef();
		meleSize = vbuff.meleSize;
		melementNum = vbuff.melementNum;
	}

	void VertexBuffer::moveFrom(VertexBuffer&& vbuff)
	{
		releaseAll();
		mvertexBuffer = vbuff.mvertexBuffer;
		vbuff.mvertexBuffer = nullptr;
		meleSize = vbuff.meleSize;
		melementNum = vbuff.melementNum;
	}



	Rect<int> System::getScreenResolution()
	{
		return Rect<int>{GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
	}

}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
	{
		using namespace iGui;
		//Initialize parameters
		_internal::hInstance = hInstance;
		_internal::pCmdLine = pCmdLine;
		_internal::nCmdShow = nCmdShow;

		//Register WNDCLASS
		WNDCLASSW wc{};
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = _internal::WndProc;
		wc.hInstance = hInstance;
		wc.lpszClassName = _internal::wcName;
		
		if (!RegisterClassW(&wc)) {
			debug.error("Fail to Register WNDCLASS");
		}

		//initialize d3d

		HRESULT hr;

		//intialize dxgiFactory
		hr = CreateDXGIFactory(IID_PPV_ARGS(&_internal::dxgiFactory));
		if (FAILED(hr)) debug.error("Fail to create IDXGIFactory", hr);
	}
	
	auto re = guiMain();

	{
		using namespace iGui;
		_internal::dxgiFactory->Release();
	}
	auto leak = iGui::debug.hasMemoryLeaks();
	if (leak) {
		iGui::debug.error("Has Memory Leak!!!");
	}

	return re;
}
