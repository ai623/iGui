#include "gui.h"
#include "file.h"

iCommon::Debug debug;
using namespace iGui;

struct MyWindow :Window {
	~MyWindow() {
	}

	MyWindow(){
		auto& painter = getPainter();

		vertexShader.init(painter, L"../x64/Debug/VertexShader.cso");
		auto re = vertexShader.hasLoaded();
		if (re == false)debug.error("");

		pixelShader.init(painter, L"../x64/Debug/PixelShader.cso");
		re = pixelShader.hasLoaded();
		if (re == false)debug.error("");

		InputElementDesc ieDesc[]{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 };

		layout.init(painter, vertexShader, ieDesc, 1);
		dsBuff.init(*this);

		BufferDesc desc{};
		desc.usage = D3D11_USAGE_IMMUTABLE;

		struct Vertex {
			float x;
			float y;
			float z;
		};

		Vertex vertices[]{
			{ 0.0f, 0.5f, 0.5f },
			{0.5f, -0.5f, 0.5f },
			{-0.5f, -0.5f, 0.5f}
		};
		vertexBuffer.init(painter, vertices, 3, desc);

		uint16_t indices[]{
			0,1,2
		};
		indexBuffer.init(painter, indices, 3, desc);
	


		auto rect = getWindowRect();

		Viewport vp{
			0,0,(FLOAT)rect.width,(FLOAT)rect.height,0,1
		};


		painter.set(vp);
		painter.setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		painter.setTarget(*this, dsBuff);
		painter.set(layout);
		painter.set(vertexShader);
		painter.set(indexBuffer);
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
		//painter.drawIndex();
		painter.drawVertex();
		present();
	}

private:
	VertexShader vertexShader = VertexShader(Nothing());
	PixelShader pixelShader = PixelShader(Nothing());
	InputLayout layout = InputLayout(Nothing());
	DepthStencilBuffer dsBuff = DepthStencilBuffer(Nothing());
	VertexBuffer vertexBuffer = VertexBuffer(Nothing());
	IndexBuffer indexBuffer = IndexBuffer(Nothing());
};

int guiMain() {

	iGuiInit.enableDebug();
	//iGuiInit.disable4xMsaa();
	iGuiInit();

	MyWindow wnd;

	{
		using namespace iCommon;
		file::fileInit();
		
		using file::FontFile;
		using file::FontFace;

		FontFace face;
		FontFile file;
		auto re = file.open(R"(D:\temp\Fonts\msyh.ttc)");
		re = file.readFontFace(face, 0);
		auto& size = face.getGlyghSize();
		re = face.setSizeByPixel(10,0);
		re = face.setEncodingType(FT_ENCODING_UNICODE);
		auto index = face.getGlyghIndex(L'a');
		re = face.loadGlygh(index);
		auto& outline = face.getOutline();

		file::fileInit();
	}

	

	return exec();
}