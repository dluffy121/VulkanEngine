#pragma once

#include <memory>
#include <iostream>
#include <string>
#include <assert.h>
#include <Type.h>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <format>

#define UINT8 uint8_t
#define UINT16 uint16_t
#define UINT32 uint32_t
#define UINT64 uint64_t

#define INT8 int8_t
#define INT16 int16_t
#define INT32 int32_t
#define INT64 int64_t

//#define VK_USE_PLATFORM_WIN32_KHR

// Graphics Library Layer (GLL)
#ifndef _glfw3_h_

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32

#ifdef VK_USE_PLATFORM_WIN32_KHR
#include <GLFW/glfw3native.h>
#endif // VK_USE_PLATFORM_WIN32_KHR

#endif // _glfw3_h_



// General Macros
#define EXPAND(x) x

#define GET_MACRO(_1, _2, NAME,...) NAME

#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

#define TO_STRING(x) std::to_string(x)

#define TYPE(typename) typename



// Smart Pointers
#define UPTR std::unique_ptr
#define SPTR std::shared_ptr
#define WPTR std::weak_ptr

#define MAKE_UPTR std::make_unique
#define MAKE_SPTR std::make_shared
#define MAKE_WPTR std::make_weak



#pragma region Main Debugging

#ifndef NDEBUG


#define ASSERT(...) EXPAND(GET_MACRO(__VA_ARGS__, ASSERT2, ASSERT1)(__VA_ARGS__))

#define ASSERT1(x)					\
					if(!(x)) __debugbreak();

#define ASSERT2(x, m)				\
					if(!__Assert(x, #x, __FILE__, __LINE__, m)) __debugbreak();

#define GLLog(x)					\
					GLClearError();	\
					x;				\
					assert(GLLogCall(#x, __FILE__, __LINE__));

#define Log(x)						\
					std::cout << x << std::endl;

#define LogError(x)					\
					std::cerr << x << std::endl;

inline bool __Assert(bool expr, const char* expr_str, const char* file, int line, const std::string& msg)
{
	if (!expr)
	{
		LogError(
			"Assert failed:\n" << msg << "\n"
			<< "Expected:\t" << expr_str << "\n"
			<< "Source:\t\t" << file << ", line " << line);
		return false;
	}
	return true;
}

#define __METHOD_NAME__ methodName(__PRETTY_FUNCTION__)

inline std::string methodName(const std::string& prettyFunction)
{
	size_t colons = prettyFunction.find("::");
	size_t begin = prettyFunction.substr(0, colons).rfind(" ") + 1;
	size_t end = prettyFunction.rfind("(") - begin;

	return prettyFunction.substr(begin, end) + "()";
}


#define __CLASS_NAME__ className(__PRETTY_FUNCTION__)

inline std::string className(const std::string& prettyFunction)
{
	size_t colons = prettyFunction.find("::");
	if (colons == std::string::npos)
		return "::";
	size_t begin = prettyFunction.substr(0, colons).rfind(" ") + 1;
	size_t end = colons - begin;

	return prettyFunction.substr(begin, end);
}


#else // RELEASE


#define ASSERT(...)

#define ASSERT1(x)

#define ASSERT2(x, m)

#define Log(x)

#define LogError(x)

#define __METHOD_NAME__

#define __CLASS_NAME__


#endif // NDEBUG

#pragma endregion

#pragma region Vulkan Debugging

#ifndef NDEBUG

// Enable Vulkan Validation Layers
#define ENABLE_VK_VAL_LAYERS

#endif // NDEBUG


#pragma endregion
