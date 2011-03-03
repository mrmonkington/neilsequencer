/*
  Kibibu Green Milk
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

#pragma once

class Commands
{
public:
  enum CommandType {
      RestartAmpEnvelope = 0x01,
      RestartFilterEnvelope = 0x02,
      RestartBothEnvelopes = 0x03,
      SetAmpEnvelopeValue = 0x04,
      SetFilterEnvelopeValue = 0x05,
		
      RandomiseUnisonPhase = 0x10,
      SynchroniseUnisonPhase = 0x11,
      RandomiseUnisonLFOPhase = 0x12,
      SynchroniseUnisonLFOPhase = 0x13,
      RandomiseUnisonPitchOffset = 0x14,
      SetUnisonPitchOffset = 0x15,
      SetUnisonPhase = 0x16,
      SetUnisonLFOPhase = 0x17,
      SetUnisonDepth = 0x18,
      SetUnisonSpeed = 0x19,
      IgnoreNewParams = 0x20,
      RestartLFO1 = 0x30,
      PauseLFO1 = 0x31,
      ResumeLFO1 = 0x32,
      SkipLFO1Delay = 0x33,
      SetLFO1Wave = 0x34,
      SetLFO1Phase = 0x35,
      SetLFO1Frequency = 0x36,
      RestartLFO2 = 0x40,
      PauseLFO2 = 0x41,
      ResumeLFO2 = 0x42,
      SkipLFO2Delay = 0x43,
      SetLFO2Wave = 0x44,
      SetLFO2Phase = 0x45,
      SetLFO2Frequency = 0x46
    };
  static const char *describeCommand(unsigned char cmd);
};
