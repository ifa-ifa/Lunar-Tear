#pragma once
#include<vector>
#include "Formats/TextureFormats.h"
#include<string>


void dumpScript(const std::string &name, const std::vector<char> &data);
void dumpTexture(const tpgxResTexture* tex);

void dumpTable(const std::string& name, const char* data);
