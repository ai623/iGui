#pragma once
#include <string>
#include <stdint.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_FONT_FORMATS_H
#include FT_OUTLINE_H

#include "debug.h"

namespace iCommon {
	namespace file {
		namespace _internal {
			extern FT_Library ftLibrary;
			extern iCommon::Debug debug;
		}

		struct FileInitializer {
			void operator () ();
			void unInit();
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

		using FontOutline = FT_Outline;

		struct FontFace {
			using EncodingType = decltype(FT_ENCODING_UNICODE);

			~FontFace() { if (mface) { FT_Done_Face(mface); } }

			unsigned int getFixedSizesNum()const { return mface->num_fixed_sizes; }
			const FT_Bitmap_Size* getFixedSizes() const { return mface->available_sizes; }

			const char* getFileFormat()const { return FT_Get_Font_Format(mface); }
			unsigned int getGlyghIndex(unsigned long code)const { return FT_Get_Char_Index(mface, code); }
			const FT_Size& getGlyghSize()const { return mface->size; }
			const FontOutline& getOutline()const { return mface->glyph->outline; }
			bool isFixedSize() const { return mface->num_fixed_sizes; }

			bool setEncodingType(EncodingType type);
			bool setSizeByDpi(unsigned int widthPer1_64, unsigned int heightPer1_64, unsigned int dotPerInchWidth, unsigned int dotPerInchHeight = 0) { return FT_Set_Char_Size(mface, widthPer1_64, heightPer1_64, dotPerInchWidth, dotPerInchHeight) == 0; }
			bool setSizeByPixel(unsigned int width, unsigned int height) { return FT_Set_Pixel_Sizes(mface, width, height) == 0; } //height or width == 0 means same as the other.

			bool loadGlygh(unsigned int index) { return FT_Load_Glyph(mface, index, FT_LOAD_DEFAULT) == 0; }

		private:
			FT_Face mface = nullptr;
			friend struct FontFile;
		};

		struct FontFile :File {
			using EncodingType = decltype(FT_ENCODING_UNICODE);
			~FontFile();

			virtual const std::string& getPath()const { return mpath; }
			virtual bool open(const std::string& path);
			virtual void close();
			virtual bool isOpen() const { return mfacesNum; }

			int getFontNum() const { return mfacesNum; }
			bool readFontFace(FontFace& face, int index);

		private:
			std::string mpath;
			int mfacesNum = 0;
		};
	}
}