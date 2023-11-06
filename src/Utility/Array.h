#pragma once

template <typename T, int n>
inline size_t ArraySize(const T(&)[n])
{
    return n;
}