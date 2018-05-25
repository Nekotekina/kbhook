#!/bin/bash

CURRENT_OLD=`/usr/bin/gsettings get org.gnome.desktop.input-sources current`

if [ "$CURRENT_OLD" == "uint32 2" ]; then
  /usr/bin/gsettings set org.gnome.desktop.input-sources current 0
else
  /usr/bin/gsettings set org.gnome.desktop.input-sources current 0
  /usr/bin/gsettings set org.gnome.desktop.input-sources current 2
fi

CURRENT=`gdbus call --session --dest org.gnome.Shell --object-path /org/gnome/Shell --method org.gnome.Shell.Eval "imports.ui.status.keyboard.getInputSourceManager().currentSource.index"`

if [ "$CURRENT" == "(true, '2')" ]; then
  gdbus call --session --dest org.gnome.Shell --object-path /org/gnome/Shell --method org.gnome.Shell.Eval "imports.ui.status.keyboard.getInputSourceManager().inputSources[0].activate()"
else
  gdbus call --session --dest org.gnome.Shell --object-path /org/gnome/Shell --method org.gnome.Shell.Eval "imports.ui.status.keyboard.getInputSourceManager().inputSources[0].activate()"
  gdbus call --session --dest org.gnome.Shell --object-path /org/gnome/Shell --method org.gnome.Shell.Eval "imports.ui.status.keyboard.getInputSourceManager().inputSources[2].activate()"
fi

