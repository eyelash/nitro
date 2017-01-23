/*

Copyright (c) 2016-2017, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include <cstdlib>
#include <new>

template <class T> class Reference {
	T* object;
public:
	Reference(T* object = nullptr): object(object) {
		if (object) {
			++object->reference_count;
		}
	}
	Reference(const Reference& reference): object(reference.object) {
		if (object) {
			++object->reference_count;
		}
	}
	~Reference() {
		if (object && --object->reference_count == 0) {
			delete object;
		}
	}
	Reference& operator =(const Reference& reference) {
		if (reference.object == object) return *this;
		if (object && --object->reference_count == 0) {
			delete object;
		}
		object = reference.object;
		if (object) {
			++object->reference_count;
		}
		return *this;
	}
	T* operator ->() const {
		return object;
	}
	operator T*() const {
		return object;
	}
};

template <class T> class Buffer {
	T* data;
	int length;
public:
	Buffer(int length): length(length) {
		data = (T*)malloc(length*sizeof(T));
		for (int i = 0; i < length; ++i) {
			new (data+i) T();
		}
	}
	Buffer(const Buffer& buffer): length(buffer.length) {
		data = (T*)malloc(length*sizeof(T));
		for (int i = 0; i < length; ++i) {
			new (data+i) T(buffer.data[i]);
		}
	}
	~Buffer() {
		for (int i = 0; i < length; ++i) {
			(data+i)->~T();
		}
		free(data);
	}
	Buffer& operator =(const Buffer& buffer) {
		for (int i = 0; i < length; ++i) {
			(data+i)->~T();
		}
		if (buffer.length != length) {
			free(data);
			length = buffer.length;
			data = (T*)malloc(length*sizeof(T));
		}
		for (int i = 0; i < length; ++i) {
			new (data+i) T(buffer.data[i]);
		}
		return *this;
	}
	operator T*() const {
		return data;
	}
	T& operator [](int i) const {
		return data[i];
	}
	int get_length() const {
		return length;
	}
};
