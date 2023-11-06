#pragma once

#include <string>

template <typename T>
inline type_info GetType()
{
	return typeid(T);
}

template <typename T>
inline std::string GetTypeName()
{
	return typeid(T).name();
}

template <typename T>
inline size_t GetTypeHash()
{
	return typeid(T).hash_code();
}