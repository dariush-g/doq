#include "index.hpp"
#include <fstream>
#include <iostream>
#include <msgpack.hpp>
#include <sstream>

std::vector<Document> load_index(const std::string &path) {
	std::ifstream file(path, std::ios::binary);
	std::stringstream buf;
	buf << file.rdbuf();
	std::string data = buf.str();

	msgpack::object_handle oh = msgpack::unpack(data.data(), data.size());
	msgpack::object obj = oh.get();

	std::vector<Document> docs;
	for (size_t i = 0; i < obj.via.array.size; i++) {
		auto &entry = obj.via.array.ptr[i];
		Document doc;
		doc.path = entry.via.map.ptr[0].val.as<std::string>();
		doc.name = entry.via.map.ptr[1].val.as<std::string>();
		doc.extension = entry.via.map.ptr[2].val.as<std::string>();
		doc.size = entry.via.map.ptr[3].val.as<uint64_t>();
		doc.type = entry.via.map.ptr[4].val.as<std::string>();

		auto &page_array = entry.via.map.ptr[5].val.via.array;
		for (size_t j = 0; j < page_array.size; j++) {
			auto &p = page_array.ptr[j];
			Page page;
			page.number = p.via.map.ptr[0].val.as<int>();
			page.text = p.via.map.ptr[1].val.as<std::string>();
			doc.pages.push_back(page);
		}

		docs.push_back(doc);
	}

	return docs;
}