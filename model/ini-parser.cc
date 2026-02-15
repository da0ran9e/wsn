#include "ini-parser.h"

namespace ns3 {
namespace wsn {

IniParser::IniParser()
{
    // TODO: initialize anything if needed
}

IniParser::~IniParser()
{

}

void IniParser::read(const std::string &filename)
{
    // - call readFile(filename)
    //std::cout << "Reading INI file: " << filename << std::endl;
    //std::cout << "Absolute path: " << std::filesystem::absolute(filename) << std::endl;
    readFile(filename);
}

void IniParser::readFile(const std::string &filename)
{
    // - create include stack
    // - call doReadFile(filename, includeStack)
    //std::cout << "IniParser: readFile: " << filename << std::endl;
    std::vector<std::string> includeFileStack;
    doReadFile(filename, includeFileStack);
}

void IniParser::readText(const std::string &text,
                         const std::string &filename,
                         const std::string &baseDir)
{
    // TODO:
    // - convert text to std::stringstream
    // - call readStream()
}

void IniParser::readStream(std::istream &in,
                           const std::string &filename,
                           const std::string &baseDir)
{
    // TODO:
    // - prepare include stack
    // - call doReadFromStream() with stream
}

void IniParser::doReadFile(const std::string &filename,
                           std::vector<std::string> &includeStack)
{
    // - detect circular includes
    // - open file stream
    // - compute base directory from filename
    // - call doReadFromStream()

    // Convert to clean absolute path
    std::filesystem::path absPath = std::filesystem::absolute(filename);
    std::string absoluteFilename = absPath.lexically_normal().string();

    std::string baseDir = absPath.parent_path().string();

    for (const auto &f : includeStack) {
        if (f == absoluteFilename) {
            throw std::runtime_error(
                "IniParser: Circular include detected: file '" +
                filename + "' includes itself (directly or indirectly).");
        }
    }

    //std::cout << "IniParser: readFile: " << filename << std::endl;
    includeStack.push_back(absoluteFilename);

    std::ifstream in(filename);
    if (!in.is_open()) {
        throw std::runtime_error("IniParser: Cannot open ini file: " + filename);
    }

    doReadFromStream(in, filename, includeStack, absoluteFilename, baseDir);

    includeStack.pop_back();

}

inline bool opp_isblank(const char *s)
{
    for (; *s == ' ' || *s == '\t'; s++)
        /**/;
    return *s == '\0' || *s == '\n' || *s == '\r';
}

inline char firstNonwhitespaceChar(const char *s)
{
    for (; *s == ' ' || *s == '\t'; s++)
        /**/;
    return *s;
}

void IniParser::doReadFromStream(std::istream &in,
                                 const std::string &filename,
                                 std::vector<std::string> &includeStack,
                                 const std::string &absoluteFilename,
                                 const std::string &baseDir)
{
    // - parse stream line by line using forEachJoinedLine
    // - detect:
    //      '#' comment
    //      "include <file>"
    //      "[section]"
    //      "key = value"
    // - call callbacks or store results

    if (!m_listener) {
        throw std::runtime_error("***IniParser: No callbacks defined");
    }

    std::string currentSection;
    std::set<std::string> sectionsInFile;
    //std::cout << "IniParser: doReadFromStream: " << filename << std::endl;
    //std::cout << "Contents:" << in.rdbuf() << std::endl;
    forEachJoinedLine(in, [&](std::string &lineBuf, int lineNumber, int numLines) {
        std::string line = lineBuf;
        // std::cout << "IniParser: Processing line " << lineNumber << ": " << line << std::endl;
        // Trim
        rtrim(line);
        if (line.size() == 0)
            return; // skip blank line

        // Skip comment
        if (line[0] == '#')
            return;

        // Handle include
        if (line.rfind("include ", 0) == 0) {
            std::string includeFile = line.substr(strlen("include "));
            rtrim(includeFile);

            // loop
            if (std::find(includeStack.begin(), includeStack.end(), absoluteFilename) != includeStack.end())
                throw std::runtime_error("Circular include detected: " + filename);

            includeStack.push_back(absoluteFilename);

            // determine relative path
            std::string includePath = baseDir + "/" + includeFile;

            doReadFile(includePath, includeStack);

            includeStack.pop_back();
            return;
        }

        // [SectionName]
        if (line[0] == '[') {
            if (line.back() != ']') {
                throw std::runtime_error("Syntax error: missing closing ] in section header at "
                                         + filename + ":" + std::to_string(lineNumber));
            }

            std::string sectionName = line.substr(1, line.size() - 2);
            rtrim(sectionName);

            if (sectionName.empty())
                throw std::runtime_error("Empty section name at " + filename);

            //duplicate section
            if (sectionsInFile.count(sectionName))
                throw std::runtime_error("Duplicate section [" + sectionName + "] in file");

            sectionsInFile.insert(sectionName);
            currentSection = sectionName;

            // callback section
            // std::cout << "IniParser: Section: " << sectionName << std::endl;
            m_listener->onSection(sectionName);

            return;
        }

        // key=value
        auto eq = line.find('=');
        if (eq == std::string::npos)
            throw std::runtime_error("Expected key=value format at " + filename);

        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        // Trim key/value
        rtrim(key);
        rtrim(value);

        // detect optional '# comment' after value
        std::string comment;
        auto hashPos = value.find('#');
        if (hashPos != std::string::npos) {
            comment = value.substr(hashPos + 1);
            value = value.substr(0, hashPos);
            rtrim(value);
            rtrim(comment);
        }

        // callback key-value
        //std::cout << "IniParser: Key: " << key << " = " << value << std::endl;
        m_listener->onKeyValue(key, value, comment, baseDir);
        
    });
}

void IniParser::forEachJoinedLine(
    std::istream &in,
    const std::function<void(std::string &, int, int)> &processLine)
{
    // TODO:
    // - implement multi-line joining algorithm similar to OMNeT++:
    //     * join with backslash continuation
    //     * join with indent continuation
    // - for each joined line, call processLine(buffer, lineNumber, numPhysicalLines)

    // join continued lines, and call processLine() for each line
    std::string concatenatedLine = "";
    int startLineNumber = -1;
    int lineNumber = 0;
    while (true) {
        lineNumber++;
        std::string rawLine;
        if (!std::getline(in, rawLine))
            break;
        if (!rawLine.empty() && rawLine.back() == '\r')
            rawLine.resize(rawLine.size()-1);  // remove trailing CR (for CRLF files on Linux))
        if (!concatenatedLine.empty() && concatenatedLine.back() == '\\') {
            // backslash continuation: basically delete the backslash+LF sequence
            concatenatedLine.resize(concatenatedLine.size()-1);  // remove backslash
            concatenatedLine += rawLine;
        }
        else if (!opp_isblank(concatenatedLine.c_str()) && firstNonwhitespaceChar(concatenatedLine.c_str()) != '#' && // only start appending to non-blank and NON-COMMENT (!) lines
                 !opp_isblank(rawLine.c_str()) && (rawLine[0] == ' ' || rawLine[0] == '\t')) // only append non-blank, indented lines
        {
            concatenatedLine += "\n" + rawLine;
        }
        else {
            if (startLineNumber != -1 && !opp_isblank(concatenatedLine.c_str())) {
                int numLines = lineNumber - startLineNumber;
                processLine(concatenatedLine, startLineNumber, numLines);
            }
            concatenatedLine = rawLine;
            startLineNumber = lineNumber;
        }
    }

    // last line
    if (startLineNumber != -1 && !opp_isblank(concatenatedLine.c_str())) {
        if (!concatenatedLine.empty() && concatenatedLine.back() == '\\')
            concatenatedLine.resize(concatenatedLine.size()-1);  // remove final stray backslash
        int numLines = lineNumber - startLineNumber;
        processLine(concatenatedLine, startLineNumber, numLines);
    }
}

const char *IniParser::findEndContent(const char *line,
                                      const char *filename,
                                      int lineNumber)
{
    // TODO:
    // - scan through string
    // - skip quoted literals
    // - find '#' comment start
    // - return pointer to the end of meaningful content
    return nullptr;
}

void IniParser::rtrim(std::string &str)
{
    // - remove trailing spaces/tabs
    int end = str.size() - 1;
    while (end >= 0 && std::isspace(static_cast<unsigned char>(str[end])))
        end--;

    if (end < 0) {
        str.clear();
        return;
    }
    str.resize(end + 1);
}

std::string IniParser::trim(const char *start, const char *end)
{
    // TODO:
    // - return substring with left+right spaces removed
    return "";
}

} // namespace wsn
} // namespace ns3