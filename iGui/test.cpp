#include "gui.h"

using namespace iGui;

struct MyWindow :Window {
	~MyWindow() {
	}

	MyWindow(){
		auto& painter = getPainter();

		vertexShader = VertexShader(painter, L"../x64/Debug/VertexShader.cso");
		auto re = vertexShader.hasLoaded();
		if (re == false)debug.error("");

		pixelShader = PixelShader(painter, L"../x64/Debug/PixelShader.cso");
		re = pixelShader.hasLoaded();
		if (re == false)debug.error("");

		InputElementDesc ieDesc[]{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 };

		layout = InputLayout(painter, vertexShader, ieDesc, 1);
		dsBuff = DepthStencilBuffer(*this);

		VertexBufferDesc desc{};
		desc.usage = D3D11_USAGE_IMMUTABLE;

		struct Vertex {
			float x;
			float y;
			float z;
		};

		Vertex arr[]{
			{ 0.0f, 0.5f, 0.5f },
			{0.5f, -0.5f, 0.5f },
			{-0.5f, -0.5f, 0.5f}
		};

		auto rect = getWindowRect();

		Viewport vp{
			0,0,rect.width,rect.height,0,1
		};

		vertexBuffer = VertexBuffer(painter, arr, 3, desc);

		painter.set(vp);
		painter.setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		painter.setTarget(*this, dsBuff);
		painter.set(layout);
		painter.set(vertexShader);
		painter.set(pixelShader);
		painter.set(vertexBuffer);
	}

	virtual void whenDestroy()
	{
		if (getWindowsNum() == 1) {
			exec.quit();
		}
	}

	virtual void whenPaint() {
		Painter& painter = getPainter();
		painter.clearTarget(*this);
		painter.clearTarget(dsBuff, Painter::CLEAR_DEPTH| Painter::CLEAR_STENCIL, 1, 0);
		painter.draw();
		present();
	}

private:
	VertexShader vertexShader = VertexShader(Nothing());
	PixelShader pixelShader = PixelShader(Nothing());
	InputLayout layout = InputLayout(Nothing());
	DepthStencilBuffer dsBuff = DepthStencilBuffer(Nothing());
	VertexBuffer vertexBuffer = VertexBuffer(Nothing());
};

int guiMain() {
	iGuiInit.enableDebug();
	iGuiInit.disable4xMsaa();
	iGuiInit();

	MyWindow wnd;

	return exec();
}