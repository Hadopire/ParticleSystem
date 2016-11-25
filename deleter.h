#pragma once
#include <CL/cl.h>

#include <functional>
#include <iostream>

template <typename T>
class Deleter
{
public:
	Deleter() : Deleter([](T) {}) {
	}

	Deleter(cl_int (_stdcall deletef)(T)) {
		this->deleter = [=](T obj) { deletef(obj); };
	}

	Deleter(std::function<void (T)> deletef) {
		this->deleter = [=](T obj) { deletef(obj);  };
	}

	~Deleter() {
		cleanup();
	}

	T* replace() {
		cleanup();
		return &object;
	}

	const T &data() const {
		return object;
	}

	const T* operator&() const {
		return &object;
	}

	operator const T() const {
		return object;
	}

	void operator=(T rhs) {
		if (object != rhs) {
			cleanup();
			object = rhs;
		}
	}

	template<typename V>
	bool operator==(V rhs) {
		return object == T(rhs);
	}

private:
	T object{ nullptr };
	std::function<void(T)> deleter;

	void cleanup() {
		if (object != nullptr) {
			deleter(object);
		}
		object = nullptr;
	}
};

