#include "file.h"

namespace iCommon {
	namespace file {
		FileInitializer fileInit;
		namespace _internal {
			FT_Library ftLibrary;
			iCommon::Debug debug;
		}

		void FileInitializer::operator()()
		{
			auto error = FT_Init_FreeType(&_internal::ftLibrary);
			if (error) {
				_internal::debug.error("Fail to Initialize file.h", error);
			}
		}

		void FileInitializer::unInit()
		{
			FT_Done_FreeType(_internal::ftLibrary);
		}



		FontFile::~FontFile()
		{
			if (mfacesNum != 0) {
				close();
			}
		}

		bool FontFile::open(const std::string& path)
		{
			using _internal::ftLibrary;

			FT_Face face;

			//Check number of faces in the file
			auto error = FT_New_Face(ftLibrary, path.c_str(), -1, &face);
			if (error) {
				if (error == FT_Err_Unknown_File_Format) {
					_internal::debug("iCommon::FontFile: unsupported format.");
				}
				else {
					_internal::debug("iCommon::FontFile: cannot open the file.");
				}
				return false;
			}

			mpath = path;
			mfacesNum = face->num_faces;
			return true;
		}
		void FontFile::close()
		{
			mfacesNum = 0;
		}

		bool FontFile::readFontFace(FontFace& face, int index)
		{
			auto error = FT_New_Face(_internal::ftLibrary, mpath.c_str(), index, &face.mface);
			if (error) {
				_internal::debug("Fail to readFontFace");
				face.mface = nullptr;
				return false;
			}
			return true;;
		}


		bool FontFace::setEncodingType(EncodingType type)
		{
			auto error = FT_Select_Charmap(mface, type);
			if (error) {
				_internal::debug("iCommon::FontFace: Invalid encoding type");
				return false;
			}
			return true;
		}
	}
}

