#include <iostream>
#include <array>
#include <stack>
#include <vector>
#include <algorithm>

using std::cout;
using std::endl;

struct Node {
    double value;
//    unsigned char c;
    Node *rhs;
    Node *lhs;

    ~Node() {
        cout << "the object is getting destroyed" << endl;
    }

    static bool intCmp(const Node *lhs, const Node *rhs) {
        return lhs->value > rhs->value;
    }
};

void recursive(Node &node, std::vector<int> &codes, unsigned int sol = 0, int n = -1) {
    n++;
    if (node.rhs == nullptr && node.lhs == nullptr) {
        cout << "Found a solution" << endl;
        for (int i = n - 1; i >= 0; --i) {
            cout << ((sol >> i) & 1);
        }
        cout << endl;
        return;
    }
    if (node.rhs != nullptr) {
//        sol = (sol << n) & (static_cast<int>(0) | 1);
        recursive(*node.rhs, codes, ((sol << 1) | 1), n);
    }
    if (node.lhs != nullptr) {
        recursive(*node.lhs, codes, (sol << 1) | 0, n);
    }
}

int main() {
    std::array<double, 13> arr = {0.2, 0.18, 0.1, 0.1, 0.1, 0.06, 0.06, 0.04, 0.04, 0.04, 0.04, 0.03, 0.01};
//    double arr2[10] =  {5.0, 7.0, 9.0, 1.0, 4.5, 2.0, 6.0, 5.5, 8.9, 10.0};
    std::vector<Node *> s;
    std::vector<int> codes;
    for (const auto &x: arr) {
        Node *n = new Node(x, nullptr, nullptr);
        s.push_back(n);
    }
    while (s.size() > 1) {
        Node *e1 = s.back();
        s.pop_back();
        Node *e2 = s.back();
        s.pop_back();
        Node *brench = new Node(e1->value + e2->value, e1, e2);
        s.push_back(brench);
        for (int i = s.size() - 1; i > 0; --i) {
            if (s.at(i)->value > s.at(i - 1)->value) {
                std::swap(s.at(i), s.at(i - 1));
            }
        }
    }
    recursive(*s[0], codes);


    return 0;
}
