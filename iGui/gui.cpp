#include "gui.h"

namespace iGui {

	DefaultConfigure defaultConfigure;
	iGuiInitializer iGuiInit;
	System system;
	Exec exec;
	iCommon::Debug debug;

	int Window::wndNum = 0;
	const D3D_FEATURE_LEVEL Painter::levelsWant[] = { D3D_FEATURE_LEVEL_11_0};

	namespace _internal {
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
				wnd->hWnd = hWnd;
				Window::wndNum++;
				Painter(*wnd, Adapter());

				//wnd->whenCreate();
				return 0;
			}
			case WM_DESTROY:
			{
				
				wnd->whenDestroy();
				wnd->releaseAll();
				wnd->hWnd = NULL;
				Window::wndNum--;
				return 0;
			}
			default:
				break;
			}

			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
	}



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
		hWnd = CreateWindowExW(
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
		if (hWnd == NULL) {
			debug.error("Fail to CreateWindowExW");
		}
		InvalidateRect(hWnd, NULL, FALSE);
		
	}

	Window::~Window()
	{
		if (hWnd) {
			DestroyWindow(hWnd);
			return;
		}
		releaseAll();
	}

	Rect<int> Window::getWindowRect() const
	{
		Rect<int> re{};
		if (hWnd) {
			RECT rect;
			if (GetClientRect(hWnd, &rect)) {
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
		auto re = swapChain->Present(0, 0);
	}

	Painter::~Painter()
	{
#define R(p) if(p) p->Release()
		R(device);
		R(context);
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



	Painter::Painter()
	{
		init(nullptr, nullptr, false, iGuiInit.sampleCount, _internal::isDebug);
	}

	Painter::Painter(bool debugMode)
	{
		init(nullptr, nullptr, false, iGuiInit.sampleCount, debugMode);
	}

	Painter::Painter(Adapter& adapter) {
		init(nullptr, adapter.adapter, false, iGuiInit.sampleCount, _internal::isDebug);
	}


	Painter::Painter(Adapter& adapter, bool debugMode)
	{
		init(nullptr, adapter.adapter, false, iGuiInit.sampleCount, debugMode);
	}

	Painter::Painter(Window& wnd, const Adapter& adapter) 
	{
		init(&wnd, adapter.adapter, false, iGuiInit.sampleCount, _internal::isDebug);
	}

	Painter::Painter(Window& wnd, const Adapter& adapter, bool debugMode) {
		init(&wnd, adapter.adapter, false, iGuiInit.sampleCount, debugMode);
	}

	Painter& Painter::operator=(const Painter& pt)
	{
		releaseAll();

		device = pt.device;
		context = pt.context;
		if(context)context->AddRef();
		if(device)device->AddRef();
		levelGet = pt.levelGet;
		vertexElementNum = pt.vertexElementNum;
		return *this;
	}

	Painter& Painter::operator=(Painter&& pt)
	{
		releaseAll();

		device = pt.device;
		context = pt.context;
		levelGet = pt.levelGet;
		pt.device = nullptr;
		pt.context = nullptr;
		levelGet = pt.levelGet;
		vertexElementNum = pt.vertexElementNum;
		return *this;
	}

	void Painter::set(const Viewport& vp)
	{
		context->RSSetViewports(1, &vp);
	}

	void Painter::setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology)
	{
		context->IASetPrimitiveTopology(topology);
	}

	void Painter::set(const VertexBuffer& buffer)
	{
		vertexElementNum = buffer.elementNum;
		ID3D11Buffer* buffs[]{ buffer.vb };
		UINT strides[]{ buffer.eleSize };
		UINT offsets[]{ 0 };
		context->IASetVertexBuffers(0, 1, buffs, strides, offsets);
	}

	void Painter::set(InputLayout& layout)
	{
		context->IASetInputLayout(layout.layout);
	}

	void Painter::setTarget(Window& wnd, DepthStencilBuffer& buff)
	{
		context->OMSetRenderTargets(1, &wnd.targetView, buff.dsView);
	}



	void Painter::set(VertexShader& shader)
	{
		context->VSSetShader(shader.vs, nullptr, 0);
	}

	void Painter::set(PixelShader& shader)
	{
		context->PSSetShader(shader.ps,nullptr,0);
	}

	void Painter::draw()
	{
		context->Draw(vertexElementNum, 0);
		
	}

	void Painter::clearTarget(Window& wnd)
	{
		FLOAT color[] = { 0,0,0,0 };
		context->ClearRenderTargetView(wnd.targetView, color);
	}

	void Painter::clearTarget(DepthStencilBuffer& buff, unsigned int buffType, float depth, uint8_t stencil)
	{
		context->ClearDepthStencilView(buff.dsView, buffType, depth, stencil);
	}

	void Painter::init(Window* wnd, IDXGIAdapter* adapter, bool fullScreen, int sampleCount, bool debugMode)
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
				desc.OutputWindow = wnd->hWnd;								//指定输出窗口
				desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;				//指定将画面传递到DWM的方式，DISCARD表示让系统自动选择

				desc.Flags = 0;			//包含一些显示模式，兼容性，隐私安全的设置
			}


			if (wnd->swapChain) {
				wnd->swapChain->Release();
			}
			
			hr = D3D11CreateDeviceAndSwapChain(
				adapter,														//显卡,nullptr使用默认显卡
				driverType,										//硬渲染
				NULL,															//软件模块，如果使用软件实现的话
				flags,	//是否debug和单线程
				levelsWant,														//想要的d3d11版本，是个数组，将会依次遍历，如果支持则立即返回
				ARRAYSIZE(levelsWant),											//想要的d3d11版本数量
				D3D11_SDK_VERSION,												//必须是该值
				&desc,
				&wnd->swapChain,
				&device,														//
				&levelGet,														//得到的d3d11版本
				&context
			);
			if (FAILED(hr)) debug.error("Fail to D3D11CreateDeviceAndSwapChain", hr);

			hr = wnd->swapChain->GetBuffer(0, IID_PPV_ARGS(&wnd->backBuffer));
			if (FAILED(hr))debug.error("backBuffer创建失败", hr);
			hr = device->CreateRenderTargetView(wnd->backBuffer, nullptr, &wnd->targetView);
			if (FAILED(hr)) debug.error("targetView创建失败", hr);
			wnd->painter = *this;
		}
		else {
			hr = D3D11CreateDevice(
				adapter,														//显卡,nullptr使用默认显卡
				driverType,										//硬渲染
				NULL,															//软件模块，如果使用软件实现的话
				flags,	//是否debug和单线程
				levelsWant,														//想要的d3d11版本，是个数组，将会依次遍历，如果支持则立即返回
				ARRAYSIZE(levelsWant),											//想要的d3d11版本数量
				D3D11_SDK_VERSION,												//必须是该值
				&device,														//
				&levelGet,														//得到的d3d11版本
				&context														//
			);
			if (FAILED(hr)) debug.error("Fail to D3D11CreateDevice", hr);
		}
		
	}





	Adapter::~Adapter()
	{
		if (adapter)adapter->Release();
	}

	bool Adapter::init(int index)
	{
		using _internal::dxgiFactory;
		HRESULT hr;
		hr = dxgiFactory->EnumAdapters(index, &adapter);
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
		wnd.backBuffer->GetDesc(&desc);
		auto device = wnd.painter.device;

		//uncertain
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		//

		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;	

		hr = device->CreateTexture2D(&desc, nullptr, &dsBuff);
		if (FAILED(hr))debug.error("Fail to CreateTexture2D", hr);
		device->CreateDepthStencilView(dsBuff, nullptr, &dsView);
		if (FAILED(hr))debug.error("Fail to CreateDepthStencilView", hr);
	}

	void DepthStencilBuffer::moveFrom(DepthStencilBuffer&& buff)
	{
		releaseAll();
		dsBuff = buff.dsBuff;
		dsView = buff.dsView;
		buff.dsBuff = nullptr;
		buff.dsView = nullptr;
	}

	VertexShader::VertexShader(Painter& painter, std::wstring path)
	{
		HRESULT hr = D3DReadFileToBlob(path.c_str(), &vsFile);
		if (FAILED(hr)) { return; }
		
		hr = painter.device->CreateVertexShader(vsFile->GetBufferPointer(), vsFile->GetBufferSize(), nullptr, &vs);
		if (FAILED(hr)) { vsFile->Release(); vsFile = nullptr; return; }
	}

	void VertexShader::copyFrom(const VertexShader& v)
	{
		releaseAll();
		vsFile = v.vsFile;
		vs = v.vs;
		if(vsFile) vsFile->AddRef();
		if(vs) vs->AddRef();
	}

	void VertexShader::moveFrom(VertexShader&& v)
	{
		releaseAll();
		vs = v.vs;
		vsFile = v.vsFile;
		v.vs = nullptr;
		v.vsFile = nullptr;
	}

	PixelShader::PixelShader(Painter& painter, std::wstring path)
	{
		HRESULT hr;
		ID3DBlob* psFile;
		hr = D3DReadFileToBlob(path.c_str(), &psFile);
		if (FAILED(hr)) return;

		hr = painter.device->CreatePixelShader(psFile->GetBufferPointer(), psFile->GetBufferSize(), NULL, &ps);
		if (FAILED(hr)) return;
		psFile->Release();
	}

	void PixelShader::moveFrom(PixelShader&& p)
	{
		releaseAll();
		ps = p.ps;
		p.ps = nullptr;
	}

	InputLayout::InputLayout(Painter& painter, VertexShader& vs, InputElementDesc* arr, int eleNum)
	{
		HRESULT hr;
		hr = painter.device->CreateInputLayout(arr, eleNum, vs.vsFile->GetBufferPointer(), vs.vsFile->GetBufferSize(), &layout);
		if (FAILED(hr))debug.error("Fail to CreateInputLayout");
	}

	void InputLayout::moveFrom(InputLayout&& ly)
	{
		releaseAll();
		layout = ly.layout;
		ly.layout = nullptr;
	}

	void VertexBuffer::initOther(Painter& painter, void* data, int eleNum, const VertexBufferDesc& desc)
	{
		elementNum = eleNum;
		D3D11_BUFFER_DESC d3dDesc{};
		d3dDesc.ByteWidth = elementNum*eleSize;
		d3dDesc.Usage = desc.usage;
		d3dDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		d3dDesc.CPUAccessFlags = desc.cpuAccessFlags;
		d3dDesc.MiscFlags = desc.miscFlags;
		//d3dDesc.StructureByteStride = 0;
		//d3dDesc.StructureByteStride = eleSize;

		D3D11_SUBRESOURCE_DATA srData{};
		srData.pSysMem = data;

		HRESULT hr;
		hr = painter.device->CreateBuffer(&d3dDesc, &srData, &vb);
		if (FAILED(hr)) debug.error("Fail to CreateBuffer", hr);
	}

	void VertexBuffer::copyFrom(const VertexBuffer& vbuff)
	{
		releaseAll();
		vb = vbuff.vb;
		if(vb)vb->AddRef();
		eleSize = vbuff.eleSize;
		elementNum = vbuff.elementNum;
	}

	void VertexBuffer::moveFrom(VertexBuffer&& vbuff)
	{
		releaseAll();
		vb = vbuff.vb;
		vbuff.vb = nullptr;
		eleSize = vbuff.eleSize;
		elementNum = vbuff.elementNum;
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
