#pragma once

#include "libs/skCrypter/skCrypter.h"

#include "libs/ImGui/imgui.h"
#include "libs/ImGui/imgui_impl_win32.h"
#include "libs/ImGui/imgui_impl_dx11.h"

#pragma comment(lib, "d3d11.lib")
#include <d3d11.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>
#include <cstring>

#include "Injection.h"