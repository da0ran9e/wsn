#ifndef INI_PARSER_H
#define INI_PARSER_H

#include <string>
#include <vector>
#include <istream>
#include <iostream>
#include <functional>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>   
#include <set>          
#include <cstring>      

namespace ns3 {
namespace wsn {

class IniParser
{
public:
    class Listener {
    public:
        virtual void onSection(const std::string& section) = 0;
        virtual void onKeyValue(const std::string& key,
                                const std::string& value,
                                const std::string& section,
                                const std::string& baseDir) = 0;
    };

public:
    IniParser();
    ~IniParser();

    void setListener(Listener* listener) { m_listener = listener; }
    void read(const std::string &filename);
    void readFile(const std::string &filename);
    void readText(const std::string &text, const std::string &filename = "", const std::string &baseDir = "");
    void readStream(std::istream &in, const std::string &filename, const std::string &baseDir);
    std::string getFilename() const { return m_filename; }
    void setFilename(const std::string &filename) { m_filename = filename; }

private: 
    std::string m_filename;
    void doReadFile(const std::string &filename, std::vector<std::string> &includeStack); 
    void doReadFromStream(std::istream &in, const std::string &filename, std::vector<std::string> &includeStack, const std::string &absoluteFilename, const std::string &baseDir);
    void forEachJoinedLine(std::istream &in, const std::function<void(std::string &, int, int)> &processLine);
    const char *findEndContent(const char *line, const char *filename, int lineNumber); 
    void rtrim(std::string &str); 
    std::string trim(const char *start, const char *end);

private:
    Listener* m_listener = nullptr;
};

} // namespace wsn
} // namespace ns3

#endif // INI_PARSER_H
