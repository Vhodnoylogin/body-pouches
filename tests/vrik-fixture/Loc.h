#pragma once
#include "LocStrings.h"
namespace BodyPouches {
struct Loc {
    template<class... T> static void Info(const char*, T&&...) {}
    template<class... T> static void Warn(const char*, T&&...) {}
    template<class... T> static void Error(const char*, T&&...) {}
};
}
