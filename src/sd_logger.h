#pragma once

#include "radiacode_data.h"

bool sdInit();
bool sdReady();
void sdLogData(const DataState& st, bool enabled);
void sdWriteLine(const char* line);
