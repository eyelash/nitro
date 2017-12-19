/*

Copyright (c) 2017, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <fstream>

constexpr const char* LINE_START = "\t\"";
constexpr const char* LINE_END = "\\n\"\n";

class Name {
	const char* c;
public:
	Name(const char* path): c(path) {
		while (*path) {
			if (*path == '/') {
				c = path + 1;
			}
			++path;
		}
	}
	friend std::ostream& operator <<(std::ostream& os, const Name& name) {
		for (const char* c = name.c; *c; ++c) {
			if (*c == '.' || *c == '-') os << '_';
			else os.put(*c);
		}
		return os;
	}
};

int main(int argc, char** argv) {
	if (argc <= 2) {
		return 1;
	}
	std::ifstream input(argv[1]);
	std::ofstream output(argv[2]);
	output << "constexpr const char* " << Name(argv[1]) << " =\n";
	output << LINE_START;
	char c;
	while (input.get(c)) {
		if (c == '\n') {
			output << LINE_END;
			output << LINE_START;
		}
		else {
			if (c == '"' || c == '\\') {
				output << '\\';
			}
			output.put(c);
		}
	}
	output << LINE_END;
	output << ";\n";
}
