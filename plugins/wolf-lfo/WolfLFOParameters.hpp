#ifndef WOLF_LFO_PARAMETERS_HPP_INCLUDED
#define WOLF_LFO_PARAMETERS_HPP_INCLUDED

#include "src/DistrhoDefines.h"

START_NAMESPACE_DISTRHO

enum Parameters
{
    paramPreGain = 0,
    paramWet,
    paramPostGain,
    paramHorizontalWarpType,
    paramHorizontalWarpAmount,
    paramLFORate,
    paramBPMSync,
    paramPhase,
    paramSmoothing,
    paramPlayheadPos,
    paramCount
};

END_NAMESPACE_DISTRHO

#endif