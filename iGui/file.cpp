#include "file.h"

namespace iCommon {
	FileInitializer fileInit;
	namespace _internal {
		FT_Library ftLibrary;
		iCommon::Debug debug;
	}

	void FileInitializer::operator()()
	{
		auto error = FT_Init_FreeType(&_internal::ftLibrary);
		if(error){
			_internal::debug.error("Fail to Initialize file.h", error);
		}
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
		auto error = FT_New_Face(ftLibrary, path.c_str(),-1,&face );
		if (error) {
			if (error == FT_Err_Unknown_File_Format) {
				_internal::debug("iCommon::FontFile: unsupported format.");
			}
			else {
				_internal::debug("iCommon::FontFile: cannot open the file.");
			}
			return false;
		}

		mfacesNum = face->num_faces;
		mfaces = new FT_Face[mfacesNum];
		//Check end

		//Get font faces
		for (int i = 0; i < mfacesNum; i++) {
			error = FT_New_Face(ftLibrary, path.c_str(), i, mfaces+i);
			if (error) {
				if (error == FT_Err_Unknown_File_Format) {
					_internal::debug("iCommon::FontFile: unsupported format.");
				}
				else {
					_internal::debug("iCommon::FontFile: cannot open the file.");	
				}

				{
					//destroy
					mfacesNum = i;
					close();
				}
				return false;
			}
		}
		//Get end

		mpath = path;

		//init encoding type
		if(mencodingType!= FT_ENCODING_NONE)
			for (int i = 0; i < mfacesNum; i++){
				auto error = FT_Select_Charmap(mfaces[i], mencodingType);
				if (error) {
					mencodingType = FT_ENCODING_NONE;
					_internal::debug("iCommon::FontFile: Invalid encoding type");
				}
		}
		//init end

		return true;
	}
	void FontFile::close()
	{
		for (int i = 0; i < mfacesNum; i++) {
			FT_Done_Face(mfaces[i]);
		}
		delete[] mfaces;
		mfaces = nullptr;
		mfacesNum = 0;
	}

	bool FontFile::setEncodingType(EncodingType encodingType)
	{
		if (mencodingType != encodingType) {
			mencodingType = encodingType;
			for (int i = 0; i < mfacesNum; i++) {
				auto error = FT_Select_Charmap(mfaces[i], mencodingType);
				if (error) {
					mencodingType = FT_ENCODING_NONE;
					return false;
				}
			}
		}
		return true;
	}
}

