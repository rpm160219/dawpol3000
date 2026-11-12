#!/bin/bash
# DAW-POL 3000 — starter PortMastera (Knulli, muOS, ROCKNIX, ArkOS...). Log: ports/dawpol3000/log.txt
XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}
if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi
source "$controlfolder/control.txt"
[ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"
get_controls

GAMEDIR="/$directory/ports/dawpol3000"
cd "$GAMEDIR" || exit 1
chmod +x ./dawpol3000
# Program czyta pada przez SDL (bez gptokeyb). Wyjscie: Menu + Select.
./dawpol3000 > "$GAMEDIR/log.txt" 2>&1
pm_finish
