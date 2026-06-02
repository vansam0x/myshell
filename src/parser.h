#pragma once
#include <string>
#include <vector>

struct ParsedCommand {
    std::string command;
    std::vector <std::string> args; 
    size_t argc;
    bool isBackground; 
    std::string fullCommandLine;
    ParsedCommand() : argc(0), isBackground(false) {} 
};

ParsedCommand parse_command(const std::string & input) {
    ParsedCommand result;
    result.fullCommandLine = input;
    std::string current;
    for (size_t i = 0; i < input.size(); ++i) {
        if(input[i] == ' ') {
            if(!current.empty()) {
                result.args.emplace_back(current);
                current.clear();
            }
            else continue;
        } else {
            current.push_back(input[i]);
        }
    }
    if(!current.empty()) {
        result.args.emplace_back(current);
    }
    if (!result.args.empty()) {
        result.command = result.args[0];
        result.argc = result.args.size();
        if(result.args.back() == "&") {
            result.isBackground = true;
            result.args.pop_back();
            --result.argc;
        } else {
            result.isBackground = false;
        }
    }
    return result;

}