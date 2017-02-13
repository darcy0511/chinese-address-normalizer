#include <iostream>
#include <fstream>
#include <string>
#include <locale>
#include <unordered_map>
#include <map>
#include <boost/locale/encoding_utf.hpp>
using boost::locale::conv::utf_to_utf;
#include <cassert>
#define MAX_LEVEL 5

// convert UTF-8 string to wstring
inline std::wstring utf8_to_wstring (const std::string& str)
{
	return utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
}

// convert wstring to UTF-8 string
inline std::string wstring_to_utf8 (const std::wstring& str)
{
	return utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
}

struct Node {
	std::unordered_map<std::wstring, Node*> children;
	Node *father;
	Node (Node *f = NULL) {
		father = f;
	}
	~Node() {
		for (auto &n : children) {
			if (n.second) delete n.second;
		}
		children.clear();
	}
};

class Preprocess {
public:
	Preprocess() {
		std::locale loc( "zh_CN.UTF-8" );
		std::locale::global( loc );
	}
	Preprocess(const std::string &fname) {
		std::locale loc( "zh_CN.UTF-8" );
		std::locale::global( loc );
		std::wifstream fin(fname);
		if (!fin.is_open()) {
			std::cout << fname << " doesn't exist";
			return;
		}
		std::wstring s;
		Node root, *node = NULL;
		while (getline(fin, s)) {
			int i = 0;
			while (i < s.length() && s[i] == '\t')
				++i;
			if (i == 0)
				node = &root;
			else s = s.substr(i, s.length() - i);
			size_t npos = s.find(L'（');
			if (npos != std::wstring::npos) {
				s = s.substr(0, npos);
			}
			if (node->children.find(s) == node->children.end())
				node->children[s] = new Node;
			node = node->children[s];
		}
		fin.close();
		print(&root, 0);
	}
	bool print (const Node *node, int depth) {
		if (node == NULL || node->children.empty()) return false;
		bool flag = true;
		// std::string prefix;
		// for (int i = 0; i < depth; ++i) {
		// 	prefix += '\t';
		// }
		for (auto &n : node->children) {
			if (flag) printf("%c", depth + 'a');
			// printf("%s", prefix.c_str());
			printf("%s\n", wstring_to_utf8(n.first).c_str());
			flag = print(n.second, depth + 1);
		}
		return true;
	}
	void Read(const std::string &fname) {
		std::wifstream fin(fname);
		if (!fin.is_open()) {
			std::cout << fname << " doesn't exist";
			return;
		}
		std::wstring s;
		int last_level = -1;
		Node root, *node = &root;
		int count = 0;
		clock_t start = clock();
		while (getline(fin, s)) {
			int current_level = s[0] - L'a';
			if (current_level < 0 || current_level >= MAX_LEVEL)
				current_level = last_level;
			else s = s.substr(1, s.length() - 1);
			while (last_level >= current_level) {
				--last_level;
				node = node->father;
			}
			Node *tmp = new Node(node);
			node->children[s] = tmp;
			node = tmp;
			last_level = current_level;
		}
		fin.close();
		// printf("time: %f ms\n", double(clock() - start) / CLOCKS_PER_SEC * 1000);
		print(&root, 0);
	}
	Node addr;
	// PrefixNode prefix_table;
};
