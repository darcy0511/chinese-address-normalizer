#include <iostream>
#include <fstream>
#include <string>
#include <locale>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <boost/locale/encoding_utf.hpp>
using boost::locale::conv::utf_to_utf;
#include <cassert>
#include <vector>
// convert wstring to UTF-8 string
inline std::string wstring_to_utf81 (const std::wstring& str)
{
	return utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
}
template<class CHAR>
class InfoNode;

template<class CHAR>
class PrefixNode {
public:
	std::unordered_map<CHAR, PrefixNode<CHAR>*> children;
	PrefixNode<CHAR> *father;
	CHAR val;
	std::vector<const InfoNode<CHAR> *> info_link;
	bool is_exact;
	PrefixNode () {
		father = NULL;
		is_exact = false;
	}
	PrefixNode (CHAR c, PrefixNode *f = NULL, bool exact = false) {
		val = c;
		father = f;
		is_exact = exact;
	}
	~PrefixNode() {
		for (auto &n : children) {
			if (n.second) delete n.second;
		}
		children.clear();
	}
	PrefixNode<CHAR> * Add(const std::basic_string<CHAR> &s, InfoNode<CHAR> **info_node) {
		// PrefixNode<CHAR> *node = this;
		// if (s.empty()) {
		// 	if (children.find(' ') == children.end()) {
		// 		PrefixNode<CHAR> *tmp = new PrefixNode<CHAR>(' ', this, true);
		// 		children[' '] = tmp;
		// 		tmp->info_link.push_back(*info_node);
		// 		(*info_node)->str = tmp;
		// 		return tmp;
		// 	} else {
		// 		PrefixNode<CHAR> *tmp = children[' '];
		// 		(*info_node)->str = tmp;
		// 		tmp->info_link.push_back(*info_node);
		// 		return tmp;
		// 	}
		// } else {
		// 	size_t cursor = 0;
		// 	const auto s_end = s.end();
		// 	auto s_cursor = s.begin();
		// 	for (; s_cursor != s_end; ++s_cursor) {
		// 		std::unordered_map<CHAR, PrefixNode<CHAR>*> &childs = node->children;
		// 		if (childs.find(*s_cursor) != childs.end())
		// 			node = childs[*s_cursor];
		// 		else break;
		// 	}
		// 	for (; s_cursor != s_end; ++s_cursor) {
		// 		PrefixNode<CHAR> *tmp = new PrefixNode<CHAR>(*s_cursor, node);
		// 		node->children[*s_cursor] = tmp;
		// 		node = tmp;
		// 	}
		// 	node->is_exact = true;
		// 	(*info_node)->str = node;
		// 	node->info_link.push_back(*info_node);
		// }
		PrefixNode<CHAR> *node = this;
		if (s.empty()) {
			if (children.find(' ') == children.end()) {
				node = new PrefixNode<CHAR>(' ', this, true);
				children[' '] = node;
			} else node = children[' '];
		} else {
			size_t cursor = 0;
			const auto s_end = s.end();
			auto s_cursor = s.begin();
			for (; s_cursor != s_end; ++s_cursor) {
				std::unordered_map<CHAR, PrefixNode<CHAR>*> &childs = node->children;
				if (childs.find(*s_cursor) != childs.end())
					node = childs[*s_cursor];
				else break;
			}
			for (; s_cursor != s_end; ++s_cursor) {
				PrefixNode<CHAR> *tmp = new PrefixNode<CHAR>(*s_cursor, node);
				node->children[*s_cursor] = tmp;
				node = tmp;
			}
		}
		node->is_exact = true;
		(*info_node)->str = node;
		node->info_link.push_back(*info_node);
		return node;
	}
	void CollectRecursive(std::vector<const InfoNode<CHAR> *> &result) {
		if (is_exact) {
			result.insert(result.end(),
			              info_link.begin(),
			              info_link.end());
		}
		for (auto &child : children) {
			child.second->CollectRecursive(result);
		}
	}
};

template<class CHAR>
class InfoNode {
public:
	InfoNode *father;
	std::unordered_set<InfoNode*> children;
	const PrefixNode<CHAR> *str;
	InfoNode (InfoNode *f = NULL) {
		father = f;
	}
};

template<class CHAR>
class AddrParser {
public:
	PrefixNode<CHAR> prefix_table;
	InfoNode<CHAR> info_table;
	AddrParser() {
		std::locale loc( "zh_CN.UTF-8" );
		std::locale::global( loc );
	}
	AddrParser(const std::string &fname) {
		std::locale loc( "zh_CN.UTF-8" );
		std::locale::global( loc );
		std::basic_ifstream<CHAR> fin(fname);
		if (!fin.is_open()) {
			std::cout << fname << " doesn't exist";
			return;
		}
		std::basic_string<CHAR> s;
		int last_level = -1;
		std::vector<InfoNode<CHAR> *> prev_level_stack(5);
		InfoNode<CHAR> *node = &info_table;
		while (getline(fin, s)) {
			if (s.empty()) continue;
			int current_level = int(s[0] - L'a');
			if (current_level < 0 || current_level >= MAX_LEVEL)
				current_level = last_level;
			else s = s.substr(1, s.length() - 1);
			while (last_level >= current_level) {
				--last_level;
				node = node->father;
			}
			InfoNode<CHAR> *tmp = new InfoNode<CHAR>(node);
			node->children.insert(tmp);
			node = tmp;
			PrefixNode<CHAR> *p = prefix_table.Add(s, &node);
			prev_level_stack[current_level] = node;
			last_level = current_level;
		}
		fin.close();
	}
	bool PrefixSearch(const std::basic_string<CHAR> &s,
	                  std::vector<const InfoNode<CHAR> *> &result,
	                  int *match_len) {
		const size_t s_len = s.length();
		if (s_len < 2) return false;
		size_t i = 0;
		PrefixNode<CHAR> *node = &prefix_table;
		for (; i < s_len; ++i) {
			std::unordered_map<CHAR, PrefixNode<CHAR>*> &childs = node->children;
			if (childs.find(s[i]) != childs.end()) {
				node = childs[s[i]];
			} else break;
		}
		if (i <= 1) return false;
		*match_len = i;
		node->CollectRecursive(result);
		return true;
	}
};