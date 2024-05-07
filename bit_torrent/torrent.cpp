#include "cstdint"
#include <cstdlib>
#include <memory>
#include "vector"
#include "string"
#include "iostream"
#include "fstream"
#include <sstream>
#include <map>
#include <iomanip>

using namespace std;

struct Element {
    uint64_t value{};
    string str{};

    virtual void print(int tab) {}

    void printTab(int tab) {
        for (int i = 0; i < tab; ++i) {
            cout << "\t";

        }
    }

    virtual void printHash(int tab) {}
};

struct EInt : Element {

    explicit EInt(istream &is) {
        uint8_t tmp;
        string tmpStr{};
        while ((tmp = is.get()) != 'e') {
            tmpStr += tmp;
        }
        value = stoi(tmpStr);
    }

    void print(int tab = 0) override {
        cout << value;
    }
};

struct EString : Element {

    explicit EString(istream &is) {
        char tmp;
        string tmpStr;
        while ((tmp = is.get()) != ':') {
            tmpStr += tmp;
        }
        int len = stoi(tmpStr);
        for (int i = 0; i < len; ++i) {
            str += is.get();
        }

    }

    void print(int tab = 0) override {
        cout << "\"";
        for (auto &item: str) {
            if (item < 32 or item > 126) {
                cout << ".";
            } else {
                cout << item;
            }
        }

        cout << "\"";
//        cout << "\"" << str << "\"";
    }

    void printHash(int tab) override {
        int count = 20;
        for (auto &item: str) {
            if (count == 20) {
                cout << endl;
                printTab(tab + 1);
                count = 0;
            }
            cout << std::setfill('0') << std::setw(2) << std::hex << (int) ((uint8_t) item);
            count++;
        }
        cout << endl;
    }

};

struct EList : Element {
    vector<unique_ptr<Element>> list;

    void print(int tab = 0) override {
        cout << "[" << endl;
        for (auto &item: list) {
            printTab(tab + 1);
            item->print(tab + 1);
            cout << endl;
        }
        printTab(tab);
        cout << "]";
    }
};

struct EDict : Element {
    vector<pair<unique_ptr<Element>, unique_ptr<Element>>> dict{};

    void print(int tab = 0) override {
        cout << "{" << endl;
        for (auto &[e1, e2]: dict) {
            printTab(tab + 1);
            e1->print(tab + 1);
            cout << " => ";
            if (e1->str.contains("pieces")) {
                e2->printHash(tab + 1);
            } else {
                e2->print(tab + 1);
                cout << endl;
            }
        }
        printTab(tab);
        cout << "}";
    }
};

struct TorrentParser {
    static unique_ptr<Element> parse(istream &is) {

        char c = is.get();
        switch (c) {
            case 'd': {
                unique_ptr<EDict> dict = make_unique<EDict>();
                while ((c = is.peek()) != 'e') {
                    unique_ptr<Element> e1 = TorrentParser::parse(is);
                    unique_ptr<Element> e2 = TorrentParser::parse(is);
                    dict->dict.emplace_back(std::move(e1), std::move(e2));
                }
                is.get();
                return dict;
            }
            case 'l': {
                unique_ptr<EList> list = make_unique<EList>();
                while ((c = is.peek()) != 'e') {
                    list->list.push_back(TorrentParser::parse(is));
                }
                is.get();
                return list;
            }
            case 'i': {
                return make_unique<EInt>(is);
            }
            default: {
                if (stoi(to_string(c)) > 0) {
                    return make_unique<EString>(is.putback(c));
                }
            }
        }

        return make_unique<Element>();
    }
};


int main(int argc, char **argv) {
    if (argc != 2) {
        perror("No more then one parameter!\n");
        return EXIT_FAILURE;
    }
    if (!string{argv[1]}.contains(".torrent")) {
        perror("the parameter passed must be the path to a .torrent file\n");
        return EXIT_FAILURE;
    }

    ifstream is(argv[1], ios::binary);
    if (is.fail()) return EXIT_FAILURE;

    unique_ptr<Element> torrentFile = TorrentParser::parse(is);
    torrentFile->print(0);

    return EXIT_SUCCESS;
}