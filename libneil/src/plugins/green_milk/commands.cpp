/*
  kibibu Green Milk
  Commands class - just describes the set of available commands
  Copyright (C) 2007  Cameron Foale

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "commands.h"

const char *Commands::describeCommand(unsigned char cmd)
{
  switch(cmd) {
  case Commands::RestartAmpEnvelope:
    return "Rstrt Amp Env";
  case Commands::RestartFilterEnvelope:
    return "Rstrt Flt Env";
  case Commands::RestartBothEnvelopes:
    return "Rstrt Both Envs";
  case Commands::SetAmpEnvelopeValue:
    return "Set Amp Env";
  case Commands::SetFilterEnvelopeValue:
    return "Set Filt Env";
  case  Commands::RandomiseUnisonPhase:
    return "Rnd Unison Phase";
  case  Commands::SynchroniseUnisonPhase:
    return "Synch Unison Phase";
  case  Commands::RandomiseUnisonLFOPhase:
    return "Rnd Unison LFO";
  case  Commands::SynchroniseUnisonLFOPhase:
    return "Synch Unison LFO";
  case  Commands::RandomiseUnisonPitchOffset:
    return "Rnd Unison Pitch (xx=range, y.y=interval)";
  case  Commands::SetUnisonPitchOffset:
    return "Set Unison Pitch";
  case  Commands::SetUnisonPhase:
    return "Set Unison Osc Phase";
  case  Commands::SetUnisonLFOPhase:
    return "Set Unison LFO";
  case  Commands::SetUnisonDepth:
    return "Set Unison Depth";
  case  Commands::SetUnisonSpeed:
    return "Set Unison Speed (ticks/16)";
  case  Commands::IgnoreNewParams:
    return "Ignore new global parameters (this tick only)";
  case  Commands::RestartLFO1:
    return "TLFO1: Restart";
  case  Commands::PauseLFO1:
    return "TLFO1: Pause";
  case  Commands::ResumeLFO1:
    return "TLFO1: Resume";
  case  Commands::SkipLFO1Delay:
    return "TLFO1: Skip delay";
  case  Commands::SetLFO1Wave:
    return "TLFO1: Set wave";
  case  Commands::SetLFO1Phase:
    return "TLFO1: Set phase";
  case  Commands::SetLFO1Frequency:
    return "TLFO1: Set freq (t/16)";
  case  Commands::RestartLFO2:
    return "TLFO2: Restart";
  case  Commands::PauseLFO2:
    return "TLFO2: Pause";
  case  Commands::ResumeLFO2:
    return "TLFO2: Resume";
  case  Commands::SkipLFO2Delay:
    return "TLFO2: Skip delay";
  case  Commands::SetLFO2Wave:
    return "TLFO2: Set wave";
  case  Commands::SetLFO2Phase:
    return "TLFO2: Set phase";
  case  Commands::SetLFO2Frequency:
    return "TLFO2: Set freq (t/16)";
  }
  return "No command";
}
