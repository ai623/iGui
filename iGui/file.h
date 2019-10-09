#pragma once
#include <d3d11.h>
#include <string>

namespace iCommon {
	namespace FileType {
		enum FileType{
			D3dShader
		};
	};

	struct File {
		void load(std::string path);
		const std::string& getPath() const { return path; }
	private:
		struct FileInterface {
			virtual ~FileInterface() { }
		};

		struct FileD3dShader {
			virtual ~FileD3dShader() { releaseAll(); }
			inline void releaseAll() {
#define R(x) if(x) {x->Release(); x = nullptr;}
				R(data)
#undef R
			}
			ID3DBlob* data = nullptr;
		};

		std::string path;
		FileType::FileType type;
		FileInterface* methods = nullptr;
	};
}