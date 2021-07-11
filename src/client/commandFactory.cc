//
// Created by chuande on 2021/7/11.
//

#include "commandFactory.h"

CommandFactory::CommandFactory() {
    addCommand();
}

ICommand *CommandFactory::getCommandProcessor(const char *pCmd) {
    ICommand *pProcessor = NULL;
    do {
        COMMAND_MAP::iterator iter;
        iter = _cmdMap.find(pCmd);
        if (iter != _cmdMap.end()) {
            pProcessor = iter->second;
        }
    } while (0);

    return pProcessor;
}


