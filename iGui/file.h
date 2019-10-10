#pragma once
#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "debug.h"

namespace iCommon {
	struct FileInitializer {
		void operator () ();
	};
	extern FileInitializer fileInit;

	struct File {
		virtual ~File() {};
		virtual const std::string& getPath()const = 0;
		virtual bool open(const std::string& path) = 0;
		virtual void close() = 0;
		virtual bool isOpen() const = 0;
	};

	struct ByteFile {

	};

	struct TextFile {

	};

	struct ImageFile {

	};

	struct FontFile :File{
		using EncodingType = decltype(FT_ENCODING_UNICODE);
		~FontFile();

		virtual const std::string& getPath()const { return mpath; }
		virtual bool open(const std::string& path);
		virtual void close();
		virtual bool isOpen() const { return mfacesNum; }
		int getFontNum() const { return mfacesNum; }
		bool setEncodingType(EncodingType et);

	private:
		std::string mpath;
		FT_Face* mfaces = nullptr;
		int mfacesNum = 0;
		EncodingType mencodingType = FT_ENCODING_NONE;
	};
}