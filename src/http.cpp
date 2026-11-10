#include "http.h"

#include <array>
#include <cstdio>

#ifdef _WIN32
#define DEPVIZ_POPEN _popen
#define DEPVIZ_PCLOSE _pclose
static const char* const NULL_DEVICE = "NUL";
#else
#define DEPVIZ_POPEN popen
#define DEPVIZ_PCLOSE pclose
static const char* const NULL_DEVICE = "/dev/null";
#endif

namespace {

const int TIMEOUT_SECONDS = 30;
const std::size_t BUFFER_SIZE = 4096;
const int NO_RESPONSE = 0;

/// Команда curl: тихий режим, переходы по редиректам, код ответа в конце.
std::string curlCommand(const std::string& url) {
    return "curl -s -L --max-time " + std::to_string(TIMEOUT_SECONDS) + " -A \"depviz/1.0\" -w \"\\n%{http_code}\" \"" +
           url + "\" 2>" + NULL_DEVICE;
}

}  // namespace

HttpResponse parseCurlOutput(const std::string& output) {
    HttpResponse response;
    std::size_t lastLine = output.rfind('\n');
    std::string code = lastLine == std::string::npos ? output : output.substr(lastLine + 1);
    while (!code.empty() && (code.back() == '\r' || code.back() == ' ')) {
        code.pop_back();
    }
    try {
        response.status = std::stoi(code);
    } catch (const std::exception&) {
        response.status = NO_RESPONSE;
    }
    if (lastLine != std::string::npos) {
        response.body = output.substr(0, lastLine);
    }
    return response;
}

HttpResponse CurlFetcher::get(const std::string& url) {
    FILE* pipe = DEPVIZ_POPEN(curlCommand(url).c_str(), "r");
    if (pipe == nullptr) {
        throw RepositoryError("cannot run curl");
    }
    std::string output;
    std::array<char, BUFFER_SIZE> buffer{};
    std::size_t count;
    while ((count = std::fread(buffer.data(), 1, buffer.size(), pipe)) > 0) {
        output.append(buffer.data(), count);
    }
    DEPVIZ_PCLOSE(pipe);
    HttpResponse response = parseCurlOutput(output);
    if (response.status == NO_RESPONSE) {
        throw RepositoryError("cannot connect to " + url + " (check the network and that curl is installed)");
    }
    return response;
}
