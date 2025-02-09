#include <iostream>
#include <string>
#include <format>
#include <filesystem>
#include <system_error>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;

using std::cout, std::endl, std::cerr;
using std::string, std::string_view, fs::path, std::error_code;
using std::vector;

const string TRASH_FILES {"/.safe_rm/files/"};
const string TRASH_INFO {"/.safe_rm/info/"};

void list_trash(string_view trash_info) {
    for (auto f : fs::directory_iterator(trash_info)) {
        cout << f.path().filename() << endl;
    }

}

int main(int argc, char *argv[])
{
    char *home {getenv("HOME")};
    if (!home) {
        cerr << "can't get user home dir" << endl;
        return 1;
    }

    error_code ec {};

    string trash_path {format("{}{}", home, TRASH_FILES)};

    if (!fs::exists(trash_path)) {

            fs::create_directories(TRASH_FILES, ec);
            if (ec) {
                cerr << "can't create trash dir '" << TRASH_FILES << "': " << ec.message() << endl;
                return 1;
            }

            fs::create_directories(TRASH_INFO, ec);
            if (ec) {
                cerr << "can't create trash dir '" << TRASH_INFO << "': " << ec.message() << endl;
                return 1;
            }
        }

    int opt {0};
    while((opt = getopt(argc, argv, "elR:")) != -1) {
        switch (opt) {
        case 'e':

            cout << "emprtying trash folders..." << endl;
            list_trash(TRASH_INFO);

            break;

        case 'l':
            cout << "list trashed files" << endl;
            break;

        case 'R':
            for(int i {optind-1}; i < argc; i++) {
                cout << "recovering file " << argv[i] << endl;
            }

            break;

        default:
            break;
        }
    }
}
