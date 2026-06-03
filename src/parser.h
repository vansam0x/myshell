#pragma once
#include <string>
#include <vector>
#include <algorithm>

struct ParsedCommand {
    std::string command;
    std::vector <std::string> args; 
    size_t argc;
    bool isBackground; 
    std::string fullCommandLine;
    std::string inputFile;    
    std::string outputFile;   
    std::string appendFile;   
    bool pipeNext;            
    
    ParsedCommand() : argc(0), isBackground(false), pipeNext(false) {} 
};

ParsedCommand parse_command(const std::string & input) {
    ParsedCommand result;
    result.fullCommandLine = input;
    
    std::string current;
    bool inQuotes = false;
    
    // Loại bỏ pipe và redirection tạm thời
    std::string workInput = input;
    size_t pipePos = workInput.find('|');
    if (pipePos != std::string::npos) {
        result.pipeNext = true;
        workInput = workInput.substr(0, pipePos);
    }
    
    // Xử lý redirection
    size_t appendPos = workInput.find(">>");
    if (appendPos != std::string::npos) {
        result.appendFile = workInput.substr(appendPos + 2);
        // Trim whitespace
        size_t start = result.appendFile.find_first_not_of(" \t");
        if (start != std::string::npos) {
            result.appendFile = result.appendFile.substr(start);
            size_t end = result.appendFile.find_last_not_of(" \t");
            result.appendFile = result.appendFile.substr(0, end + 1);
        }
        workInput = workInput.substr(0, appendPos);
    } else {
        size_t redirectOut = workInput.find('>');
        if (redirectOut != std::string::npos) {
            result.outputFile = workInput.substr(redirectOut + 1);
            // Trim whitespace
            size_t start = result.outputFile.find_first_not_of(" \t");
            if (start != std::string::npos) {
                result.outputFile = result.outputFile.substr(start);
                size_t end = result.outputFile.find_last_not_of(" \t");
                result.outputFile = result.outputFile.substr(0, end + 1);
            }
            workInput = workInput.substr(0, redirectOut);
        }
    }
    
    size_t redirectIn = workInput.find('<');
    if (redirectIn != std::string::npos) {
        result.inputFile = workInput.substr(redirectIn + 1);
        // Trim whitespace
        size_t start = result.inputFile.find_first_not_of(" \t");
        if (start != std::string::npos) {
            result.inputFile = result.inputFile.substr(start);
            size_t end = result.inputFile.find_last_not_of(" \t");
            result.inputFile = result.inputFile.substr(0, end + 1);
        }
        workInput = workInput.substr(0, redirectIn);
    }
    
    // Parse arguments với support quoted strings
    for (size_t i = 0; i < workInput.size(); ++i) {
        char c = workInput[i];
        
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if ((c == ' ' || c == '\t') && !inQuotes) {
            if (!current.empty()) {
                result.args.emplace_back(current);
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    
    if (!current.empty()) {
        result.args.emplace_back(current);
    }
    
    // Extract command và argc
    if (!result.args.empty()) {
        result.command = result.args[0];
        result.argc = result.args.size();
        
        // Check background flag
        if (!result.args.empty() && result.args.back() == "&") {
            result.isBackground = true;
            result.args.pop_back();
            --result.argc;
        } else {
            result.isBackground = false;
        }
    }
    
    return result;
}