// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

// Based on code in ImGui v1.65, which is:
// Copyright (c) 2014-2018 Omar Cornut
// Licensed under the MIT Licence
// See https://opensource.org/licenses/MIT

#include <imgui.h>
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui_internal.h>
using namespace ImGui;

#include "scale_picker.h"

#include <ctype.h>      // toupper, isprint
#include <stdint.h>     // intptr_t
#include <iostream>

// Those MIN/MAX values are not define because we need to point to them
#if 0
static const ImS32  IM_S32_MIN = INT_MIN;    // (-2147483647 - 1), (0x80000000);
static const ImS32  IM_S32_MAX = INT_MAX;    // (2147483647), (0x7FFFFFFF)
static const ImU32  IM_U32_MIN = 0;
static const ImU32  IM_U32_MAX = UINT_MAX;   // (0xFFFFFFFF)
#ifdef LLONG_MIN
static const ImS64  IM_S64_MIN = LLONG_MIN;  // (-9223372036854775807ll - 1ll);
static const ImS64  IM_S64_MAX = LLONG_MAX;  // (9223372036854775807ll);
#else
static const ImS64  IM_S64_MIN = -9223372036854775807LL - 1;
static const ImS64  IM_S64_MAX = 9223372036854775807LL;
#endif
static const ImU64  IM_U64_MIN = 0;
#ifdef ULLONG_MAX
static const ImU64  IM_U64_MAX = ULLONG_MAX; // (0xFFFFFFFFFFFFFFFFull);
#else
static const ImU64  IM_U64_MAX = (2ULL * 9223372036854775807LL + 1);
#endif
#endif

struct ImGuiDataTypeInfo
{
    size_t      Size;
    const char* PrintFmt;   // Unused
    const char* ScanFmt;
};

static const ImGuiDataTypeInfo GDataTypeInfo[] =
{
    { sizeof(int),          "%d",   "%d"    },
    { sizeof(unsigned int), "%u",   "%u"    },
#ifdef _MSC_VER
    { sizeof(ImS64),        "%I64d","%I64d" },
    { sizeof(ImU64),        "%I64u","%I64u" },
#else
    { sizeof(ImS64),        "%lld", "%lld"  },
    { sizeof(ImU64),        "%llu", "%llu"  },
#endif
    { sizeof(float),        "%f",   "%f"    },  // float are promoted to double in va_arg
    { sizeof(double),       "%f",   "%lf"   },
};
IM_STATIC_ASSERT(IM_ARRAYSIZE(GDataTypeInfo) == ImGuiDataType_COUNT);

// FIXME-LEGACY: Prior to 1.61 our DragInt() function internally used floats and because of this the compile-time default value for format was "%.0f".
// Even though we changed the compile-time default, we expect users to have carried %f around, which would break the display of DragInt() calls.
// To honor backward compatibility we are rewriting the format string, unless IMGUI_DISABLE_OBSOLETE_FUNCTIONS is enabled. What could possibly go wrong?!
static const char*
PatchFormatStringFloatToInt(const char* fmt)
{
    if (fmt[0] == '%' && fmt[1] == '.' && fmt[2] == '0' && fmt[3] == 'f' && fmt[4] == 0) // Fast legacy path for "%.0f" which is expected to be the most common case.
        return "%d";
    const char* fmt_start = ImParseFormatFindStart(fmt);    // Find % (if any, and ignore %%)
    const char* fmt_end = ImParseFormatFindEnd(fmt_start);  // Find end of format specifier, which itself is an exercise of confidence/recklessness (because snprintf is dependent on libc or user).
    if (fmt_end > fmt_start && fmt_end[-1] == 'f')
    {
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
        if (fmt_start == fmt && fmt_end[0] == 0)
            return "%d";
        ImGuiContext& g = *GImGui;
        ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), "%.*s%%d%s", (int)(fmt_start - fmt), fmt, fmt_end); // Honor leading and trailing decorations, but lose alignment/precision.
        return g.TempBuffer;
#else
        IM_ASSERT(0 && "DragInt(): Invalid format string!"); // Old versions used a default parameter of "%.0f", please replace with e.g. "%d"
#endif
    }
    return fmt;
}

static inline int
DataTypeFormatString(char* buf, int buf_size, ImGuiDataType data_type, const void* data_ptr, const char* format)
{
    if (data_type == ImGuiDataType_S32 || data_type == ImGuiDataType_U32)   // Signedness doesn't matter when pushing the argument
        return ImFormatString(buf, buf_size, format, *(const ImU32*)data_ptr);
    if (data_type == ImGuiDataType_S64 || data_type == ImGuiDataType_U64)   // Signedness doesn't matter when pushing the argument
        return ImFormatString(buf, buf_size, format, *(const ImU64*)data_ptr);
    if (data_type == ImGuiDataType_Float)
        return ImFormatString(buf, buf_size, format, *(const float*)data_ptr);
    if (data_type == ImGuiDataType_Double)
        return ImFormatString(buf, buf_size, format, *(const double*)data_ptr);
    IM_ASSERT(0);
    return 0;
}

static float
GetMinimumStepAtDecimalPrecision(int decimal_precision)
{
    static const float min_steps[10] = { 1.0f, 0.1f, 0.01f, 0.001f, 0.0001f, 0.00001f, 0.000001f, 0.0000001f, 0.00000001f, 0.000000001f };
    if (decimal_precision < 0)
        return FLT_MIN;
    return (decimal_precision >= 0 && decimal_precision < 10) ? min_steps[decimal_precision] : ImPow(10.0f, (float)-decimal_precision);
}

// This is called by DragBehavior() when the widget is active (held by mouse or being manipulated with Nav controls)
template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
bool
MyDragBehaviorT(ImGuiDataType data_type, TYPE* v, float v_speed, const TYPE v_min, const TYPE v_max, const char* format, float power)
{
    ImGuiContext& g = *GImGui;

    // Default tweak speed
    bool has_min_max = (v_min != v_max) && (v_max - v_max < FLT_MAX);
#if 0
    if (v_speed == 0.0f && has_min_max)
        v_speed = (float)((v_max - v_min) * g.DragSpeedDefaultRatio);
std::cerr << ","<<v_speed;
#endif

    // Inputs accumulates into g.DragCurrentAccum, which is flushed into the current value as soon as it makes a difference with our precision settings
    float adjust_delta = 0.0f;
    if (g.ActiveIdSource == ImGuiInputSource_Mouse && IsMousePosValid() && g.IO.MouseDragMaxDistanceSqr[0] > 1.0f*1.0f)
    {
        adjust_delta = g.IO.MouseDelta.x;
        if (g.IO.KeyAlt)
            adjust_delta *= 1.0f/10.0f;
        if (g.IO.KeyShift)
            adjust_delta *= 10.0f;
    }
    else if (g.ActiveIdSource == ImGuiInputSource_Nav)
    {
        int decimal_precision = (data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double) ? ImParseFormatPrecision(format, 3) : 0;
        adjust_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard|ImGuiNavDirSourceFlags_PadDPad, ImGuiInputReadMode_RepeatFast, 1.0f/10.0f, 10.0f).x;
        v_speed = ImMax(v_speed, GetMinimumStepAtDecimalPrecision(decimal_precision));
    }
    adjust_delta *= v_speed;

    // Clear current value on activation
    // Avoid altering values and clamping when we are _already_ past the limits and heading in the same direction, so e.g. if range is 0..255, current value is 300 and we are pushing to the right side, keep the 300.
    bool is_just_activated = g.ActiveIdIsJustActivated;
    bool is_already_past_limits_and_pushing_outward = has_min_max && ((*v >= v_max && adjust_delta > 0.0f) || (*v <= v_min && adjust_delta < 0.0f));
    if (is_just_activated || is_already_past_limits_and_pushing_outward)
    {
        g.DragCurrentAccum = 0.0f;
        g.DragCurrentAccumDirty = false;
    }
    else if (adjust_delta != 0.0f)
    {
        g.DragCurrentAccum += adjust_delta;
        g.DragCurrentAccumDirty = true;
    }

    if (!g.DragCurrentAccumDirty)
        return false;

    TYPE v_cur = *v;
    //FLOATTYPE v_old_ref_for_accum_remainder = (FLOATTYPE)0.0f;

    const bool is_power = (power != 1.0f);
    if (is_power)
    {
      #if 0
        // Offset + round to user desired precision, with a curve on the v_min..v_max range to get more precision on one side of the range
        FLOATTYPE v_old_norm_curved = ImPow((FLOATTYPE)(v_cur - v_min) / (FLOATTYPE)(v_max - v_min), (FLOATTYPE)1.0f / power);
        FLOATTYPE v_new_norm_curved = v_old_norm_curved + (g.DragCurrentAccum / (v_max - v_min));
        v_cur = v_min + (TYPE)ImPow(ImSaturate((float)v_new_norm_curved), power) * (v_max - v_min);
        v_old_ref_for_accum_remainder = v_old_norm_curved;
      #else
        v_cur *= pow(power, g.DragCurrentAccum);
      #endif
    }
    else
    {
        v_cur += (TYPE)g.DragCurrentAccum;
    }

    // Round to user desired precision based on format string
    //v_cur = RoundScalarWithFormatT<TYPE, SIGNEDTYPE>(format, data_type, v_cur);

#if 0
    // Preserve remainder after rounding has been applied. This also allow slow tweaking of values.
    g.DragCurrentAccumDirty = false;
    if (is_power)
    {
        FLOATTYPE v_cur_norm_curved = ImPow((FLOATTYPE)(v_cur - v_min) / (FLOATTYPE)(v_max - v_min), (FLOATTYPE)1.0f / power);
        g.DragCurrentAccum -= (float)(v_cur_norm_curved - v_old_ref_for_accum_remainder);
    }
    else
    {
        g.DragCurrentAccum -= (float)((SIGNEDTYPE)v_cur - (SIGNEDTYPE)*v);
    }
#endif

    // Lose zero sign for float/double
    if (v_cur == (TYPE)-0)
        v_cur = (TYPE)0;

    // Clamp values (handle overflow/wrap-around)
#if 0
    if (*v != v_cur && has_min_max)
    {
        if (v_cur < v_min || (v_cur > *v && adjust_delta < 0.0f))
            v_cur = v_min;
        if (v_cur > v_max || (v_cur < *v && adjust_delta > 0.0f))
            v_cur = v_max;
    }
#else
    if (v_cur < FLT_MIN) v_cur = FLT_MIN;
    if (v_cur > FLT_MAX) v_cur = FLT_MAX;
#endif

    // Apply result
    if (*v == v_cur)
        return false;
    *v = v_cur;
    return true;
}

bool
MyDragBehavior(ImGuiID id, ImGuiDataType data_type, void* v, float v_speed, const void* v_min, const void* v_max, const char* format, float power)
{
    ImGuiContext& g = *GImGui;
    if (g.ActiveId == id)
    {
        if (g.ActiveIdSource == ImGuiInputSource_Mouse && !g.IO.MouseDown[0])
            ClearActiveID();
        else if (g.ActiveIdSource == ImGuiInputSource_Nav && g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
            ClearActiveID();
    }
    if (g.ActiveId != id)
        return false;

    return MyDragBehaviorT<float, float, float >(data_type, (float*)v,  v_speed, v_min ? *(const float* )v_min : -FLT_MAX,   v_max ? *(const float* )v_max : FLT_MAX,    format, power);
}

bool
MyDragScalar(const char* label, ImGuiDataType data_type, void* v, float v_speed, const void* v_min, const void* v_max, const char* format, float power)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    if (power != 1.0f)
        IM_ASSERT(v_min != NULL && v_max != NULL); // When using a power curve the drag needs to have known bounds

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y*2.0f));
    const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    // NB- we don't call ItemSize() yet because we may turn into a text edit box below
    if (!ItemAdd(total_bb, id, &frame_bb))
    {
        ItemSize(total_bb, style.FramePadding.y);
        return false;
    }
    const bool hovered = ItemHoverable(frame_bb, id);

    // Default format string when passing NULL
    // Patch old "%.0f" format string to use "%d", read function comments for more details.
    IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
    if (format == NULL)
        format = GDataTypeInfo[data_type].PrintFmt;
    else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0)
        format = PatchFormatStringFloatToInt(format);

    // Tabbing or CTRL-clicking on Drag turns it into an input box
    bool start_text_input = false;
    const bool tab_focus_requested = FocusableItemRegister(window, id);
    if (tab_focus_requested || (hovered && (g.IO.MouseClicked[0] || g.IO.MouseDoubleClicked[0])) || g.NavActivateId == id || (g.NavInputId == id && g.ScalarAsInputTextId != id))
    {
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
        g.ActiveIdAllowNavDirFlags = (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
        if (tab_focus_requested || g.IO.KeyCtrl || g.IO.MouseDoubleClicked[0] || g.NavInputId == id)
        {
            start_text_input = true;
            g.ScalarAsInputTextId = 0;
        }
    }
    if (start_text_input || (g.ActiveId == id && g.ScalarAsInputTextId == id))
        return InputScalarAsWidgetReplacement(frame_bb, id, label, data_type, v, format);

    // Actual drag behavior
    ItemSize(total_bb, style.FramePadding.y);
    const bool value_changed = MyDragBehavior(id, data_type, v, v_speed, v_min, v_max, format, power);
    if (value_changed)
        MarkItemEdited(id);

    // Draw frame
    const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    RenderNavHighlight(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, style.FrameRounding);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, v, format);
    RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);

    return value_changed;
}

bool
run_scale_picker(const char* label, float* v)
{
    float v_min = FLT_MIN;
    float v_max = FLT_MAX;
    const char* format = "%g";
    float power = 2.0f;
    float v_speed = 0.0005f;
    bool b = MyDragScalar(label, ImGuiDataType_Float, v,
        v_speed, &v_min, &v_max, format, power);
    if (*v < FLT_MIN) *v = FLT_MIN;
    if (*v > FLT_MAX) *v = FLT_MAX;
    return b;
}
