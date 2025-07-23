Place all the loose files you want to load in a folder with your mod name. To load that mod place that folder in `[gameinstall]/LunarTear/mods/`. There are three types of files this mod supports. If you have questions ask on nexus or github or discord (@iiiba):

## Textures

Extract files by enabling texture dumping in the config. Alternatively something like Replicant2Blender (https://github.com/WoefulWolf/Replicant2Blender) may be more useful. Then change them however you want and save them as dds images. New textures must have the same resolution, mipmap count, and compression format as the original. The last two can be found out by enabling File Info logging.

## Lua scripts

Extract lua scripts using script dumping in the config. Alternatively use "Kaine" by yretenai (https://github.com/yretenai/kaine) and binary templates by WoefulWolf (https://github.com/WoefulWolf/replicant_templates/tree/main). The map scripts are found in `snow/script/`. Decompile using a lua 5.0.2 decompiler (i recommend https://github.com/loosepolygon/luadec-5.0). You will need to manually fix these files. Look out for comments like "Decompiler error" or "Tried to add end here". Its not too hard, its mostly just fixing some if statments by adding ends in the correct space. although you need to pay attention to the program flow. There are some japanese debug prints that got decoded incorrectly, use the python script in `other/` to fix it. You do not need to recompile, just save the sourc code file with .lua extension.

## STBL files

Extract the same way as lua files (table dumping option), they are usually found right alongside each other. use the SETTBLL editor in this repository to edit them. Then save them with .settbll extension.



