#pragma once

// Narrow boundary fixture: distinct engine transform fields, no engine/VR simulation.
#include <map>
#include <string>

namespace RE
{
    struct NiPoint3
    {
        float x{}, y{}, z{};
        NiPoint3 operator+(const NiPoint3& p) const { return {x+p.x, y+p.y, z+p.z}; }
        NiPoint3 operator-(const NiPoint3& p) const { return {x-p.x, y-p.y, z-p.z}; }
        NiPoint3 operator*(float s) const { return {x*s, y*s, z*s}; }
        NiPoint3 operator/(float s) const { return {x/s, y/s, z/s}; }
    };
    struct NiMatrix3
    {
        float entry[3][3]{{1,0,0}, {0,1,0}, {0,0,1}};
        NiPoint3 operator*(const NiPoint3& p) const
        {
            return {entry[0][0]*p.x+entry[0][1]*p.y+entry[0][2]*p.z,
                    entry[1][0]*p.x+entry[1][1]*p.y+entry[1][2]*p.z,
                    entry[2][0]*p.x+entry[2][1]*p.y+entry[2][2]*p.z};
        }
        NiMatrix3 Transpose() const
        {
            NiMatrix3 r;
            for (int i=0;i<3;++i) for (int j=0;j<3;++j) r.entry[i][j]=entry[j][i];
            return r;
        }
    };
    struct NiTransform { NiMatrix3 rotate; NiPoint3 translate; float scale{1}; };
    struct NiAVObject
    {
        NiTransform world, previousWorld;
        std::map<std::string, NiAVObject*> children;
        NiAVObject* GetObjectByName(const char* name)
        {
            const auto it = children.find(name);
            return it == children.end() ? nullptr : it->second;
        }
    };
    struct PlayerCharacter
    {
        static inline PlayerCharacter* instance{};
        NiAVObject* firstPerson{};
        NiAVObject* thirdPerson{};
        static PlayerCharacter* GetSingleton() { return instance; }
        NiAVObject* Get3D(bool first) { return first ? firstPerson : thirdPerson; }
    };
}
