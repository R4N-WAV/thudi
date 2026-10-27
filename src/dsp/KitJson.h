// Thudi — minimal JSON reader/writer for .thudikit / .thudipad (dependency free; used by tools and tests)
// Format:
// { "format":"thudi-kit", "version":1, "name":"RHYNO",
//   "global": { "master":0, ... },
//   "pads": [ { "label":"KICK", "note":36, "params": { "model":0, "tone_pitch":99, ... } }, ... ] }
// A pad preset is { "format":"thudi-pad", "version":1, "label":"...", "params":{...} }
#pragma once
#include <cctype>
#include <cstdlib>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "ThudiParams.h"

namespace thudi::json
{
struct Value
{
    enum Type { Null, Num, Str, Arr, Obj, Bool } type = Null;
    double num = 0; std::string str; bool b = false;
    std::vector<Value> arr; std::vector<std::pair<std::string, Value>> obj;
    const Value* get (const std::string& k) const { for (auto& kv : obj) if (kv.first == k) return &kv.second; return nullptr; }
};

struct Parser
{
    const std::string& s; size_t i = 0; bool ok = true;
    explicit Parser (const std::string& src) : s (src) {}
    void ws() { while (i < s.size() && std::isspace ((unsigned char) s[i])) ++i; }
    bool eat (char c) { ws(); if (i < s.size() && s[i] == c) { ++i; return true; } return false; }
    std::string parseString()
    {
        std::string out; if (! eat ('"')) { ok = false; return out; }
        while (i < s.size() && s[i] != '"')
        {
            char c = s[i++];
            if (c == '\\' && i < s.size())
            {
                char e = s[i++];
                if (e == 'n') out += '\n'; else if (e == 't') out += '\t';
                else if (e == 'u' && i + 4 <= s.size())
                {
                    unsigned cp = (unsigned) std::strtoul (s.substr (i, 4).c_str(), nullptr, 16); i += 4;
                    if (cp < 0x80) out += (char) cp;
                    else if (cp < 0x800) { out += (char) (0xC0 | (cp >> 6)); out += (char) (0x80 | (cp & 0x3F)); }
                    else { out += (char) (0xE0 | (cp >> 12)); out += (char) (0x80 | ((cp >> 6) & 0x3F)); out += (char) (0x80 | (cp & 0x3F)); }
                }
                else out += e;
            }
            else out += c;
        }
        ++i; return out;
    }
    Value parse()
    {
        ws(); Value v;
        if (i >= s.size()) { ok = false; return v; }
        char c = s[i];
        if (c == '{')
        {
            ++i; v.type = Value::Obj; ws();
            if (eat ('}')) return v;
            do { ws(); std::string k = parseString(); if (! eat (':')) { ok = false; return v; } v.obj.emplace_back (k, parse()); } while (eat (','));
            if (! eat ('}')) ok = false;
        }
        else if (c == '[')
        {
            ++i; v.type = Value::Arr; ws();
            if (eat (']')) return v;
            do { v.arr.push_back (parse()); } while (eat (','));
            if (! eat (']')) ok = false;
        }
        else if (c == '"') { v.type = Value::Str; v.str = parseString(); }
        else if (s.compare (i, 4, "true") == 0) { v.type = Value::Bool; v.b = true; i += 4; }
        else if (s.compare (i, 5, "false") == 0) { v.type = Value::Bool; i += 5; }
        else if (s.compare (i, 4, "null") == 0) { i += 4; }
        else { char* end = nullptr; v.num = std::strtod (s.c_str() + i, &end); v.type = Value::Num; i = (size_t) (end - s.c_str()); }
        return v;
    }
};

inline void readParams (const Value* params, PadSound& snd)
{
    if (! params) return;
    const auto& t = paramTable();
    for (int k = 0; k < kNumPadParams; ++k)
        if (auto* v = params->get (t[(size_t) k].id))
            if (v->type == Value::Num) snd.v[(size_t) k] = (float) v->num;
}

struct KitPad { std::string label; int note = 36; PadSound sound; std::string sample; };
struct Kit
{
    std::string name = "Init";
    float global[kNumGlobalParams] {};
    KitPad pads[kNumPads];
    KitPad solo;            // the solo-mode sound (one sound across the keyboard)
    Kit()
    {
        solo.label = ""; solo.note = 60;
        for (int g = 0; g < kNumGlobalParams; ++g) global[g] = globalTable()[(size_t) g].def;
        for (int p = 0; p < kNumPads; ++p) { pads[p].note = 36 + p; pads[p].label = "PAD " + std::to_string (p + 1); }
    }
};

inline bool parseKit (const std::string& text, Kit& kit)
{
    Parser ps (text); Value root = ps.parse();
    if (! ps.ok || root.type != Value::Obj) return false;
    if (auto* n = root.get ("name")) kit.name = n->str;
    if (auto* g = root.get ("global"))
        for (int k = 0; k < kNumGlobalParams; ++k)
            if (auto* v = g->get (globalTable()[(size_t) k].id)) kit.global[k] = (float) v->num;
    if (auto* pads = root.get ("pads"))
        for (size_t p = 0; p < pads->arr.size() && p < (size_t) kNumPads; ++p)
        {
            const auto& pv = pads->arr[p];
            if (auto* l = pv.get ("label")) kit.pads[p].label = l->str;
            if (auto* n = pv.get ("note")) kit.pads[p].note = (int) n->num;
            readParams (pv.get ("params"), kit.pads[p].sound);
            if (auto* sm = pv.get ("sample")) kit.pads[p].sample = sm->str;
        }
    if (auto* so = root.get ("solo"))
    {
        if (auto* l = so->get ("label")) kit.solo.label = l->str;
        readParams (so->get ("params"), kit.solo.sound);
        if (auto* sm = so->get ("sample")) kit.solo.sample = sm->str;
    }
    return true;
}

inline bool parsePad (const std::string& text, KitPad& pad)
{
    Parser ps (text); Value root = ps.parse();
    if (! ps.ok || root.type != Value::Obj) return false;
    if (auto* l = root.get ("label")) pad.label = l->str;
    readParams (root.get ("params"), pad.sound);
    if (auto* sm = root.get ("sample")) pad.sample = sm->str;
    return true;
}

inline std::string esc (const std::string& s)
{
    std::string o; for (char c : s) { if (c == '"' || c == '\\') o += '\\'; o += c; } return o;
}

inline std::string fmtNum (float x)
{
    std::ostringstream ss; ss.precision (9); ss << x; return ss.str();
}

inline std::string paramsJson (const PadSound& s, const std::string& indent)
{
    std::string o = "{";
    const auto& t = paramTable();
    for (int k = 0; k < kNumPadParams; ++k)
        o += std::string (k ? "," : "") + "\n" + indent + "  \"" + t[(size_t) k].id + "\": " + fmtNum (s.v[(size_t) k]);
    return o + "\n" + indent + "}";
}

inline std::string writeKit (const Kit& kit)
{
    std::string o = "{\n  \"format\": \"thudi-kit\",\n  \"version\": 1,\n  \"name\": \"" + esc (kit.name) + "\",\n  \"global\": {";
    for (int g = 0; g < kNumGlobalParams; ++g)
        o += std::string (g ? "," : "") + "\n    \"" + globalTable()[(size_t) g].id + "\": " + fmtNum (kit.global[g]);
    o += "\n  },\n  \"pads\": [";
    for (int p = 0; p < kNumPads; ++p)
        o += std::string (p ? "," : "") + "\n    {\n      \"label\": \"" + esc (kit.pads[p].label) + "\",\n      \"note\": " + std::to_string (kit.pads[p].note)
           + (kit.pads[p].sample.empty() ? std::string() : ",\n      \"sample\": \"" + esc (kit.pads[p].sample) + "\"")
           + ",\n      \"params\": " + paramsJson (kit.pads[p].sound, "      ") + "\n    }";
    o += "\n  ],\n  \"solo\": {\n    \"label\": \"" + esc (kit.solo.label) + "\""
       + (kit.solo.sample.empty() ? std::string() : ",\n    \"sample\": \"" + esc (kit.solo.sample) + "\"")
       + ",\n    \"params\": " + paramsJson (kit.solo.sound, "    ") + "\n  }";
    return o + "\n}\n";
}

inline std::string writePad (const KitPad& pad)
{
    return "{\n  \"format\": \"thudi-pad\",\n  \"version\": 1,\n  \"label\": \"" + esc (pad.label) + "\","
         + (pad.sample.empty() ? std::string() : "\n  \"sample\": \"" + esc (pad.sample) + "\",") + "\n  \"params\": " + paramsJson (pad.sound, "  ") + "\n}\n";
}
} // namespace thudi::json
