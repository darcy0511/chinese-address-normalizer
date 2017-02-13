#include "preprocess.hpp"
#include "prefix.hpp"

void printNode(const PrefixNode<wchar_t> *node, const std::string &prefix) {
	if (node) {
		std::cout << prefix << wstring_to_utf81(std::wstring(1, node->val)) << std::endl;
		getchar();
		std::string str = prefix + "\t";
		for (auto c : node->children)
			printNode(c.second, str);
	}
}

int main() {
	// Preprocess p;
	// p.Read("log3_org");
	AddrParser<wchar_t> AP("log3_org");
	printf("load done!\n");
	// PrefixNode<wchar_t> *node = &AP.prefix_table;
	// printNode(node, std::string(""));
	while (true) {
		std::wstring s;
		std::vector<const InfoNode<wchar_t> *> result;
		int match_len;
		std::wcin >> s;
		if (AP.PrefixSearch(s, result, &match_len)) {
			printf("result count: %d\n", result.size());
		} else printf("match failed\n");
	}
	return 0;
}
