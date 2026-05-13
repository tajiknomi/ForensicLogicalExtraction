// Copyright (c) Nouman Tajik [github.com/tajiknomi]
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE. 


#include "stringUtil.h"
#include <windows.h>
#include <iostream>


bool StringUtils::endsWith(const std::wstring_view& str, const std::wstring_view& suffix) {
	return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

bool StringUtils::startsWith(const std::wstring_view& str, const std::wstring_view& prefix) {
	return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

std::string StringUtils::convertWStringToUTF8(const std::wstring& wstr) {
	
	if (wstr.empty()) {
		return {};
	}
	int size_needed = WideCharToMultiByte(
		CP_UTF8, 0,
		wstr.data(), (int)wstr.size(),
		nullptr, 0,
		nullptr, nullptr
	);
	if (size_needed <= 0) {
		//throw std::runtime_error("MultiByteToWideChar failed");
		return {};
	}
	std::string result(size_needed, 0);
	WideCharToMultiByte(
		CP_UTF8, 0,
		wstr.data(), (int)wstr.size(),
		result.data(), size_needed,
		nullptr, nullptr
	);
	return result;
}

std::wstring StringUtils::convertUTF8ToWString(const std::string& str) {
	if (str.empty()) {
		return {};
	}

	int size_needed = MultiByteToWideChar(
		CP_UTF8, 0,
		str.data(), (int)str.size(),
		nullptr, 0
	);

	if (size_needed <= 0){
		//throw std::runtime_error("MultiByteToWideChar failed");
		return {};
	}
	std::wstring result(size_needed, 0);
	MultiByteToWideChar(
		CP_UTF8, 0,
		str.data(), (int)str.size(),
		result.data(), size_needed
	);
	return result;
}

std::vector<std::string> StringUtils::extract_items_from_str(const std::string& input_str, const std::string& delimiter) {

	std::string str(input_str);
	// Extract strings using a delimiter
	size_t pos = 0;
	std::string token;
	std::vector<std::string> items;

	while ((pos = str.find(delimiter)) != std::string::npos) {
		items.push_back(str.substr(0, pos));
		str.erase(0, pos + delimiter.length());
	}
	items.push_back(str.data());

	return items;
}