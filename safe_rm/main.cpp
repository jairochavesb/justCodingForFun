#include <format>
#include <iostream>
#include <string>
#include <filesystem>
#include <unistd.h>
#include <vector>
#include <system_error>
#include <fstream>
#include <cstring>

#include <QFile>
#include <QCryptographicHash>
#include <QByteArray>
#include <QDateTime>

namespace fs = std::filesystem;

using std::cout, std::cerr, std::cin, std::endl;
using std::string, std::string_view, std::format;
using std::vector, std::error_code;
using std::optional, std::nullopt;
using std::ofstream, std::transform, std::tolower;

void create_trash_dir (string_view trash_dir) {
    error_code ec {};

    fs::create_directory(trash_dir, ec);
    if (ec) {
        cerr << ec.message() << endl;
        exit(1);
    }

    fs::perms perms {fs::perms::owner_all|fs::perms::group_all|fs::perms::others_exec};

    fs::permissions(trash_dir, perms);

    fs::current_path(trash_dir, ec);
    if (ec) {
        cerr << ec.message() << endl;
        exit(1);
    }

    vector<string> sub_dirs {"files", "info"};

    for (auto &dir : sub_dirs ) {
        ec.clear();

        fs::create_directory(dir, ec);
        if (ec) {
            cerr << ec.message() << endl;
            exit(1);
        }

        fs::permissions(dir, perms);
    }
}

string get_md5 (string file_name) {
    QString f_name = QString::fromStdString(file_name);

    QFile file(f_name);
    if (!file.open(QIODevice::ReadOnly)) {
        cerr << "can't read file '" << file_name <<"'" << endl;
        exit(1);
    }

    QCryptographicHash md5sum(QCryptographicHash::Md5);

    QByteArray buffer {};

    while (!file.atEnd()) {
        buffer = file.read(1024);
        md5sum.addData(buffer);
    }

    return md5sum.result().toHex().toStdString();
}

void trash_file(string file_name, string trash_dir) {
    cout << "inside trash_file()" << endl;

    string deletion_time {QDateTime::currentDateTime().toString().toStdString()};
    qint64 deletion_timestamp {QDateTime::currentSecsSinceEpoch()};

    error_code ec {};
    fs::current_path(trash_dir, ec);
    if (ec) {
        cerr << ec.message() << endl;
        return;
    }

    string file_name_md5 {get_md5(file_name)};
    string trash_file_name {format("{}/files/{}_{}", trash_dir, file_name_md5, deletion_timestamp)};
    string trash_info_name {format("{}/info/{}_{}", trash_dir, file_name_md5, deletion_timestamp)};

    ofstream info_f(trash_info_name);
    if (!info_f) {
        cerr << "can't create file " << trash_info_name << endl;
        return;
    }

    info_f << "[TRASH INFO]" << endl;
    info_f << "ORIGIN: " << file_name << endl;
    info_f << "DELETION DATE: " << deletion_time << endl;
    info_f << "TRASH_NAME: " << trash_file_name << endl;
    info_f.close();

    fs::copy_file(file_name, trash_file_name, ec);
    if (ec) {
        cerr << "can't copy file '" << file_name <<"'" << ec.message() << endl;

        ec.clear();

        fs::remove(trash_info_name, ec);
        if (ec) {
            cerr << ec.message() << endl;
        }

        return;
    }

    ec.clear();

    fs::remove_all(file_name, ec);
    if (ec) {
        cerr << ec.message();
    }
}

int main (int argc, char *argv[]) {

    char *home_dir_p {getenv("HOME")};
    if (!home_dir_p) {
        cerr << "can't determine user home dir" << endl;
        exit(1);
    }

    string home_dir {home_dir_p};
    string trash_dir = format("{}/{}", home_dir, ".saferm");

    if (!fs::exists(trash_dir)) {
        create_trash_dir(trash_dir);
    }

    optional<bool> trash_files {nullopt};
    optional<bool> purge_files {nullopt};
    optional<bool> ask_confirm {nullopt};

    int deletion_mode {0};

    int opt {0};
    while ((opt = getopt(argc, argv, "p:P:t:")) != -1) {
        switch (opt) {
        case 'p':
            purge_files = true;
            ask_confirm = true;
            break;

        case 'P':
            purge_files = true;

            break;

        case 't':
            trash_files = true;
            break;

        default:
            cout << "Unknown flag: " << optarg << endl;
            exit(1);
        }
    }

    error_code ec {};
    string abs_path {};
    vector<string> file_list {};

    for (int i {optind+1}; i <= argc; i++) {
        abs_path = fs::canonical(argv[i], ec);
        if (ec) {
            cerr << "can't get absolute path for: " << argv[i] << endl;
            continue;
        }

        file_list.push_back(abs_path);
    }

    if (purge_files.has_value()) {

        if(ask_confirm.has_value()) {
            cout << "This will delete the file(s) from disk. Continue? [Yes/no]: ";

            string answer {""};
            cin >> answer;

            for(int i {0}; i < answer.length(); i++) {
                answer[i] = tolower(answer[i]);
            }

            if(answer.compare("no") == 0) {
                exit(0);
            }
        }

        for (auto &file : file_list) {
            ec.clear();

            fs::remove_all(file, ec);
            if (ec) {
                cerr << ec.message() << endl;
            }
        }

    } else {
        for (auto &file : file_list) {
            trash_file(file, trash_dir);
        }
    }
}
