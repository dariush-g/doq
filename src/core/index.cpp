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

std::unordered_map<std::string, std::vector<std::tuple<int, int, int>>>
load_inv_index(const std::string &path) {
	std::ifstream file(path, std::ios::binary);
	std::stringstream buf;
	buf << file.rdbuf();
	std::string data = buf.str();

	msgpack::object_handle oh = msgpack::unpack(data.data(), data.size());
	msgpack::object obj = oh.get();

	std::unordered_map<std::string, std::vector<std::tuple<int, int, int>>> inv;

	auto &map = obj.via.map;
	for (size_t i = 0; i < map.size; i++) {
		std::string term = map.ptr[i].key.as<std::string>();
		auto &postings = map.ptr[i].val.via.array;

		std::vector<std::tuple<int, int, int>> entries;
		for (size_t j = 0; j < postings.size; j++) {
			auto &entry = postings.ptr[j].via.array;
			int di = entry.ptr[0].as<int>();
			int pi = entry.ptr[1].as<int>();
			int tf = entry.ptr[2].as<int>();
			entries.emplace_back(di, pi, tf);
		}
		inv[term] = std::move(entries);
	}

	return inv;
}

void load_bm25_meta(
	const std::string &path, std::unordered_map<std::string, int> &doc_freq,
	std::unordered_map<int, std::unordered_map<int, int>> &page_lengths,
	double &avg_page_len, int &total_pages) {
	std::ifstream file(path, std::ios::binary);
	std::stringstream buf;
	buf << file.rdbuf();
	std::string data = buf.str();

	msgpack::object_handle oh = msgpack::unpack(data.data(), data.size());
	msgpack::object obj = oh.get();

	auto &map = obj.via.map;
	for (size_t i = 0; i < map.size; i++) {
		std::string key = map.ptr[i].key.as<std::string>();

		if (key == "avg_page_len") {
			avg_page_len = map.ptr[i].val.as<double>();
		} else if (key == "total_pages") {
			total_pages = map.ptr[i].val.as<int>();
		} else if (key == "doc_freq") {
			auto &df_map = map.ptr[i].val.via.map;
			for (size_t j = 0; j < df_map.size; j++) {
				std::string term = df_map.ptr[j].key.as<std::string>();
				doc_freq[term] = df_map.ptr[j].val.as<int>();
			}
		} else if (key == "page_lengths") {
			auto &pl_map = map.ptr[i].val.via.map;
			for (size_t j = 0; j < pl_map.size; j++) {
				int di = pl_map.ptr[j].key.as<int>();
				auto &inner = pl_map.ptr[j].val.via.map;
				for (size_t k = 0; k < inner.size; k++) {
					int pi = inner.ptr[k].key.as<int>();
					page_lengths[di][pi] = inner.ptr[k].val.as<int>();
				}
			}
		}
	}
}