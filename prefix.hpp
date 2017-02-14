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
#include <algorithm>
#include <queue>

#define MAX_CANDIDATES 5

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
	std::unordered_set<InfoNode<CHAR>*> children;
	const PrefixNode<CHAR> *str;
	char depth;
	InfoNode (InfoNode *f = NULL, char d = 0) {
		father = f;
		depth = d;
	}
	~InfoNode () {
		for (auto &n : children) {
			if (n) delete n;
		}
		children.clear();
	}
	std::basic_string<CHAR> GetText() const {
		std::basic_string<CHAR> s;
		const PrefixNode<CHAR> *node = str;
		while (node->val != -1) {
			s += node->val;
			node = node->father;
		}
		std::reverse(s.begin(), s.end());
		return s;
	}
	bool IsFatherOf(const InfoNode<CHAR> *other) const {
		if (other) {
			while (other->depth > depth) {
				if (other->father == this) return true;
				other = other->father;
			}
		}
		return false;
	}
	std::basic_string<CHAR> print(const std::basic_string<CHAR> &gap) const {
		std::basic_string<CHAR> result;
		const InfoNode<CHAR> *node = this;
		result = node->GetText();
		while (node->depth) {
			node = node->father;
			result = node->GetText() + gap + result;
		}
		return result;
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
		char last_level = -1;
		std::vector<InfoNode<CHAR> *> prev_level_stack(5);
		InfoNode<CHAR> *node = &info_table;
		info_table.father = NULL;
		prefix_table.father = NULL;
		prefix_table.val = -1;
		while (getline(fin, s)) {
			if (s.empty()) continue;
			int current_level = s[0] - L'a';
			if (current_level < 0 || current_level >= MAX_LEVEL)
				current_level = last_level;
			else s = s.substr(1, s.length() - 1);
			while (last_level >= current_level) {
				--last_level;
				node = node->father;
			}
			InfoNode<CHAR> *tmp = new InfoNode<CHAR>(node, current_level);
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
	                  int *match_len,
	                  int min_len = 2) {
		const size_t s_len = s.length();
		if (s_len < min_len) return false;
		size_t i = 0;
		PrefixNode<CHAR> *node = &prefix_table;
		for (; i < s_len; ++i) {
			std::unordered_map<CHAR, PrefixNode<CHAR>*> &childs = node->children;
			if (childs.find(s[i]) != childs.end()) {
				node = childs[s[i]];
			} else break;
		}
		if (i < min_len) return false;
		*match_len = i;
		node->CollectRecursive(result);
		return true;
	}
	typedef std::pair<std::basic_string<CHAR>, bool> partstype;
	typedef std::pair<const InfoNode<CHAR> *, int> pathpair;
	typedef std::vector<pathpair> pathvec;
	class pathtype {
	public:
		pathvec pv;
		float score;
		pathtype() {}
		pathtype(const pathvec &pv_, const int score_) {
			pv = pv_;
			score = score_;
		}
		friend bool operator > (const pathtype &n1, const pathtype & n2) {
			if (n1.pv.size() != n2.pv.size()) return n1.pv.size() > n2.pv.size();
			return n1.score > n2.score;
		}
	};
	void Search(const std::basic_string<CHAR> &s,
	            std::vector<std::basic_string<CHAR>> &result) {
		clock_t start = clock();
		size_t idx = 0;
		size_t s_len = s.length();
		std::vector<partstype> s_parts;
		std::vector<pathtype> paths;
		size_t last_idx = 0;
		float min_score = -1;
		while (idx < s_len) {
			std::vector<const InfoNode<CHAR> *> result_node;
			int match_len;
			std::basic_string<CHAR> subs = s.substr(idx, s_len);
			if (PrefixSearch(subs, result_node, &match_len)) {
				if (last_idx != idx) {
					// push unfound string
					s_parts.push_back(partstype(s.substr(last_idx, idx - last_idx), false));
				}
				s_parts.push_back(partstype(s.substr(idx, match_len), true));
				for (const auto &r : result_node) {
					float score = CalScore(r->depth);
					// save best N candidates
					// only find the first father
					bool if_found = false;
					pathpair cpath(r, s_parts.size() - 1);
					for (size_t pidx = 0; pidx < paths.size(); ++pidx) {
						auto &path = paths[pidx].pv;
						for (int i = path.size() - 1; i >= 0 ; --i) {
							if (path[i].first->IsFatherOf(r)) {
								if_found = true;
								if (i == path.size() - 1) {
									path.push_back(cpath);
									paths[pidx].score += score;
									ReSort(paths, pidx);
								} else {
									// new path
									++i;
									pathvec *tmp = NULL;
									for (size_t j = 0; j < i; ++j)
										score += CalScore(path[j].first->depth);
									if (PushPath(paths, &tmp, score)) {
										tmp->insert(tmp->begin(), paths[pidx].pv.begin(), paths[pidx].pv.begin() + i);
										tmp->push_back(cpath);
									}
								}
								break;
							}
						}
						if (if_found) break;
					}
					if (!if_found) {
						pathvec *tmp = NULL;
						if (PushPath(paths, &tmp, score)) tmp->push_back(cpath);
					}
				}
				idx += match_len;
				last_idx = idx;
			} else ++idx;
		}
		printf("time: %f ms\n", double(clock() - start) / CLOCKS_PER_SEC * 1000);
		for (auto &p : paths) {
			printf("%.2f %s\n", p.score, wstring_to_utf81(p.pv.back().first->print(L"->")).c_str());
		}
	}
private:
	int CalScore(int depth) {
		return 1 << (MAX_LEVEL - depth - 1);
	}
	void ReSort(std::vector<pathtype> &paths, int idx) {
		const float c_score = paths[idx].score;
		int i = idx - 1;
		for (; i >= 0; --i) {
			if (paths[i].score >= c_score)
				break;
		}
		++i;
		if (i == idx) return;// position not changed
		pathtype bk = paths[idx];
		while (idx > i) {
			paths[idx] = paths[idx - 1];
			--idx;
		}
		paths[i] = bk;
	}
	bool PushPath(std::vector<pathtype> &paths, pathvec **pv, float score) {
		int idx = paths.size();
		if (idx == MAX_CANDIDATES && paths.back().score >= score) return false;
		int i = idx - 1;
		for (; i >= 0; --i) {
			if (paths[i].score >= score)
				break;
		}
		++i;
		if (i == idx) {
			if (i < MAX_CANDIDATES) {
				paths.push_back(pathtype());
				*pv = &(paths.back().pv);
				paths.back().score = score;
				return true;
			} else return false;
		}
		if (idx < MAX_CANDIDATES) {
			paths.push_back(pathtype());
			paths.back() = paths[paths.size() - 2];
		}
		--idx;
		while (idx > i) {
			paths[idx] = paths[idx - 1];
			--idx;
		}
		paths[i].score = score;
		*pv = &(paths[i].pv);
		(*pv)->clear();
		return true;
	}
};