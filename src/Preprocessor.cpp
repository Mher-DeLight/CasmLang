#include "../include/Common.h"
#include "../include/ErrorHandler.h"
#include "../include/Tokenizer.h"
#include <filesystem>
#include <fstream>
#define ucharcast(x) static_cast<unsigned char>(x)

namespace ocarlang {
namespace fs = std::filesystem;
void Tokenizer::prcs_process() {
    if (current() != '#')
        panic("Cannot preprocess line that does not start with '#'");
    advance();

    std::string command = "";
    while (!eof() && current() != ' ' && current() != '\n') {
        command += current();
        advance();
    }
    advance(); // skip space or newline

    if (command == "stdlib") {
        prcs_process_include();
    } else if (command == "noheader") {
        prcs_process_noheader();
    }
}

void Tokenizer::prcs_process_include() {
    fs::path stdlib = OCAR_STDLIB_PATH;

    std::string arg;
    while (!eof() && current() != '\n') {
        arg += current();
        advance();
    }

    // remove whitespaces around it
    while (!arg.empty() && std::isspace(ucharcast(arg.front())))
        arg.erase(arg.begin());

    while (!arg.empty() && std::isspace(ucharcast(arg.back())))
        arg.pop_back();

    fs::path linkfile = stdlib / (arg + ".ocar");

    if (!fs::exists(linkfile)) {
        panic("Preprocessor: couldn't find file \"" + arg + ".ocar\" in standard library at line " +
              std::to_string(row - borrowedlines));
    }

    std::ifstream file(linkfile);
    if (!file) {
        panic("Preprocessor: Found file \"" + arg +
              ".ocar\" in standard library but couldn't open it; check permissions");
    }

    // read the included file.
    std::string included((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // Make sure the included file cannot accidentally merge its last
    // token with the token following the #stdlib directive.
    if (!included.empty() && included.back() != '\n')
        included += '\n';

    /*
     * cursor currently points at the newline after:
     *
     *     #stdlib filename
     *
     * insert the file immediately before that newline.
     *
     * we intentionally do NOT advance() here. the next iteration of
     * tokenize() will process the first character of the included file.
     */
    for (char ch: included){
        if (ch=='\n'){
            borrowedlines++;
        }
    }

    code.insert(cursor, included);
    /* we don't have to remove the #include line because we're not gonna see it again anyway
     * if preprocessor jumps are ever implemented, which i hope they aren't, we might have to remove
     * the line to prevent infinite copying
     * but otherwise, it's okay
     */
}
void Tokenizer::prcs_process_noheader() {
    makeHeader = false;
}

} // namespace ocarlang