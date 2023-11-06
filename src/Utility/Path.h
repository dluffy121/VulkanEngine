#pragma once

#include <Common.h>

template<class T>
inline T RemoveExtension(const T& filename)
{
	typename T::size_type const p{ filename.find_last_of('.') };

	return
		p > 0 && p != T::npos
		?
		filename.substr(0, p)
		:
		filename;
}

inline std::string RemoveExtension(const std::string& filename)
{
	std::string::size_type const p{ filename.find_last_of('.') };

	return
		p > 0 && p != std::string::npos
		?
		filename.substr(0, p)
		:
		filename;
}


template<class T>
inline T FileNameWithExt(const T& path, const T& delims = "/\\")
{
	return path.substr(path.find_last_of(delims) + 1);
}

inline std::string FileNameWithExt(const std::string& path, const std::string& delims = "/\\")
{
	return path.substr(path.find_last_of(delims) + 1);
}


template<class T>
inline T FileName(const T& path, const T& delims = "/\\")
{
	return RemoveExtension(FileNameWithExt(path, delims));
}

inline std::string FileName(const std::string& path, const std::string& delims = "/\\")
{
	return RemoveExtension(FileNameWithExt(path, delims));
}