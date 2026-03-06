#pragma once

#include "pch.h"

// ReSharper disable CppClangTidyClangDiagnosticPadded
template <typename T>
class ISingleton
{
public:

	friend T;

	ISingleton(const ISingleton&) = delete;
	ISingleton& operator=(const ISingleton&) = delete;
	ISingleton(ISingleton&&) = delete;
	ISingleton& operator=(ISingleton&&) = delete;

	virtual ~ISingleton() = default;

	static T& GetInstance()
	{
		static T sInstance;
		return sInstance;
	}

private:

	explicit ISingleton() = default;
};