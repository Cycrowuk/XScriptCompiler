// XScript.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <sstream>

#include "../XScriptLib/XScriptLib.h"
#include <BaseParse.cpp>

#pragma warning(disable:4996)

int main(int argc, char *argv[])
{
    std::cout << "XScript Compiler V" << XSCRIPT_VERSION;
#ifdef XSCRIPT_BETA
    std::cout << " (BETA)";
#endif
    std::cout << std::endl;

    std::map<std::string, std::string> commands;

    std::string currentCmd;
    std::stringstream currentRest;
    for (int i = 1; i < argc; i++)
    {
        std::string str(argv[i]);

        if (str.substr(0, 2) == "--")
        {
            if (!currentCmd.empty())
                commands[currentCmd] = currentRest.str();
            currentCmd = str.substr(2);
            currentRest.str(std::string());
        }
        else if (!currentCmd.empty())
        {
            if (!currentRest.str().empty())
                currentRest << " ";
            currentRest << str;
        }
        else
        {
            std::cout << "Syntax error: " << str << std::endl;
            return 1;
        }
    }

    if (!currentCmd.empty())
        commands[currentCmd] = currentRest.str();

    std::string exe = argv[0];
    auto pos = exe.find_last_of('\\');
    if (pos != std::string::npos)
        exe = exe.substr(pos + 1);

    if (commands.empty())
    {
        std::cout << "Syntax:" << std::endl;
        std::cout << "\t" << exe << " --load_data <datafile> --compile <script.XScript> --out <my.script.xml>" << std::endl;
        std::cout << "\t\t - Compiles an XScript file into a game script" << std::endl;
        std::cout << "\t" << exe << " --load_data <datafile> --compile <script.XScript> --out <my.script.xml> --define:MYSYMBOL" << std::endl;
        std::cout << "\t\t - Compiles with a pre-defined symbol (multiple --define:NAME arguments supported)" << std::endl;
        std::cout << "\t" << exe << " --load_data <datafile> --decompile <my.script.xml> --out <out.XScript>" << std::endl;
        std::cout << "\t\t - Decompiles a game script file into an XScript file" << std::endl;
        std::cout << "\t" << exe << " --load_data <datafile> --decompile <my.script.xml> --out <out.XScript> --usenamespace" << std::endl;
        std::cout << "\t\t - Decompiles using namespace syntax (e.g. Utils::random instead of random)" << std::endl;
        std::cout << "\t" << exe << " --builddata <game.xml> --out <data.dat>" << std::endl;
        std::cout << "\t\t - Builds a game data file from an xml reference" << std::endl;
        std::cout << "\t" << exe << " --load_data <datafile> --exportudl" << std::endl;
        std::cout << "\t\t - Exports notepad++ User Defined Language file" << std::endl;
        return 0;
    }

    //--load_data XScript_X3FL.dat --out decompile.XScript --decompile d:/x/x3 Terran Conflict/addon2/scripts/plugin.property.menu.xml
    //--load_data XScript_X3FL.dat --compile decompile.XScript --out d:/test.xml
    //--load_data Data/XScript_X3FL.dat --compile script.XScript --out compiled.xml
    try
    {
        enum class CommandType
        {
            None,
            Compile,
            Decompile,
            BuildData,
            ExportUDL,
        };

        bool outputNeeded = false;
        std::string output;
        std::string file;
        std::string data = "default_data.dat";
        CommandType type = CommandType::None;
        CommandType setType = CommandType::None;
        std::vector<std::string> defines;
        bool useNamespace = false;

        for (auto itr = commands.begin(); itr != commands.end(); itr++)
        {
            if (itr->first == "load_data")
                data = itr->second;
            else if (itr->first == "out")
                output = itr->second;
            else if (itr->first.substr(0, 7) == "define:")
            {
                // --define:NAME  — pre-define a symbol for #ifdef use
                std::string defineName = itr->first.substr(7);
                if (!defineName.empty())
                    defines.push_back(defineName);
                else
                    std::cout << "Warning: --define: requires a symbol name, e.g. --define:MYSYMBOL" << std::endl;
            }
            else if (itr->first == "compile")
            {
                outputNeeded = true;
                setType = CommandType::Compile;
                file = itr->second;
            }
            else if (itr->first == "usenamespace")
            {
                useNamespace = true;
            }
            else if (itr->first == "decompile")
            {
                outputNeeded = true;
                setType = CommandType::Decompile;
                file = itr->second;
            }
            else if (itr->first == "builddata")
            {
                outputNeeded = true;
                setType = CommandType::BuildData;
                file = itr->second;
            }
            else if (itr->first == "exportudl")
            {
                outputNeeded = false;
                setType = CommandType::ExportUDL;
                file = itr->second;
            }

            if (setType != CommandType::None)
            {
                if (type != CommandType::None)
                {
                    std::cout << "Syntax Error, command mistach '--" << itr->first << ", command type already specificed" << std::endl;
                    return 1;
                }
                type = setType;
                setType = CommandType::None;
            }
        }

        if (type == CommandType::None)
        {
            std::cout << "Syntax Error: missing command type" << std::endl;
            return 1;
        }
        if (outputNeeded && output.empty())
        {
            std::cout << "Syntax Error, missing output file, use --out <filename>" << std::endl;
            return 1;
        }

#ifdef _DEBUG
        loadXmlData("XML/x3fl.xml", data);
#endif
        if (type != CommandType::BuildData)
        {
            std::cout << "Loading data file: " << data << "...  ";
            if (!loadData(data))
            {
                std::cout << "FAILED" << std::endl;
                std::cout << "  Note: if the data file was built with an older version of XScriptCompiler," << std::endl;
                std::cout << "  please rebuild it using: --builddata <x3fl.xml> --out <data.dat>" << std::endl;
                return 1;
            }
            std::cout << "OK" << std::endl;
        }

        switch (type)
        {
        case CommandType::Compile:
            std::cout << "Compiling script: " << file << "...  ";
            if (!compileScriptFile(file, output, defines))
            {
#ifdef _DEBUG
                if (BaseParse::_DEBUG_COUNT != 0)
                {
                    int debug = 1;
                }
#endif
                std::cout << "FAILED" << std::endl;
                return 1;
            }
            std::cout << "OK" << std::endl;
            std::cout << "\tScript file: " << output << " has been written" << std::endl;
#ifdef _DEBUG
            if (BaseParse::_DEBUG_COUNT != 0)
            {
                int debug = 1;
            }
#endif
            break;
        case CommandType::Decompile:
            std::cout << "Decompiling script: " << file << "...  ";
            if (!decompileScriptFile(file, output, useNamespace))
            {
                std::cout << "FAILED" << std::endl;
                return 1;
            }
            std::cout << "OK" << std::endl;
            std::cout << "\tXScript file: " << output << " has been written" << std::endl;
            break;
        case CommandType::BuildData:
            std::cout << "Building new data file: " << file << "...  ";
            if (!loadXmlData(file, output))
            {
                std::cout << "FAILED" << std::endl;
                return 1;
            }
            std::cout << "OK" << std::endl;
            std::cout << "\tData file: " << output << " has been written" << std::endl;
            break;
        case CommandType::ExportUDL:
            std::cout << "Building new notepad++ User Defined Language file: " << file << "...  ";
            if (!exportUDL("XScript_UDL.xml", "xscript.xml"))
            {
                std::cout << "FAILED" << std::endl;
                return 1;
            }
            std::cout << "OK" << std::endl;
            std::cout << "\tAutocomplete file: xscript.xml written." << std::endl;
            std::cout << "\tUser Defined Language file: xscript_UDL.xml written." << std::endl;
            break;

        default:
            std::cout << "Syntax Error: missing command type" << std::endl;
            return 1;
        }
    }
    catch (std::exception& e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
